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

#ifndef DWARFCOMPILEUNITPARAMS_H
#define DWARFCOMPILEUNITPARAMS_H

#include <libdwarf.h>

class DwarfCompileUnit;

class DwarfCompileUnitParams
{
private:
	Dwarf_Unsigned cu_length;
	Dwarf_Half cu_version;
	Dwarf_Off cu_abbrev_offset;
	Dwarf_Half cu_pointer_size;
	Dwarf_Half cu_offset_size;
	Dwarf_Half cu_extension_size;
	Dwarf_Sig8 type_signature;
	Dwarf_Unsigned type_offset;
	Dwarf_Unsigned cu_next_offset;

	friend class DwarfCompileUnit;

public:

	Dwarf_Half GetDwarfVersion() const
	{
		return cu_version;
	}

	Dwarf_Half GetOffsetSize() const
	{
		return cu_offset_size;
	}
};

#endif
