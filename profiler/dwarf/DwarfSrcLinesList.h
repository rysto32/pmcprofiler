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

#ifndef DWARFSRCLINESLIST_H
#define DWARFSRCLINESLIST_H

#include "DwarfArray.h"
#include "DwarfException.h"
#include "DwarfUtil.h"

#include <format>

struct DwarfSrcLinesDeleter
{
	void operator()(Dwarf_Debug dwarf, Dwarf_Line *lines, Dwarf_Signed count)
	{
		dwarf_srclines_dealloc(dwarf, lines, count);
	}
};

class DwarfSrcLinesList : public DwarfArray<Dwarf_Line, DwarfSrcLinesDeleter>
{
	typedef DwarfArray<Dwarf_Line, DwarfSrcLinesDeleter> Super;

	static Super::Array GetArray(Dwarf_Die die)
	{
		Dwarf_Error derr;
		Dwarf_Line *array;
		Dwarf_Signed count;
		int error;

		error = dwarf_srclines(die, &array, &count, &derr);
		if (error != DW_DLV_OK) {
			throw DwarfException(std::format("dwarf_srclines failed on {}: {}", GetDieOffset(die), dwarf_errmsg(derr)));
		}

		return Super::Array(array, count);
	}

public:
	DwarfSrcLinesList(Dwarf_Debug dwarf, Dwarf_Die die)
	  : DwarfArray(dwarf, GetArray(die))
	{
	}
};

#endif
