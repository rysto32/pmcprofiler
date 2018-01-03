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

#include "DwarfDieRanges.h"

#include "DwarfCompileUnit.h"
#include "DwarfRangeList.h"
#include "DwarfUtil.h"

#include <dwarf.h>

DwarfDieRanges::DwarfDieRanges(Dwarf_Debug dwarf, const DwarfCompileUnit & cu)
  : dwarf(dwarf), compileUnit(cu)
{
}

DwarfDieRanges::DwarfDieRanges(Dwarf_Debug dwarf, Dwarf_Die die,
    const DwarfCompileUnit & cu)
  : dwarf(dwarf), compileUnit(cu)
{
	Reinit(die);
}

void
DwarfDieRanges::Reinit(Dwarf_Die die)
{
	Dwarf_Error derr;
	Dwarf_Unsigned low_pc, high_pc, off;
	int error;

	Reset();
	this->die = die;

	error = dwarf_attrval_unsigned(die, DW_AT_ranges, &off, &derr);
	if (error == DW_DLV_OK) {
		InitFromRanges(die, off);
		return;
	}

	error = dwarf_attrval_unsigned(die, DW_AT_low_pc, &low_pc, &derr);
	if (error != DW_DLV_OK)
		return;

	error = GetHighPc(die, low_pc, high_pc);
	if (error == DW_DLV_OK) {
		AddRange(low_pc, high_pc);
	}
}

int
DwarfDieRanges::GetHighPc(Dwarf_Die die, Dwarf_Unsigned low_pc, Dwarf_Unsigned &high_pc)
{
	int error;
	Dwarf_Attribute attr;
	Dwarf_Unsigned high;
	Dwarf_Half form;
	Dwarf_Error derr;
	Dwarf_Form_Class cls;

	error = dwarf_attr(die, DW_AT_high_pc, &attr, &derr);
	if (error != 0)
		return error;

	error = dwarf_attrval_unsigned(die, DW_AT_high_pc, &high, &derr);
	if (error != DW_DLV_OK)
		return error;

	error = dwarf_whatform(attr, &form, &derr);
	if (error != 0)
		return error;

	cls = dwarf_get_form_class(compileUnit.GetDwarfVersion(), DW_AT_high_pc,
	    compileUnit.GetOffsetSize(), form);
	switch (cls) {
	case DW_FORM_CLASS_ADDRESS:
		high_pc = high;
		return 0;
	case DW_FORM_CLASS_CONSTANT:
		high_pc = low_pc + high;
		return 0;
	default:
		return DW_DLV_ERROR;
	}
}


void
DwarfDieRanges::Reset()
{
	ranges.clear();
}

void
DwarfDieRanges::AddRange(TargetAddr low, TargetAddr high)
{
	ranges.emplace_back(low, high);
}

void
DwarfDieRanges::InitFromRanges(Dwarf_Die die, Dwarf_Unsigned rangeOff)
{
	TargetAddr baseAddr, low, high;

	DwarfRangeList rangeList(dwarf, rangeOff);

	baseAddr = compileUnit.GetBaseAddr();

	for (const auto & range : rangeList) {
		switch (range.dwr_type) {
		case DW_RANGES_ENTRY:
			low = baseAddr + range.dwr_addr1;
			high = baseAddr + range.dwr_addr2;
			AddRange(low, high);
			break;
		case DW_RANGES_ADDRESS_SELECTION:
			baseAddr = range.dwr_addr2;
			break;
		case DW_RANGES_END:
			return;
		}
	}

	// DWARF does not guaranteed that the ranges are ordered, so make sure
	// that they are here.
	std::sort(ranges.begin(), ranges.end());
}

bool
DwarfDieRanges::Contains(TargetAddr a) const
{
	for (const auto & range : ranges) {
		if (range.Contains(a)) {
			return true;
		}
	}

	return false;
}

bool
DwarfDieRanges::Preceeds(TargetAddr a) const
{
	return ranges.back().high < a;
}

bool
DwarfDieRanges::Succeeds(TargetAddr a) const
{
	return ranges.front().low > a;
}
