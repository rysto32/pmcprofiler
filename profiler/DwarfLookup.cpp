// Copyright (c) 2014 Sandvine Incorporated.  All rights reserved.
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
#include "DwarfLocation.h"
#include "DwarfRange.h"

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
	Dwarf_Debug dwarf;
	Dwarf_Error derr;
	Elf *elf;
	int fd;

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

	elf = GetSymbolFile(elf);

	/*
	 * It is not fatal if this fails: we'll do out best without debug
	 * symbols.
	 */
	if (dwarf_elf_init(elf, DW_DLC_READ, NULL, NULL, &dwarf, &derr) == 0) {
		FillRangeMap(dwarf);
		dwarf_finish(dwarf, &derr);
	}

	elf_end(elf);
}

DwarfLookup::~DwarfLookup()
{

	for (LocationList::iterator it = m_locationList.begin();
	    it != m_locationList.end(); ++it)
		delete *it;

	for (RangeList::iterator it = m_ranges.begin();
	    it != m_ranges.end(); ++it)
		delete *it;
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
DwarfLookup::FillRangeMap(Dwarf_Debug dwarf)
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
	DwarfLocation *loc;
	DwarfRange *range;
	std::string fileStr;
	char *file;
	Dwarf_Line *lbuf;
	Dwarf_Signed i, lcount;
	Dwarf_Unsigned line;
	Dwarf_Addr addr;
	Dwarf_Error de;

	if (dwarf_srclines(die, &lbuf, &lcount, &de) != DW_DLV_OK)
		return;

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

		RangeMap::const_iterator it = m_functions.lower_bound(addr);
		if (it != m_functions.end()) {
			DwarfLocation &funcLoc = it->second->
			    GetOutermostCaller()->GetLocation();
			if (funcLoc.NeedsDebug() && line != 0)
				funcLoc.SetDebug(file, line);
		}

		if (m_locations.count(addr) == 0) {
			loc = new DwarfLocation(fileStr, "", line, 0);
			m_locationList.push_back(loc);
			range = new DwarfRange(*loc);
			m_locations[addr] = range;
			m_ranges.push_back(range);
		}
	}

	FillCUInlines(dwarf, die);
}

void
DwarfLookup::FillCUInlines(Dwarf_Debug dwarf, Dwarf_Die cu)
{

	FillInlineFunctions(dwarf, cu, cu);
}

void
DwarfLookup::FillInlineFunctions(Dwarf_Debug dwarf, Dwarf_Die cu, Dwarf_Die die)
{
	Dwarf_Die last_die, child;
	Dwarf_Error derr;
	Dwarf_Half tag;
	int error, tag_err;

	do {

		error = dwarf_child(die, &child, &derr);
		if (error == DW_DLV_OK)
			FillInlineFunctions(dwarf, cu, child);

		tag_err = dwarf_tag(die, &tag, &derr);
		if (tag_err == DW_DLV_OK) {
			if (tag == DW_TAG_inlined_subroutine)
				AddInlines(dwarf, cu, die);
			else if(tag == DW_TAG_subprogram)
				SetInlineCaller(dwarf, die);
		}

		last_die = die;
		error = dwarf_siblingof(dwarf, last_die, &die, &derr);
		if (last_die != cu)
			dwarf_dealloc(dwarf, last_die, DW_DLA_DIE);
	} while (error == DW_DLV_OK);
}

void
DwarfLookup::AddInlines(Dwarf_Debug dwarf, Dwarf_Die cu, Dwarf_Die die)
{
	DwarfLocation *loc;
	Dwarf_Unsigned low_pc, high_pc;
	Dwarf_Error derr;
	int error;

	loc = GetInlineCaller(dwarf, cu, die);

	error = dwarf_attrval_unsigned(die, DW_AT_low_pc, &low_pc, &derr);
	if (error == DW_DLV_OK) {
		error = dwarf_attrval_unsigned(die, DW_AT_high_pc, &high_pc,
		    &derr);
		if (error != DW_DLV_OK)
			high_pc = low_pc + 1;
		
		AddInlineLoc(loc, dwarf, die, low_pc, high_pc);
	} else
		AddInlineRanges(dwarf, cu, die, loc);
}

Dwarf_Unsigned
DwarfLookup::GetCUBaseAddr(Dwarf_Die cu)
{
	Dwarf_Error derr;
	Dwarf_Unsigned low_pc;
	int error;

	error = dwarf_attrval_unsigned(cu, DW_AT_low_pc, &low_pc, &derr);
	if (error != 0)
		abort();

	return (low_pc);
}

void
DwarfLookup::AddInlineRanges(Dwarf_Debug dwarf, Dwarf_Die cu, Dwarf_Die die,
    DwarfLocation *loc)
{
	Dwarf_Error derr;
	Dwarf_Ranges *ranges;
	Dwarf_Signed i, count;
	Dwarf_Unsigned low_pc, high_pc, base_addr, off, size;
	int error;

	ranges = NULL;
	count = 0;

	error = dwarf_attrval_unsigned(die, DW_AT_ranges, &off, &derr);
	if (error != DW_DLV_OK)
		return;

	error = dwarf_get_ranges(dwarf, off, &ranges, &count, &size, &derr);
	if (error != DW_DLV_OK)
		return;

	base_addr = GetCUBaseAddr(cu);
	for (i = 0; i < count; i++) {
		switch (ranges[i].dwr_type) {
		case DW_RANGES_ENTRY:
			low_pc = base_addr + ranges[i].dwr_addr1;
			high_pc = base_addr + ranges[i].dwr_addr2;
			AddInlineLoc(loc, dwarf, die, low_pc, high_pc);
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

void
DwarfLookup::AddInlineLoc(DwarfLocation *loc, Dwarf_Debug dwarf, Dwarf_Die die,
    uintptr_t low, uintptr_t high)
{
	RangeMap::iterator insert_hint;
	DwarfRange *inline_range, *outer;

	RangeMap::iterator it = m_locations.lower_bound(low);
	if (it == m_locations.end())
		return;

	inline_range = new DwarfRange(*loc);
	m_ranges.push_back(inline_range);
	std::string inlineFunc(GetSubprogramName(dwarf, die));
	for (; it != m_locations.end() && it->first < high; --it) {
		outer = it->second->GetOutermostCaller();
		DwarfLocation &loc = outer->GetLocation();
		if (outer != inline_range)
			SetLocationFunc(loc, inlineFunc);

		it->second->SetCaller(inline_range);
	}
}

DwarfLocation *
DwarfLookup::UnknownLocation()
{
	DwarfLocation *loc;

	loc = new DwarfLocation(m_image_file, "<unknown>", 0, 0);
	m_locationList.push_back(loc);
	return (loc);
}

DwarfLocation *
DwarfLookup::GetInlineCaller(Dwarf_Debug dwarf, Dwarf_Die cu, Dwarf_Die die)
{
	DwarfLocation *loc;
	Dwarf_Unsigned  fileno, fileindex, lineno;
	Dwarf_Signed numfiles;
	Dwarf_Error derr;
	char **filenames;
	int error;

	error = dwarf_attrval_unsigned(die, DW_AT_call_file, &fileno, &derr);
	if (error != DW_DLV_OK)
		return (UnknownLocation());

	error = dwarf_attrval_unsigned(die, DW_AT_call_line, &lineno, &derr);
	if (error != DW_DLV_OK)
		return (UnknownLocation());

	error = dwarf_srcfiles(cu, &filenames, &numfiles, &derr);
	if (error != DW_DLV_OK)
		return (UnknownLocation());

	/* filenames is indexed from 0 but fileno starts at 1, so subtract 1. */
	fileindex = fileno - 1;
	if (fileindex >= (Dwarf_Unsigned)numfiles)
		return (UnknownLocation());

	Dwarf_Off offset;
	dwarf_dieoffset(die, &offset, &derr);

	std::string filename(filenames[fileindex]);

	/*
	 * The name of the function isn't available here.  We'll fill in the
	 * function name a bit later.
	 */
	loc = new DwarfLocation(filename, "", lineno, offset);
	m_locationList.push_back(loc);
	return (loc);
}

std::string
DwarfLookup::GetSubprogramName(Dwarf_Debug dwarf, Dwarf_Die func)
{
	Dwarf_Die origin_die;
	Dwarf_Attribute origin_attr;
	Dwarf_Off ref;
	Dwarf_Error derr;
	std::string name;
	std::string attr_name;
	int error;

	/* Fallback name to use if all else fails. */
	attr_name = GetNameAttr(dwarf, func);

	error = dwarf_attr(func, DW_AT_abstract_origin, &origin_attr, &derr);
	if (error != DW_DLV_OK)
		return (attr_name);

	error = dwarf_global_formref(origin_attr, &ref, &derr);
	if (error != DW_DLV_OK)
		return (attr_name);

	error = dwarf_offdie(dwarf, ref, &origin_die, &derr);
	if (error != DW_DLV_OK)
		return (attr_name);

	name = SpecSubprogramName(dwarf, origin_die);

	dwarf_dealloc(dwarf, origin_die, DW_DLA_DIE);

	if (name.empty())
		return (attr_name);
	else
		return (name);
}

std::string
DwarfLookup::GetNameAttr(Dwarf_Debug dwarf, Dwarf_Die die)
{
	Dwarf_Error derr;
	const char *func;
	std::string funcStr;
	int error;

	error = dwarf_attrval_string(die, DW_AT_MIPS_linkage_name, &func,
	    &derr);
	if (error != DW_DLV_OK) {
		error = dwarf_attrval_string(die, DW_AT_name, &func, &derr);
		if (error != DW_DLV_OK)
			func = "";
	}

	funcStr = func;
	return (funcStr);
}

std::string
DwarfLookup::SpecSubprogramName(Dwarf_Debug dwarf, Dwarf_Die func_die)
{
	Dwarf_Die die;
	Dwarf_Attribute spec_at;
	Dwarf_Error derr;
	Dwarf_Off ref;
	std::string funcStr;
	int error;

	error = dwarf_attr(func_die, DW_AT_specification, &spec_at, &derr);
	if (error != DW_DLV_OK)
		return (GetNameAttr(dwarf, func_die));

	error = dwarf_global_formref(spec_at, &ref, &derr);
	if (error != DW_DLV_OK)
		return ("");

	error = dwarf_offdie(dwarf, ref, &die, &derr);
	if (error != DW_DLV_OK)
		return ("");

	funcStr = GetNameAttr(dwarf, die);
	dwarf_dealloc(dwarf, die, DW_DLA_DIE);
	return (funcStr);
}

void
DwarfLookup::SetInlineCaller(Dwarf_Debug dwarf, Dwarf_Die die)
{
	Dwarf_Unsigned low_pc, high_pc;
	Dwarf_Error derr;
	int error;
	Dwarf_Off offset;
	dwarf_dieoffset(die, &offset, &derr);

	std::string func(GetSubprogramName(dwarf, die));

	error = dwarf_attrval_unsigned(die, DW_AT_low_pc, &low_pc, &derr);
	if (error != DW_DLV_OK)
		return;

	error = dwarf_attrval_unsigned(die, DW_AT_high_pc, &high_pc,
	    &derr);
	if (error != DW_DLV_OK)
		high_pc = low_pc + 1;

	RangeMap::iterator it = m_locations.lower_bound(low_pc);
	for (; it != m_locations.end() && it->first <= high_pc; --it) {
		SetLocationFunc(it->second->GetOutermostCaller()->GetLocation(), func);
	}
}

void
DwarfLookup::SetLocationFunc(DwarfLocation &loc, const std::string func)
{

	if (loc.NeedsFunc())
		loc.SetFunc(func);
}

bool
DwarfLookup::Lookup(uintptr_t addr, const RangeMap &map, size_t inlineDepth,
    std::string &fileStr, std::string &funcStr, u_int &line) const
{
	const DwarfRange *range;
	RangeMap::const_iterator it = map.lower_bound(addr);
	size_t i;

	if (it != map.end()) {
		range = it->second;
		for (i = 0; i < inlineDepth; i++)
			range = range->GetCaller();

		DwarfLocation &location = range->GetLocation();
		fileStr = location.GetFile();
		funcStr = location.GetFunc();
		line = location.GetLineNumber();
		return (true);
	}

	return (false);
}

bool
DwarfLookup::LookupLine(uintptr_t addr, size_t depth, std::string &file,
    std::string &func, u_int &line) const
{
	bool success;

	success = Lookup(addr, m_locations, depth, file, func, line);
	if (success)
		return (true);

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

	return (Lookup(addr, m_functions, 0, file, func, line));
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
	RangeMap::const_iterator it = m_locations.lower_bound(addr);

	if (it != m_locations.end())
		return (it->second->GetInlineDepth());
	return (1);
}

