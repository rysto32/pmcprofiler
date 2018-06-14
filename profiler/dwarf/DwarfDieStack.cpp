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
#include "DwarfLocList.h"
#include "DwarfRangeLookup.h"
#include "DwarfException.h"
#include "DwarfStackState.h"
#include "DwarfUtil.h"
#include "MapUtil.h"
#include "VariableLookup.h"

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

// 			LOG("Take %lx from state\n", GetDieOffset(**diePtr));
			for (const auto & range : state.GetRanges()) {
// 				LOG("Insert %lx at %lx-%lx\n",
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
DwarfDieStack::FillSubprogramSymbols(DwarfLocationList& list,
    const DwarfDieRanges &ranges)
{
	AddSubprogramSymbol(list, ranges);

	size_t stackPos = dieStack.size();

// 	LOG("*** Start scan for inlines in %lx\n", GetDieOffset(topDie));
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

void
DwarfDieStack::FillSubprogramVars(VariableLookup &vars)
{
	size_t stackPos = dieStack.size();

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
		case DW_TAG_formal_parameter:
		case DW_TAG_variable:
			try {
				AddVariable(vars, die);
			} catch (const DwarfException &) {
				// Ignore the error and move onto the next DIE
			}
			break;
		}

		dieStack.emplace_back(dwarf, die, cu);
	}

}

void
DwarfDieStack::AddVariable(VariableLookup &vars, Dwarf_Die def)
{
	Dwarf_Attribute attr;
	Dwarf_Error derr;
	int error;

	DwarfDieOffset off = GetTypeOff(def);

	error = dwarf_attr(def, DW_AT_location, &attr, &derr);
	if (error != 0)
		return;

	DwarfLocList list(dwarf, attr);

	for (Dwarf_Locdesc * desc : list) {
		if (desc->ld_lopc != desc->ld_hipc) {
			// Only check the first location description.  If the operator
			// is not a DW_OP_regX we don't need to worry about it here.
			Dwarf_Loc *lr = &desc->ld_s[0];

			if (lr->lr_atom >= DW_OP_reg0 && lr->lr_atom <= DW_OP_reg31) {
				vars.AddVariable(lr->lr_atom, desc->ld_lopc, desc->ld_hipc, off);
			}
		}
	}
}

DwarfDieOffset
DwarfDieStack::GetTypeOff(Dwarf_Die die)
{
	Dwarf_Attribute attr;
	Dwarf_Error derr;

	int error = dwarf_attr(die, DW_AT_type, &attr, &derr);
	if (error != 0) {
		error = dwarf_attr(die, DW_AT_abstract_origin, &attr, &derr);
		if (error != 0) {
			throw DwarfException("No type associated with variable");
		}

		Dwarf_Off originOff;
		error = dwarf_global_formref(attr, &originOff, &derr);
		if (error != 0) {
			throw DwarfException("Can't formref");
		}

		DwarfDie originDie = DwarfDie::OffDie(dwarf, originOff);
		if (!originDie)
			throw DwarfException("Could not get origin die");

		error = dwarf_attr(*originDie, DW_AT_type, &attr, &derr);
		if (error != 0)
			throw DwarfException("No type attribute");
	}

	Dwarf_Off subtypeOff;
	error = dwarf_global_formref(attr, &subtypeOff, &derr);
	if (error != 0)
		throw DwarfException("dwarf_global_formref failed");

	return subtypeOff;
}
