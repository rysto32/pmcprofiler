// Copyright (c) 2017 Ryan Stone.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#include "DwarfDieStack.h"

#include "Callframe.h"
#include "DwarfException.h"
#include "DwarfStackState.h"
#include "DwarfUtil.h"

#include <cassert>
#include <dwarf.h>

DwarfDieStack::DwarfDieStack(SharedString imageFile, Dwarf_Debug dwarf, Dwarf_Die cu,
    const SymbolMap & symbols)
  : imageFile(imageFile),
    dwarf(dwarf),
    peekState(dwarf),
    cuDie(cu),
    elfSymbols(symbols)
{
	dieStack.emplace_back(dwarf, cu, SharedPtr<DwarfSubprogramInfo>());
}

bool
DwarfDieStack::Advance(TargetAddr offset)
{
	if (peekState.Contains(offset)) {
		PushPeekState();
		PeekInline(offset);
	}

	if (peekState && peekState.Preceeds(offset)) {
		peekState.Reset();
		PeekInline(offset);
	}

	while(1) {
		DwarfStackState &top = dieStack.back();
		bool advanced = top.Advance(offset);

		if (!top) {
			dieStack.pop_back();
			if (dieStack.empty())
				return false;
			continue;
		}

		if (advanced)
			PeekInline(offset);

		return true;
	}
}

void
DwarfDieStack::PeekInline(TargetAddr offset)
{
	assert (!peekState);

	while(1) {
		Dwarf_Die die = dieStack.back().GetLeafDie();

		peekState = DwarfStackState(dwarf, die, GetSharedInfo(die));
		while (peekState && Skippable(peekState.GetLeafDie()))
			peekState.Skip();

		if (!peekState || !peekState.Contains(offset))
			break;
		PushPeekState();
	}
}

bool
DwarfDieStack::Skippable(Dwarf_Die die)
{
	Dwarf_Half tag;

	tag = GetDieTag(die);

	return (tag != DW_TAG_subprogram) &&
	    (tag != DW_TAG_inlined_subroutine);
}

void
DwarfDieStack::PushPeekState()
{
	dieStack.push_back(std::move(peekState));
	/*fprintf(stderr, "Pushed back for die %lx; funcInfo=%p\n",
	    GetDieOffset(dieStack.back().GetLeafDie()),
	    dieStack.back().GetFuncInfo().get());*/
	assert(!peekState);
}

SharedPtr<DwarfSubprogramInfo>
DwarfDieStack::GetSharedInfo(Dwarf_Die die) const
{
	if (GetDieTag(die) != DW_TAG_subprogram)
		return dieStack.back().GetFuncInfo();

	return SharedPtr<DwarfSubprogramInfo>::make(dwarf, die);
}

bool
DwarfDieStack::AdvanceAndMap(Callframe& frame, SharedString leafFile, int leafLine)
{
	bool found = Advance(frame.getOffset());
	if (found) {
		bool mapped = Map(frame, leafFile, leafLine);
		if (mapped)
			return true;
	}

	SharedString func("[unknown_function]");
	auto it = elfSymbols.find(frame.getOffset());
	if (it != elfSymbols.end())
		func = it->second;
	frame.addFrame(leafFile, func, func,
	    leafLine, leafLine);
	return true;
}

bool
DwarfDieStack::Map(Callframe& frame, SharedString leafFile, int leafLine)
{
	SharedString nextFile = leafFile;
	int nextLine = leafLine;
	bool mapped = false;

// 	size_t i = 0;
	for (auto rit = dieStack.rbegin(); rit != dieStack.rend(); ++rit) {
// 		i++;
// 		fprintf(stderr, "iteration %zd/%zd\n", i, dieStack.size());
		Dwarf_Die die = rit->GetLeafDie();
		Dwarf_Half tag = GetDieTag(die);

		switch (tag) {
		case DW_TAG_subprogram: {
				DwarfSubprogramInfo info(dwarf, die);
				/*fprintf(stderr, "Map subprogram die %lx funcInfo=%p\n",
				    GetDieOffset(die), rit->GetFuncInfo().get());*/
				frame.addFrame(nextFile, info.GetFunc(), info.GetDemangled(),
				    nextLine, info.GetLine());
				mapped = true;
				goto out;
			}
		case DW_TAG_inlined_subroutine: {
				DwarfSubprogramInfo info(dwarf, die);
				frame.addFrame(nextFile, info.GetFunc(), info.GetDemangled(),
				     nextLine, rit->GetFuncLine());

				nextFile = GetCallFile(die);
				nextLine = GetCallLine(die);
				mapped = true;
				break;
			}
		}
	}

out:
	return mapped;
}

SharedString
DwarfDieStack::GetCallFile(Dwarf_Die die)
{
	int error;
	Dwarf_Error derr;
	Dwarf_Unsigned fileno, fileindex;
	Dwarf_Signed numfiles;
	char **filenames;

	error = dwarf_attrval_unsigned(die, DW_AT_call_file, &fileno, &derr);
	if (error != 0)
		return imageFile;

	error = dwarf_srcfiles(cuDie, &filenames, &numfiles, &derr);
	if (error != DW_DLV_OK)
		return imageFile;

	/* filenames is indexed from 0 but fileno starts at 1, so subtract 1. */
	fileindex = fileno - 1;
	if (fileindex >= (Dwarf_Unsigned)numfiles)
		return imageFile;

	return filenames[fileindex];
}

int
DwarfDieStack::GetCallLine(Dwarf_Die die)
{
	Dwarf_Unsigned lineno;
	Dwarf_Error derr;
	int error;

	error = dwarf_attrval_unsigned(die, DW_AT_call_line, &lineno, &derr);
	if (error != DW_DLV_OK)
		return -1;

	return lineno;
}
