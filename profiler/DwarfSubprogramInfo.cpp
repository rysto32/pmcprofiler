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

#include "DwarfSubprogramInfo.h"
#include "DwarfDie.h"
#include "Image.h"

void
DwarfSubprogramInfo::CheckInitialized()
{
	Dwarf_Attribute attr;
	Dwarf_Error derr;
	int error;

	if (inited)
		return;

	error = dwarf_attr(die, DW_AT_abstract_origin, &attr, &derr);
	if (error == 0)
		InitFromAbstractOrigin(attr);
	else
		InitFromSpecification(die);
}

void
DwarfSubprogramInfo::InitFromAbstractOrigin(Dwarf_Attribute attr)
{
	Dwarf_Off ref;
	Dwarf_Error derr;
	int error;

	error = dwarf_global_formref(attr, &ref, &derr);
	if (error != 0) {
		InitFromLocalAttr(die);
		return;
	}

	DwarfDie originDie(DwarfDie::OffDie(dwarf, ref));
	if (originDie)
		InitFromSpecification(*originDie);
	else
		InitFromLocalAttr(die);
}

void
DwarfSubprogramInfo::InitFromSpecification(Dwarf_Die srcDie)
{
	Dwarf_Attribute attr;
	Dwarf_Off ref;
	Dwarf_Error derr;
	int error;

	error = dwarf_attr(srcDie, DW_AT_specification, &attr, &derr);
	if (error != 0) {
		InitFromLocalAttr(srcDie);
		return;
	}

	error = dwarf_global_formref(attr, &ref, &derr);
	if (error != 0) {
		InitFromLocalAttr(srcDie);
		return;
	}

	DwarfDie specDie(DwarfDie::OffDie(dwarf, ref));
	if (specDie)
		InitFromLocalAttr(*specDie);
	else
		InitFromLocalAttr(srcDie);
}

void
DwarfSubprogramInfo::InitFromLocalAttr(Dwarf_Die srcDie)
{
	Dwarf_Error derr;
	const char *func;
	Dwarf_Unsigned lineno;
	int error;

	func = NULL;
	lineno = -1;

	// If this fails we have no fallback
	dwarf_attrval_unsigned(srcDie, DW_AT_decl_line, &lineno, &derr);

	error = dwarf_attrval_string(srcDie, DW_AT_MIPS_linkage_name, &func,
	    &derr);
	if (error == DW_DLV_OK) {
		SetFunc(func, lineno);
		return;
	}

	error = dwarf_attrval_string(srcDie, DW_AT_name, &func, &derr);
	if (error == DW_DLV_OK) {
		SetFunc(func, lineno);
		return;
	}

	SetFunc("", lineno);
}

void
DwarfSubprogramInfo::SetFunc(SharedString f, int l)
{
	func = f;
	demangled = Image::demangle(f);
	line = l;
}


SharedString
DwarfSubprogramInfo::GetDemangled()
{
	CheckInitialized();

	return demangled;
}

SharedString
DwarfSubprogramInfo::GetFunc()
{
	CheckInitialized();

	return func;
}

int
DwarfSubprogramInfo::GetLine()
{
	CheckInitialized();

	return line;
}
