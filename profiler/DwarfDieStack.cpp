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
#include "DwarfCompileUnit.h"
#include "DwarfException.h"
#include "DwarfStackState.h"
#include "DwarfUtil.h"
#include "MapUtil.h"

#include <cassert>
#include <dwarf.h>

DwarfDieStack::DwarfDieStack(SharedString imageFile, Dwarf_Debug dwarf,
    const DwarfCompileUnit &cu)
  : imageFile(imageFile),
    dwarf(dwarf),
    cu(cu)
{
	dieStack.emplace_back(dwarf, cu.GetDie(), cu);
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

	error = dwarf_srcfiles(cu.GetDie(), &filenames, &numfiles, &derr);
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
			dieStack.emplace_back(dwarf, die, cu);
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
//  		LOG("Add subprogram covering %lx-%lx\n", range.low, range.high);
		list.insert(std::make_pair(range.low, SharedPtr<DwarfLocation>::make(
		    range.low, range.high, info.GetFunc(), info.GetLine())));
	}
}

void
DwarfDieStack::AddInlineSymbol(DwarfLocationList &list, Dwarf_Die die)
{
	DwarfSubprogramInfo info(dwarf, die);

	for (const auto & range : dieStack.back().GetRanges()) {
//  		LOG("Add inline %lx covering %lx-%lx\n", GetDieOffset(die), range.low, range.high);
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
	dieStack.emplace_back(dwarf, subprogram, cu);

// 	LOG("*** Start scan for inlines in %lx\n", GetDieOffset(subprogram));
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
			dieStack.emplace_back(dwarf, die, cu);
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
