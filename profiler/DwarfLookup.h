/*-
 * Copyright (c) 2009 Kai Wang
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef DWARF_LOOKUP_H
#define DWARF_LOOKUP_H

#include <dwarf.h>
#include <libdwarf.h>
#include <libelf.h>

#include <map>
#include <string>

class DwarfLocation;

class DwarfLookup
{
private:
	typedef std::map<uintptr_t, DwarfLocation *> LocationMap;

	std::string m_image_file;
	LocationMap m_functions;
	LocationMap m_locations;
	uint64_t m_text_start;
	uint64_t m_text_end;

	void FindTextRange(Elf *);

	void FillFunctionMap(Elf *);
	void FillFunctionsFromDie(Dwarf_Debug, Dwarf_Die);
	void AddFunction(Dwarf_Debug, Dwarf_Die);
	std::string GetFuncNameFromAttr(Dwarf_Attribute attr);
	std::string GetFuncNameFromSpec(Dwarf_Debug dwarf, Dwarf_Die die);
	bool GetFileNameFromAttr(Dwarf_Debug dwarf, Dwarf_Die die,
	    std::string &file);
	bool GetLineNumberFromAttr(Dwarf_Debug dwarf, Dwarf_Die die,
	    int &lineno);
	
	void FillLocationMap(Dwarf_Debug);
	void FillLocationsFromDie(Dwarf_Debug, Dwarf_Die);
	void AddLocations(Dwarf_Debug, Dwarf_Die);
	
	bool Lookup(uintptr_t addr, const LocationMap &map,
	    std::string &fileStr, std::string &funcStr, int &line) const;

public:
	DwarfLookup(const std::string &file);
	~DwarfLookup();

	bool LookupLine(uintptr_t addr, std::string &file, std::string &func,
            int &line) const;
	bool LookupFunc(uintptr_t addr, std::string &file, std::string &func,
            int &line) const;
	    
	bool isContained(uintptr_t addr) const;
	bool isOk() const;
};

#endif
