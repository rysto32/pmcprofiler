// Copyright (c) 2014-2015 Sandvine Incorporated.  All rights reserved.
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

#ifndef DWARF_LOOKUP_H
#define DWARF_LOOKUP_H

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

class DwarfCompileUnit;

class DwarfLookup
{
private:
	typedef std::map<uintptr_t, DwarfCompileUnit*> CompileUnitMap;
	typedef std::vector<DwarfCompileUnit*> CompileUnitList;

	Elf *m_elf;
	Dwarf_Debug m_dwarf;

	CompileUnitMap m_compile_units;
	RangeMap m_functions;

	CompileUnitList m_cu_list;
	RangeList m_ranges;
	DwarfLocationList m_locationList;
	
	std::string m_image_file;
	std::string m_symbols_file;
	uint64_t m_text_start;
	uint64_t m_text_end;

	Elf * GetSymbolFile(Elf *);
	int FindSymbolFile();
	void ParseElfFile(Elf *);
	void ParseDebuglink(Elf_Scn *);
	void FillFunctionsFromSymtab(Elf *, Elf_Scn *, GElf_Shdr *);
	void AddFunction(GElf_Addr, const std::string &);
	
	void EnumerateCompileUnits(Dwarf_Debug dwarf);
	void ProcessCompileUnit(Dwarf_Debug dwarf, Dwarf_Die die);
	void AddCompileUnit(Dwarf_Debug dwarf, Dwarf_Die die);
	void AddCU_PC(Dwarf_Debug dwarf, Dwarf_Die die, DwarfCompileUnit *cu);
	void AddCU_Ranges(Dwarf_Debug dwarf, Dwarf_Die die,
	    DwarfCompileUnit *cu, Dwarf_Unsigned off);

public:
	DwarfLookup(const std::string &file);
	~DwarfLookup();

	bool LookupLine(uintptr_t addr, size_t inelineDepth, std::string &file,
		std::string &func, u_int &line) const;
	bool LookupFunc(uintptr_t addr, std::string &file, std::string &func,
		u_int &line) const;
	size_t GetInlineDepth(uintptr_t addr) const;

	const std::string & getImageFile() const;
	bool isContained(uintptr_t addr) const;
};

#endif
