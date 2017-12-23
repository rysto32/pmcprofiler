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

#include "DwarfRangeList.h"

#include <dwarf.h>

DwarfDieRanges::DwarfDieRanges(Dwarf_Debug dwarf)
  : dwarf(dwarf)
{
}

void
DwarfDieRanges::Reinit(Dwarf_Die die)
{
	Dwarf_Error derr;
	Dwarf_Unsigned low_pc, high_pc, off;
	int error, err_lo, err_hi;

	Reset();

	error = dwarf_attrval_unsigned(die, DW_AT_ranges, &off, &derr);
	if (error == DW_DLV_OK) {
		InitFromRanges(die, off);
		return;
	}

	err_lo = dwarf_attrval_unsigned(die, DW_AT_low_pc, &low_pc, &derr);
	err_hi = dwarf_attrval_unsigned(die, DW_AT_high_pc, &high_pc, &derr);
	if (err_lo == DW_DLV_OK && err_hi == DW_DLV_OK)
		AddRange(low_pc, high_pc);
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
	Dwarf_Unsigned baseAddr, low, high;
	int error;
	Dwarf_Error derr;

	DwarfRangeList rangeList(dwarf, rangeOff);

	error = dwarf_attrval_unsigned(die, DW_AT_low_pc, &baseAddr, &derr);
	if (error == 0)
		baseAddr = 0;

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
}

bool
DwarfDieRanges::Contains(TargetAddr a) const
{
	for (const auto & range : ranges) {
		if (range.Contains(a))
			return true;
	}

	return false;
}

bool
DwarfDieRanges::Preceeds(TargetAddr a) const
{
	return ranges.back().high < a;
}
