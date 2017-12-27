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

#ifndef DWARFSRCLINE_H
#define DWARFSRCLINE_H

#include <libdwarf.h>

#include "DwarfException.h"
#include "SharedString.h"

class DwarfSrcLine
{
private:
	Dwarf_Line line;
	Dwarf_Addr addr;

	static Dwarf_Addr GetAddr(const Dwarf_Line &l)
	{
		Dwarf_Error derr;
		Dwarf_Addr addr;
		int error;

		error = dwarf_lineaddr(l, &addr, &derr);
		if (error != 0)
			throw DwarfException("dwarf_lineaddr failed");

		return addr;
	}

public:
	DwarfSrcLine(const Dwarf_Line & l)
	  : line(l), addr(GetAddr(l))
	{
	}

	DwarfSrcLine(const DwarfSrcLine &) = delete;
	DwarfSrcLine(DwarfSrcLine &&) = delete;

	DwarfSrcLine & operator=(const DwarfSrcLine &) = delete;
	DwarfSrcLine & operator=(DwarfSrcLine && other) = default;

	Dwarf_Addr GetAddr() const
	{
		return addr;
	}

	Dwarf_Unsigned GetLine() const
	{
		Dwarf_Error derr;
		Dwarf_Unsigned lineno;
		int error;

		error = dwarf_lineno(line, &lineno, &derr);
		if (error != 0)
			throw DwarfException("dwarf_lineno failed");

		return lineno;
	}

	SharedString GetFile(SharedString image) const
	{
		Dwarf_Error derr;
		char *file;
		int error;

		error = dwarf_linesrc(line, &file, &derr);
		if (error != 0)
			return image;

		return file;
	}
};

#endif