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

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include "DwarfLookup.h"

#include "DwarfCompileUnit.h"
#include "DwarfLocation.h"
#include "DwarfRange.h"
#include "MapUtil.h"

#include <err.h>
#include <fcntl.h>
#include <gelf.h>
#include <libgen.h>

#include <stdlib.h>
#include <string.h>

DwarfLookup::DwarfLookup(const std::string &filename)
  : m_image_file(filename),
    m_symbols_file(),
    m_text_start(0),
    m_text_end(0)
{
	Dwarf_Error derr;
	Elf *elf;
	int error, fd;

	if (filename.empty())
		return;

	fd = open(filename.c_str(), O_RDONLY);
	if (fd < 0) {
		warnx("unable to open file %s", filename.c_str());
		return;
	}

	elf = elf_begin(fd, ELF_C_READ, NULL);
	if (elf == NULL) {
		warnx("elf_begin failed: filename=%s elf_errno=%s",
		      filename.c_str(), elf_errmsg(elf_errno()));
		return;
	}

	ParseElfFile(elf);

	m_elf = GetSymbolFile(elf);

	/*
	 * It is not fatal if this fails: we'll do out best without debug
	 * symbols.
	 */
	error = dwarf_elf_init(m_elf, DW_DLC_READ, NULL, NULL, &m_dwarf, &derr);
	if (error == 0)
		EnumerateCompileUnits(m_dwarf);
}

DwarfLookup::~DwarfLookup()
{
	Dwarf_Error derr;

	for (CompileUnitList::iterator it = m_cu_list.begin();
	    it != m_cu_list.end(); ++it)
		delete *it;	
	
	for (LocationList::iterator it = m_locationList.begin();
	    it != m_locationList.end(); ++it)
		delete *it;

	for (RangeList::iterator it = m_ranges.begin();
	    it != m_ranges.end(); ++it)
		delete *it;

	dwarf_finish(m_dwarf, &derr);
	elf_end(m_elf);
}

Elf *
DwarfLookup::GetSymbolFile(Elf *elf)
{
	Elf * debug_elf;
	int fd;

	if (m_symbols_file.empty())
		return (elf);

	fd = FindSymbolFile();
	if (fd < 0)
		return (elf);

	debug_elf = elf_begin(fd, ELF_C_READ, NULL);
	if (debug_elf == NULL)
		return (elf);

	elf_end(elf);
	return (debug_elf);
}

int
DwarfLookup::FindSymbolFile()
{
	std::string file;
	std::string image_dir;
	int fd;

	image_dir = std::string(dirname(m_image_file.c_str()));

	file = image_dir + "/" + m_symbols_file;
	fd = open(file.c_str(), O_RDONLY);
	if (fd >= 0)
		return (fd);

	file = image_dir + "/.debug/" + m_symbols_file;
	fd = open(file.c_str(), O_RDONLY);
	if (fd >= 0)
		return (fd);

	file = "/usr/lib/debug/" + image_dir + "/" + m_symbols_file;
	fd = open(file.c_str(), O_RDONLY);
	if (fd >= 0)
		return (fd);

	return (-1);
}

void
DwarfLookup::ParseElfFile(Elf *elf)
{
	Elf_Scn *section;
	const char *name;
	GElf_Shdr shdr;
	size_t shdrstrndx;

	if (elf_getshdrstrndx(elf, &shdrstrndx) != 0)
		return;

	section = NULL;
	while ((section = elf_nextscn(elf, section)) != NULL) {
		if (gelf_getshdr(section, &shdr) == NULL)
			continue;

		gelf_getshdr(section, &shdr);
		name = elf_strptr(elf, shdrstrndx, shdr.sh_name);
		if (name != NULL) {
			if (strcmp(name, ".text") == 0) {
				m_text_start = shdr.sh_addr;
				m_text_end = m_text_start + shdr.sh_size;
			} else if (strcmp(name, ".gnu_debuglink") == 0)
				ParseDebuglink(section);
		}

		if (shdr.sh_type == SHT_SYMTAB)
			FillFunctionsFromSymtab(elf, section, &shdr);
	}

}

void
DwarfLookup::ParseDebuglink(Elf_Scn *section)
{
	Elf_Data *data;

	data = elf_rawdata(section, NULL);
	if (data != NULL && data->d_buf != NULL)
		m_symbols_file =
		    std::string(static_cast<const char *>(data->d_buf));
}

void
DwarfLookup::FillFunctionsFromSymtab(Elf *elf, Elf_Scn *section,
    GElf_Shdr *header)
{
	GElf_Sym symbol;
	Elf_Data *data;
	size_t i, count;

	count = header->sh_size / header->sh_entsize;
	data = elf_getdata(section, NULL);

	for (i = 0; i < count; i++) {
		gelf_getsym(data, i, &symbol);
		if (GELF_ST_TYPE(symbol.st_info) == STT_FUNC &&
		    symbol.st_shndx != SHN_UNDEF)
			AddFunction(symbol.st_value,
			    elf_strptr(elf, header->sh_link, symbol.st_name));
	}
}

void
DwarfLookup::AddFunction(GElf_Addr addr, const std::string &func)
{
	DwarfRange *range;
	DwarfLocation *loc;

	if (m_functions.count(addr) == 0) {
		loc = new DwarfLocation(m_image_file, func);
		m_locationList.push_back(loc);
		range = new DwarfRange(*loc);
		m_functions[addr] = range;
		m_ranges.push_back(range);
	}
}

void
DwarfLookup::EnumerateCompileUnits(Dwarf_Debug dwarf)
{
	Dwarf_Die die;
	Dwarf_Error derr;
	Dwarf_Unsigned next_offset;
	int error;

	while ((error = dwarf_next_cu_header(dwarf, NULL, NULL, NULL, NULL,
	    &next_offset, &derr)) == DW_DLV_OK) {
		if (dwarf_siblingof(dwarf, NULL, &die, &derr) == DW_DLV_OK)
			ProcessCompileUnit(dwarf, die);
	}
}

void
DwarfLookup::ProcessCompileUnit(Dwarf_Debug dwarf, Dwarf_Die die)
{
	Dwarf_Die last_die;
	Dwarf_Error derr;
	Dwarf_Half tag;
	int error;

	do {
		if (dwarf_tag(die, &tag, &derr) == DW_DLV_OK) {
			if (tag == DW_TAG_compile_unit)
				AddCompileUnit(dwarf, die);
		}

		last_die = die;
		error = dwarf_siblingof(dwarf, last_die, &die, &derr);

		dwarf_dealloc(dwarf, last_die, DW_DLA_DIE);
	} while (error == DW_DLV_OK);
}

void
DwarfLookup::AddCompileUnit(Dwarf_Debug dwarf, Dwarf_Die die)
{
	DwarfCompileUnit *cu;
	Dwarf_Error derr;
	Dwarf_Unsigned off;
	int error;
	
	cu = new DwarfCompileUnit(dwarf, die, m_image_file, m_functions);
	m_cu_list.push_back(cu);

	error = dwarf_attrval_unsigned(die, DW_AT_ranges, &off, &derr);
	if (error != DW_DLV_OK)
		AddCU_PC(dwarf, die, cu);
	else
		AddCU_Ranges(dwarf, die, cu, off);
}

void
DwarfLookup::AddCU_PC(Dwarf_Debug dwarf, Dwarf_Die die, DwarfCompileUnit *cu)
{
	Dwarf_Unsigned low_pc;
	Dwarf_Error derr;
	int error;

	error = dwarf_attrval_unsigned(die, DW_AT_low_pc, &low_pc, &derr);
	if (error == DW_DLV_OK)
		m_compile_units[low_pc] = cu;
}

void
DwarfLookup::AddCU_Ranges(Dwarf_Debug dwarf, Dwarf_Die die,
    DwarfCompileUnit *cu, Dwarf_Unsigned off)
{
	Dwarf_Error derr;
	Dwarf_Ranges *ranges;
	Dwarf_Signed i, count;
	Dwarf_Unsigned low_pc, base_addr, size;
	int error;

	error = dwarf_get_ranges(dwarf, off, &ranges, &count, &size, &derr);
	if (error != DW_DLV_OK)
		return;

	base_addr = cu->GetCUBaseAddr();
	for (i = 0; i < count; i++) {
		switch (ranges[i].dwr_type) {
		case DW_RANGES_ENTRY:
			low_pc = base_addr + ranges[i].dwr_addr1;
			m_compile_units[low_pc] = cu;
			break;
		case DW_RANGES_ADDRESS_SELECTION:
			base_addr = ranges[i].dwr_addr2;
			break;
		case DW_RANGES_END:
			goto break_loop;
		}
	}

break_loop:
	dwarf_ranges_dealloc(dwarf, ranges, count);
}

bool
DwarfLookup::LookupLine(uintptr_t addr, size_t depth, std::string &file,
    std::string &func, u_int &line) const
{
	bool success;
	
	CompileUnitMap::const_iterator it =
	    LastSmallerThan(m_compile_units, addr);
	if (it != m_compile_units.end()) {
		success = it->second->LookupLine(addr, depth, file, func, line);
		if (success)
			return (true);
	}

	/*
	 * If debug symbols are not present then fall back on ELF data.  We
	 * won't get the source line or file, but the function name is better
	 * than nothing.
	 */
	return (LookupFunc(addr, file, func, line));
}

bool
DwarfLookup::LookupFunc(uintptr_t addr, std::string &file,
    std::string &func, u_int &line) const
{

	return (DwarfCompileUnit::Lookup(addr, m_functions, file, func, line));
}

const std::string &
DwarfLookup::getImageFile() const
{

	return (m_image_file);
}

bool
DwarfLookup::isContained(uintptr_t addr) const
{

	return ((addr >= m_text_start) && (addr < m_text_end));
}

size_t
DwarfLookup::GetInlineDepth(uintptr_t addr) const
{
	CompileUnitMap::const_iterator it =
	    LastSmallerThan(m_compile_units, addr);
	if (it != m_compile_units.end())
		return (it->second->GetInlineDepth(addr));
	return (1);
}

