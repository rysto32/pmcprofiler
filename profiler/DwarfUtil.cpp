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

#include "DwarfUtil.h"

#include "DwarfException.h"

#include <dwarf.h>

DwarfDieOffset GetDieOffset(Dwarf_Die die)
{
	Dwarf_Error derr;
	Dwarf_Off offset;
	int error;

	error = dwarf_dieoffset(die, &offset, &derr);
	if (error != 0)
		throw DwarfException("dwarf_dieoffset failed");
	return (offset);
}

Dwarf_Half GetDieTag(Dwarf_Die die)
{
	Dwarf_Error derr;
	Dwarf_Half tag;
	int error;

	error = dwarf_tag(die, &tag, &derr);
	if (error != 0)
		throw DwarfException("dwarf_tag failed");
	return (tag);
}

#ifdef GNU_LIBDWARF
int
dwarf_attrval_string(Dwarf_Die die, Dwarf_Half tag, const char **str, Dwarf_Error *derr)
{
	Dwarf_Attribute attr;
	int error = dwarf_attr(die, tag, &attr, derr);
	if (error != 0)
		return (error);

	return dwarf_formstring(attr, const_cast<char**>(str), derr);

}

int
dwarf_attrval_unsigned(Dwarf_Die die, Dwarf_Half tag, Dwarf_Unsigned *val, Dwarf_Error *derr)
{
	Dwarf_Attribute attr;
	Dwarf_Half form;
	Dwarf_Addr addr;
	Dwarf_Off off;
	int error;

	error = dwarf_attr(die, tag, &attr, derr);
	if (error != 0)
		return (error);

	error = dwarf_whatform(attr, &form, derr);
	if (error != 0)
		return error;

	switch (form) {
	case DW_FORM_ref_addr:
	case DW_FORM_ref_udata:
	case DW_FORM_ref1:
	case DW_FORM_ref2:
	case DW_FORM_ref4:
	case DW_FORM_ref8:
	case DW_FORM_sec_offset:
		error = dwarf_global_formref(attr, &off, derr);
		if (error != 0)
			return error;
		*val = off;
		return error;

	case DW_FORM_udata:
	case DW_FORM_data1:
	case DW_FORM_data2:
	case DW_FORM_data4:
	case DW_FORM_data8:
		return dwarf_formudata(attr, val, derr);

	case DW_FORM_addr:
		error = dwarf_formaddr(attr, &addr, derr);
		if (error != 0)
			return error;
		*val = addr;
		return error;

	default:
		return DW_DLV_ERROR;
	}
}
#endif
