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
#include "MapUtil.h"

#include <cassert>
#include <dwarf.h>

DwarfDieStack::DwarfDieStack(SharedString imageFile, Dwarf_Debug dwarf, Dwarf_Die cu,
    const SymbolMap & symbols)
  : imageFile(imageFile),
    dwarf(dwarf),
    cuBaseAddr(GetBaseAddr(cu)),
    peekState(dwarf, cuBaseAddr),
    cuDie(cu),
    elfSymbols(symbols)//,
//     subprogramState(NULL)
{

	dieStack.emplace_back(dwarf, cu, SharedPtr<DwarfSubprogramInfo>(), cuBaseAddr);
}

TargetAddr
DwarfDieStack::GetBaseAddr(Dwarf_Die cu)
{
	Dwarf_Unsigned addr;
	Dwarf_Error derr;
	int error;

	error = dwarf_attrval_unsigned(cu, DW_AT_low_pc, &addr, &derr);
	if (error != DW_DLV_OK)
		return 0;

	return addr;
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

		peekState = DwarfStackState(dwarf, die, GetSharedInfo(die), cuBaseAddr);
		LOG("peek at %lx (tag %d)\n",
		    peekState ? GetDieOffset(peekState.GetLeafDie()) : 0,
		    peekState ? GetDieTag(peekState.GetLeafDie()) : 0);
		while (peekState && Skippable(offset)) {
			peekState.Skip();
			LOG("peek at %lx (tag %d)\n",
			peekState ? GetDieOffset(peekState.GetLeafDie()) : 0,
			peekState ? GetDieTag(peekState.GetLeafDie()) : 0);
		}

		if (!peekState || !peekState.Contains(offset))
			break;
		PushPeekState();
	}
}

bool
DwarfDieStack::Skippable(TargetAddr offset)
{
	return !peekState.HasRanges() || peekState.Preceeds(offset);
}

void
DwarfDieStack::PushPeekState()
{
	dieStack.push_back(std::move(peekState));
// 	LOG("Pushed back for die %lx; funcInfo=%p\n",
// 	    GetDieOffset(dieStack.back().GetLeafDie()),
// 	    dieStack.back().GetFuncInfo().get());
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
				    nextLine, info.GetLine(), GetDieOffset(die));
				mapped = true;
				goto out;
			}
		case DW_TAG_inlined_subroutine: {
				DwarfSubprogramInfo info(dwarf, die);
				frame.addFrame(nextFile, info.GetFunc(), info.GetDemangled(),
				     nextLine, rit->GetFuncLine(), GetDieOffset(die));

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

bool
DwarfDieStack::AdvanceToSubprogram(Callframe& frame)
{
	TargetAddr offset = frame.getOffset();

	while (1) {
		while (!dieStack.empty() && !dieStack.back()) {
			dieStack.pop_back();
			if (dieStack.empty())
				break;
			dieStack.back().Skip();
		}

		if (dieStack.empty())
			return false;

		DwarfStackState &state = dieStack.back();

		if (state.HasRanges() && state.Succeeds(offset))
			return false;

		Dwarf_Die die = state.GetLeafDie();

		Dwarf_Half tag = GetDieTag(die);
		if (tag == DW_TAG_namespace) {
			dieStack.emplace_back(dwarf, die,
			    GetSharedInfo(die), cuBaseAddr);
			continue;
		} else if (tag == DW_TAG_subprogram) {
			if (state.Contains(offset))
				return true;
		}

		state.Skip();
	}
}

void
DwarfDieStack::AddSubprogramSymbol(DwarfLocationList &list)
{
	Dwarf_Die die = dieStack.back().GetLeafDie();

	assert (GetDieTag(die) == DW_TAG_subprogram);

	DwarfSubprogramInfo info(dwarf, die);

	for (const auto & range : dieStack.back().GetRanges()) {
// 		LOG("Add subprogram covering %lx-%lx\n", range.low, range.high);
		list.insert(std::make_pair(range.low, SharedPtr<DwarfLocation>::make(
		    range.low, range.high, info.GetFunc(), info.GetLine())));
	}
}

void
DwarfDieStack::AddInlineSymbol(DwarfLocationList &list, Dwarf_Die die)
{
	DwarfSubprogramInfo info(dwarf, die);

	for (const auto & range : dieStack.back().GetRanges()) {
// 		LOG("Add inline %lx covering %lx-%lx\n", GetDieOffset(die), range.low, range.high);
		AddDwarfSymbol(list, range.low, range.high,
		    GetCallFile(die), GetCallLine(die), info.GetFunc(),
		    GetDieOffset(die));
	}
}

void
DwarfDieStack::FillSubprogramSymbols(DwarfLocationList& list)
{
	AddSubprogramSymbol(list);

	size_t stackPos = dieStack.size();
	Dwarf_Die subprogram = dieStack.back().GetLeafDie();
	dieStack.emplace_back(dwarf, subprogram, GetSharedInfo(subprogram), cuBaseAddr);

	LOG("*** Start scan for inlines in %lx\n", GetDieOffset(subprogram));
	while (1) {
		while (dieStack.size() > stackPos && !dieStack.back()) {
			dieStack.pop_back();
			if (dieStack.size() == stackPos)
				break;
			dieStack.back().Skip();
		}

		if (dieStack.size() == stackPos)
			break;

		Dwarf_Die die = dieStack.back().GetLeafDie();
		Dwarf_Half tag = GetDieTag(die);

		switch (tag) {
			case DW_TAG_inlined_subroutine:
				AddInlineSymbol(list, die);
				break;
		}

		if (dieStack.back().HasRanges()) {
			dieStack.emplace_back(dwarf, die, GetSharedInfo(die), cuBaseAddr);
			continue;
		}

		dieStack.back().Skip();
	}

	assert(subprogram == dieStack.back().GetLeafDie());
}

bool
DwarfDieStack::SubprogramContains(TargetAddr addr) const
{
	return dieStack.back().Contains(addr);
}


bool
DwarfDieStack::SubprogramSucceeds(TargetAddr addr) const
{
	return dieStack.back().Succeeds(addr);
}
