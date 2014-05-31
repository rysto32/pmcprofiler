// Copyright (c) 2009-2014 Sandvine Incorporated.  All rights reserved.
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

#include "DwarfLookup.h"
#include "DwarfLocation.h"

#include <err.h>
#include <fcntl.h>
#include <gelf.h>

DwarfLookup::DwarfLookup(const std::string &filename)
    : m_image_file(filename), m_text_start(0), m_text_end(0)
{
	Dwarf_Debug dwarf;
	Dwarf_Error derr;
	Elf *elf;
	int fd;

	fd = open(filename.c_str(), O_RDONLY);
	if (fd < 0)
		return;

	elf = elf_begin(fd, ELF_C_READ, NULL);
	if (elf == NULL)
		return;

	FindTextRange(elf);
	FillFunctionMap(elf);

	/* 
	 * It is not fatal if this fails: we'll do out best without debug
	 * symbols.
	 */
	if (dwarf_elf_init(elf, DW_DLC_READ, NULL, NULL, &dwarf, &derr) == 0) {
		FillLocationMap(dwarf);
		dwarf_finish(dwarf, &derr);
	}

	elf_end(elf);
}

DwarfLookup::~DwarfLookup()
{

	for (LocationMap::iterator it = m_functions.begin();
	    it != m_functions.end(); ++it) {
		delete it->second;
	}

	for (LocationMap::iterator it = m_locations.begin();
	    it != m_locations.end(); ++it) {
		delete it->second;
	}
}

void
DwarfLookup::FindTextRange(Elf *elf)
{
	Elf_Scn *section;
	const char *name;
	GElf_Shdr shdr;
	size_t shdrstrndx;

	if (elf_getshdrstrndx(elf, &shdrstrndx) != 0)
		return;

	while ((section = elf_nextscn(elf, section)) != NULL) {
		if (gelf_getshdr(section, &shdr) == NULL)
			return;

		name = elf_strptr(elf, shdrstrndx, shdr.sh_name);
		if (name == NULL)
			return;

		if (strcmp(name, ".text") == 0) {
			m_text_start = shdr.sh_offset;
			m_text_end = shdr.sh_offset + shdr.sh_size;
			return;
		}
	}
	
}

void
DwarfLookup::FillFunctionMap(Elf *elf)
{
	Dwarf_Die die;
	Dwarf_Error derr;
	int error;
	
	while ((error = dwarf_next_cu_header(dwarf, NULL, NULL, NULL, NULL,
	    NULL, &derr)) == DW_DLV_OK) {
		if (dwarf_siblingof(dwarf, NULL, &die, &derr) == DW_DLV_OK)
			FillFunctionsFromDie(dwarf, die);
	}
}

void
DwarfLookup::FillFunctionsFromDie(Dwarf_Debug dwarf, Dwarf_Die die)
{
	Dwarf_Die last_die, child;
	Dwarf_Error derr;
	Dwarf_Half tag;
	int error;

	do {
		if (dwarf_tag(die, &tag, &derr) == DW_DLV_OK) {
			if (tag == DW_TAG_subprogram)
				AddFunction(dwarf, die);
		}

		error = dwarf_child(die, &child, &derr);
		if (error == DW_DLV_OK)
			FillFunctionsFromDie(dwarf, child);

		last_die = die;
		error = dwarf_siblingof(dwarf, last_die, &die, &derr);
		dwarf_dealloc(dwarf, last_die, DW_DLA_DIE);
	} while (error == DW_DLV_OK);
}

void
DwarfLookup::AddFunction(Dwarf_Debug dwarf, Dwarf_Die die)
{
	Dwarf_Die attr_die;
	Dwarf_Attribute attr;
	Dwarf_Unsigned addr;
	Dwarf_Error derr;
	std::string func;
	std::string file;
	int line, error;

	if (dwarf_attrval_unsigned(die, DW_AT_low_pc, &addr, &derr) != 0)
		return;

	error = dwarf_attr(die, DW_AT_name, &attr, &derr);
	if (error == DW_DLV_OK)
		func = GetFuncNameFromAttr(attr);
	else
		func = GetFuncNameFromSpec(dwarf, die);

	if (!GetFileNameFromAttr(dwarf, die, file))
		warnx("Failed for %s", func.c_str());
		file = m_image_file;

	if (!GetLineNumberFromAttr(dwarf, die, line)) {
		warnx("Failed for %s", func.c_str());
		line = 0;
	}

	m_functions[addr] = new DwarfLocation(file, func, line);
}

std::string
DwarfLookup::GetFuncNameFromAttr(Dwarf_Attribute attr)
{
	char *attrStr;
	Dwarf_Error derr;

	if (dwarf_formstring(attr, &attrStr, &derr) != 0)
		return ("");

	return (attrStr);
}

std::string
DwarfLookup::GetFuncNameFromSpec(Dwarf_Debug dwarf, Dwarf_Die die)
{
	Dwarf_Die attr_die;
	Dwarf_Attribute attr;
	Dwarf_Error derr;
	Dwarf_Off off;
	const char *func;
	int error;
	
	if (dwarf_attr(die, DW_AT_specification, &attr, &derr) != 0)
		return ("");
	if (dwarf_global_formref(attr, &off, &derr) != 0)
		return ("");
	if (dwarf_offdie(dwarf, off, &attr_die, &derr) != 0)
		return ("");

	error = dwarf_attrval_string(attr_die, DW_AT_name, &func, &derr);
	dwarf_dealloc(dwarf, attr_die, DW_DLA_DIE);
	if (error != 0)
		return ("");
	
	return (func);
}

bool
DwarfLookup::GetFileNameFromAttr(Dwarf_Debug dwarf, Dwarf_Die die,
    std::string &file)
{
	Dwarf_Die attr_die;
	Dwarf_Attribute attr;
	Dwarf_Error derr;
	Dwarf_Off off;
	const char *str;
	int error;
	
	if (dwarf_attr(die, DW_AT_decl_file, &attr, &derr) != 0) {
		warnx("Error @ %s:%d: %s", __FILE__, __LINE__, dwarf_errmsg(derr));
		return (false);
	}
	if (dwarf_global_formref(attr, &off, &derr) != 0){
		warnx("Error @ %s:%d: %s", __FILE__, __LINE__, dwarf_errmsg(derr));
		return (false);
	}
	if (dwarf_offdie(dwarf, off, &attr_die, &derr) != 0){
		warnx("Error @ %s:%d: %s", __FILE__, __LINE__, dwarf_errmsg(derr));
		return (false);
	}

	error = dwarf_attrval_string(attr_die, DW_AT_name, &str, &derr);
	dwarf_dealloc(dwarf, attr_die, DW_DLA_DIE);
	if (error != 0){
		warnx("Error @ %s:%d: %s", __FILE__, __LINE__, dwarf_errmsg(derr));
		return (false);
	}
	
	file = str;
	return (true);
}

bool
DwarfLookup::GetLineNumberFromAttr(Dwarf_Debug dwarf, Dwarf_Die die,
    int &lineno)
{
	Dwarf_Attribute attr;
	Dwarf_Error derr;
	Dwarf_Unsigned line;
	int error;
	
	error = dwarf_attr(die, DW_AT_decl_line, &attr, &derr);
	if (error != 0) {
		warnx("Error @ %s:%d: %s", __FILE__, __LINE__, dwarf_errmsg(derr));
		return (false);
	}

	error = dwarf_formudata(attr, &line, &derr);
	if (error != 0) {
		warnx("Error @ %s:%d: %s", __FILE__, __LINE__, dwarf_errmsg(derr));
		return (false);
	}

	lineno = line;
	return (true);
}

void
DwarfLookup::FillLocationMap(Dwarf_Debug dwarf)
{
	Dwarf_Die die;
	Dwarf_Error derr;
	int error;
	
	while ((error = dwarf_next_cu_header(dwarf, NULL, NULL, NULL, NULL,
	    NULL, &derr)) == DW_DLV_OK) {
		if (dwarf_siblingof(dwarf, NULL, &die, &derr) == DW_DLV_OK)
			FillLocationsFromDie(dwarf, die);
	}
}

void
DwarfLookup::FillLocationsFromDie(Dwarf_Debug dwarf, Dwarf_Die die)
{
	Dwarf_Die last_die;
	Dwarf_Error derr;
	Dwarf_Half tag;
	int error;

	do {
		if (dwarf_tag(die, &tag, &derr) == DW_DLV_OK) {
			if (tag == DW_TAG_compile_unit)
				AddLocations(dwarf, die);
		}

		last_die = die;
		error = dwarf_siblingof(dwarf, last_die, &die, &derr);
		dwarf_dealloc(dwarf, last_die, DW_DLA_DIE);
	} while (error == DW_DLV_OK);
}

void
DwarfLookup::AddLocations(Dwarf_Debug dwarf, Dwarf_Die die)
{
	std::string func, fileStr;
	char *file;
	Dwarf_Line *lbuf;
	Dwarf_Signed i, lcount;
	Dwarf_Unsigned line;
	Dwarf_Addr addr;
	Dwarf_Error de;

	if (dwarf_srclines(die, &lbuf, &lcount, &de) != DW_DLV_OK) {
		warnx("dwarf_srclines: %s", dwarf_errmsg(de));
		return;
	}

	for (i = 0; i < lcount; i++) {
		if (dwarf_lineaddr(lbuf[i], &addr, &de)) {
			warnx("dwarf_lineaddr: %s",
				dwarf_errmsg(de));
			return;
		}
		if (dwarf_lineno(lbuf[i], &line, &de)) {
			warnx("dwarf_lineno: %s",
				dwarf_errmsg(de));
			return;
		}
		if (dwarf_linesrc(lbuf[i], &file, &de)) {
			warnx("dwarf_linesrc: %s",
			    dwarf_errmsg(de));
			fileStr = m_image_file.c_str();
		} else
			fileStr = file;

		LocationMap::const_iterator it = m_functions.lower_bound(addr);
		if (it != m_functions.end())
			func = it->second->GetFunc();
		else
			func = "";
		m_locations[addr] = new DwarfLocation(fileStr, func, line);
	}
}

bool
DwarfLookup::Lookup(uintptr_t addr, const LocationMap &map,
    std::string &fileStr, std::string &funcStr, int &line) const
{

	LocationMap::const_iterator it = map.lower_bound(addr);

	if (it != map.end()) {
		fileStr = it->second->GetFile();
		funcStr = it->second->GetFunc();
		line = it->second->GetLineNumber();
		return (true);
	}

	return (false);
}

bool
DwarfLookup::LookupLine(uintptr_t addr, std::string &file, std::string &func,
	int &line) const
{

	return Lookup(addr, m_locations, file, func, line);
}

bool
DwarfLookup::LookupFunc(uintptr_t addr, std::string &file, std::string &func,
	int &line) const
{
	return Lookup(addr, m_functions, file, func, line);
}

bool
DwarfLookup::isContained(uintptr_t addr) const
{

	return ((addr >= m_text_start) && (addr < m_text_end));
}

bool
DwarfLookup::isOk() const
{

	return (m_text_end != 0);
}
