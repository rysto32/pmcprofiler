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
#include "DwarfCompileUnitDie.h"
#include "DwarfRangeLookup.h"
#include "DwarfException.h"
#include "DwarfStackState.h"
#include "DwarfUtil.h"
#include "MapUtil.h"

#include <cassert>
#include <dwarf.h>

DwarfDieStack::DwarfDieStack(SharedString imageFile, Dwarf_Debug dwarf,
    const DwarfCompileUnitDie &cu, Dwarf_Die die)
  : imageFile(imageFile),
    dwarf(dwarf),
    topDie(die),
    cu(cu)
{
	dieStack.emplace_back(dwarf, die, cu);
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

void
DwarfDieStack::EnumerateSubprograms(DwarfRangeLookup<DwarfDie> &map)
{
	while (1) {
		while (!dieStack.empty() && !dieStack.back()) {
			dieStack.pop_back();
			if (dieStack.empty())
				break;
			dieStack.back().Skip();
		}

		if (dieStack.empty())
			return;

		DwarfStackState &state = dieStack.back();
		Dwarf_Die die = state.GetLeafDie();

		Dwarf_Half tag = GetDieTag(die);
		if (tag == DW_TAG_namespace) {
			dieStack.emplace_back(dwarf, die, cu);
			continue;
		} else if (tag == DW_TAG_subprogram && state.HasRanges()) {
			SharedPtr<DwarfDie> diePtr =
			    SharedPtr<DwarfDie>::make(state.TakeLeafDie());

// 			LOG("Take %lx from state", GetDieOffset(**diePtr));
			for (const auto & range : state.GetRanges()) {
// 				LOG("Insert %lx at %lx-%lx",
// 				    GetDieOffset(**diePtr), range.low, range.high);
				map.insert(range.low, range.high, diePtr);
			}
		}

		state.Skip();
	}
}

void
DwarfDieStack::AddSubprogramSymbol(DwarfLocationList &list, const DwarfDieRanges &ranges)
{
	Dwarf_Die die = topDie;

	assert (GetDieTag(die) == DW_TAG_subprogram);

	DwarfSubprogramInfo info(dwarf, die);

	for (const auto & range : ranges) {
//  		LOG("Add subprogram covering %lx-%lx", range.low, range.high);
		list.insert(std::make_pair(range.low, SharedPtr<DwarfLocation>::make(
		    range.low, range.high, info.GetFunc(), info.GetLine())));
	}
}

void
DwarfDieStack::AddInlineSymbol(DwarfLocationList &list, Dwarf_Die die)
{
	DwarfSubprogramInfo info(dwarf, die);

	for (const auto & range : dieStack.back().GetRanges()) {
//  		LOG("Add inline %lx covering %lx-%lx", GetDieOffset(die), range.low, range.high);
		AddDwarfSymbol(list, range.low, range.high,
		    GetCallFile(die), GetCallLine(die), info.GetFunc(),
		    GetDieOffset(die));
	}
}

void
DwarfDieStack::FillSubprogramSymbols(DwarfLocationList& list,
    const DwarfDieRanges &ranges)
{
	AddSubprogramSymbol(list, ranges);

	size_t stackPos = dieStack.size();

// 	LOG("*** Start scan for inlines in %lx", GetDieOffset(topDie));
	while (1) {
		while (dieStack.size() >= stackPos && !dieStack.back()) {
			dieStack.pop_back();
			if (dieStack.size() < stackPos)
				break;
			dieStack.back().Skip();
		}

		if (dieStack.size() < stackPos)
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
}

