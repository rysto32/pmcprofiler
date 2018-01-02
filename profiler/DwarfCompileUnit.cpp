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

#include "DwarfCompileUnit.h"

#include "DwarfException.h"

#include <dwarf.h>

DwarfCompileUnit::DwarfCompileUnit(Dwarf_Debug dwarf, Dwarf_Bool is_info)
  : dwarf(dwarf), is_info(is_info)
{
	AdvanceToSibling();
}

DwarfCompileUnit
DwarfCompileUnit::GetFirstCU(Dwarf_Debug dwarf, Dwarf_Bool is_info)
{
	return DwarfCompileUnit(dwarf, is_info);
}

void
DwarfCompileUnit::AdvanceToSibling()
{
	int error = dwarf_next_cu_header_c(dwarf, is_info, &cu_length,
	    &cu_version, &cu_abbrev_offset, &cu_pointer_size, &cu_offset_size,
	    &cu_extension_size, &type_signature, &type_offset, &cu_next_offset,
	    NULL);

	if (error == DW_DLV_ERROR) {
		throw DwarfException("dwarf_next_cu_header_c failed");
	} else if (error == DW_DLV_NO_ENTRY) {
		dwarf = nullptr;
		baseAddr = 0;
	} else {
		die = DwarfDie::GetCuDie(dwarf);
		InitBaseAddr();
	}
}

void
DwarfCompileUnit::AdvanceDie()
{
	die.AdvanceToSibling();
}

void
DwarfCompileUnit::InitBaseAddr()
{
	Dwarf_Unsigned addr;
	Dwarf_Error derr;
	int error;

	error = dwarf_attrval_unsigned(*die, DW_AT_low_pc, &addr, &derr);
	if (error != DW_DLV_OK)
		baseAddr = 0;
	else
		baseAddr = addr;
}

