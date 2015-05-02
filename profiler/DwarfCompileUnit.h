// Copyright (c) 2015 Sandvine Incorporated.  All rights reserved.
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
//
// $FreeBSD$

#ifndef DWARF_COMPILE_UNIT_H
#define DWARF_COMPILE_UNIT_H

#include <dwarf.h>
#include <libdwarf.h>
#include <libelf.h>
#include <gelf.h>

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "DwarfLocation.h"
#include "DwarfRange.h"

class DwarfCompileUnit
{
private:
	Dwarf_Debug m_dwarf;
	Dwarf_Off m_die_offset;
	bool m_die_processed;
	Dwarf_Unsigned m_base_addr;

	RangeMap m_locations;
	RangeList m_ranges;
	DwarfLocationList m_locationList;
	
	const std::string & m_image_file;
	const RangeMap & m_functions;

	void AddLocations();

	void FillCUInlines(Dwarf_Debug dwarf, Dwarf_Die cu);
	void FillInlineFunctions(Dwarf_Debug, Dwarf_Die, Dwarf_Die);
	void AddInlines(Dwarf_Debug, Dwarf_Die, Dwarf_Die);
	DwarfLocation *UnknownLocation();
	DwarfLocation *GetInlineCaller(Dwarf_Debug, Dwarf_Die, Dwarf_Die);
	std::string GetSubprogramName(Dwarf_Debug dwarf, Dwarf_Die func);
	std::string GetNameAttr(Dwarf_Debug dwarf, Dwarf_Die func);
	std::string SpecSubprogramName(Dwarf_Debug dwarf, Dwarf_Die func_die);
	void AddInlineRanges(Dwarf_Debug, Dwarf_Die , Dwarf_Die, DwarfLocation *);
	void AddInlineLoc(DwarfLocation *, Dwarf_Debug, Dwarf_Die, uintptr_t,
	    uintptr_t);

	void SetInlineCaller(Dwarf_Debug dwarf, Dwarf_Die die);
	void SetLocationFunc(DwarfLocation &loc, const std::string func);
	void SetAssemblyFuncs();
	
	Dwarf_Die GetDie() const;
	void FreeDie(Dwarf_Die die) const;

	/*
	 * Some compilers, including clang, put the mangled name of a symbol
	 * in this non-standard DWARF attribute.
	 */
	static const int DW_AT_MIPS_linkage_name = 0x2007;

public:
	DwarfCompileUnit(Dwarf_Debug dwarf, Dwarf_Die die,
	    const std::string &file, const RangeMap &functions);
	~DwarfCompileUnit();

	static bool Lookup(uintptr_t addr, const RangeMap &map,
	    std::string &fileStr, std::string &funcStr, u_int &line,
	    size_t inlineDepth = 0);

	bool LookupLine(uintptr_t addr, size_t inelineDepth, std::string &file,
		std::string &func, u_int &line);
	bool LookupFunc(uintptr_t addr, std::string &file, std::string &func,
		u_int &line) const;
	size_t GetInlineDepth(uintptr_t addr);

	Dwarf_Unsigned GetCUBaseAddr() const;
	
	static Dwarf_Off GetDieOffset(Dwarf_Die die);
};

#endif
