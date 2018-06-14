// Copyright (c) 2018 Ryan Stone.
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

#include "DwarfLocList.h"

#include "DwarfException.h"

DwarfLocList::DwarfLocList(Dwarf_Debug dwarf, Dwarf_Attribute attr)
  : dwarf(dwarf)
{
	Dwarf_Error derr;

	int error = dwarf_loclist_n(attr, &buf, &count, &derr);
	if (error != 0)
		throw DwarfException("dwarf_loclist_n failed");
}

DwarfLocList::~DwarfLocList()
{
	Dwarf_Signed i;

	for (i = 0; i < count; ++i) {
		dwarf_dealloc(dwarf, buf[i]->ld_s, DW_DLA_LOC_BLOCK);
		dwarf_dealloc(dwarf, buf[i], DW_DLA_LOCDESC);
	}

	dwarf_dealloc(dwarf, buf, DW_DLA_LIST);
}
