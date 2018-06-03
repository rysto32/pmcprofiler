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

#ifndef DWARFRANGELIST_H
#define DWARFRANGELIST_H

#include <libdwarf.h>

#include "DwarfArray.h"
#include "DwarfException.h"

struct DwarfRangeDeleter
{
	void operator()(Dwarf_Debug dwarf, Dwarf_Ranges *ranges, Dwarf_Signed count)
	{
		dwarf_ranges_dealloc(dwarf, ranges, count);
	}
};

class DwarfRangeList : public DwarfArray<Dwarf_Ranges, DwarfRangeDeleter>
{
	typedef DwarfArray<Dwarf_Ranges, DwarfRangeDeleter> Super;

	static Super::Array GetArray(Dwarf_Debug dwarf, Dwarf_Unsigned off)
	{
		Dwarf_Error derr;
		Dwarf_Ranges *array;
		Dwarf_Signed count;
		Dwarf_Unsigned size;
		int error;

		error = dwarf_get_ranges(dwarf, off, &array, &count, &size, &derr);
		if (error != DW_DLV_OK)
			throw DwarfException("dwarf_get_ranges failed");

		return Super::Array(array, count);
	}

public:
	DwarfRangeList(Dwarf_Debug dwarf, Dwarf_Unsigned off)
	  : DwarfArray(dwarf, GetArray(dwarf, off))
	{
	}
};

#endif
