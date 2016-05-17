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

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

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

DwarfCompileUnit::DwarfCompileUnit(Dwarf_Debug dwarf, Dwarf_Die die,
   const SharedString &file, const RangeMap &functions)
  : m_dwarf(dwarf),
    m_die_offset(GetDieOffset(die)),
    m_die_processed(false),
    m_image_file(file),
    m_functions(functions)
{
	Dwarf_Error derr;
	int error;

	error = dwarf_attrval_unsigned(die, DW_AT_low_pc, &m_base_addr, &derr);
	if (error != 0)
		m_base_addr = 0;
}

DwarfCompileUnit::~DwarfCompileUnit()
{

	for (DwarfLocationList::iterator it = m_locationList.begin();
	    it != m_locationList.end(); ++it)
		delete *it;

	for (RangeList::iterator it = m_ranges.begin();
	    it != m_ranges.end(); ++it)
		delete *it;
}

void
DwarfCompileUnit::AddLocations()
{
	DwarfLocation *loc;
	DwarfRange *range;
	SharedString fileStr;
	char *file;
	Dwarf_Die die;
	Dwarf_Line *lbuf;
	Dwarf_Signed i, lcount;
	Dwarf_Unsigned line;
	Dwarf_Addr addr;
	Dwarf_Error de;

	if (m_die_processed)
		return;
	m_die_processed = true;
	
	die = GetDie();
	if (die == NULL)
		return;

	if (dwarf_srclines(die, &lbuf, &lcount, &de) != DW_DLV_OK)
		goto out;

	for (i = 0; i < lcount; i++) {
		if (dwarf_lineaddr(lbuf[i], &addr, &de)) {
			warnx("dwarf_lineaddr: %s",
				dwarf_errmsg(de));
			goto out;
		}
		if (dwarf_lineno(lbuf[i], &line, &de)) {
			warnx("dwarf_lineno: %s",
				dwarf_errmsg(de));
			goto out;
		}
		if (dwarf_linesrc(lbuf[i], &file, &de)) {
			warnx("dwarf_linesrc: %s",
			    dwarf_errmsg(de));
			fileStr = m_image_file;
		} else
			fileStr = file;

		RangeMap::const_iterator it = LastSmallerThan(m_functions, addr);
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

	FillCUInlines(m_dwarf, die);

out:
	FreeDie(die);
}

void
DwarfCompileUnit::FillCUInlines(Dwarf_Debug dwarf, Dwarf_Die cu)
{

	FillInlineFunctions(dwarf, cu, cu);
	SetAssemblyFuncs();
}

/*
 * There is no DWARF information mapping addresses to functions.  As a final pass,
 * look for all locations that don't have a function mapped and use the ELF data
 * to find its function name.
 */
void
DwarfCompileUnit::SetAssemblyFuncs()
{
	RangeMap::iterator it = m_locations.begin();
	for (; it != m_locations.end(); ++it) {
		DwarfLocation &loc = it->second->GetOutermostCaller()->GetLocation();
		if (loc.NeedsFunc()) {
			RangeMap::const_iterator func = LastSmallerThan(m_functions, it->first);
			if (func != m_functions.end())
				loc.SetFunc(func->second->GetLocation().GetFunc());
		}
	}
}

void
DwarfCompileUnit::FillInlineFunctions(Dwarf_Debug dwarf, Dwarf_Die cu, Dwarf_Die die)
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
DwarfCompileUnit::AddInlines(Dwarf_Debug dwarf, Dwarf_Die cu, Dwarf_Die die)
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
DwarfCompileUnit::GetCUBaseAddr() const
{

	return (m_base_addr);
}

void
DwarfCompileUnit::AddInlineRanges(Dwarf_Debug dwarf, Dwarf_Die cu, Dwarf_Die die,
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

	base_addr = GetCUBaseAddr();
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
DwarfCompileUnit::AddInlineLoc(DwarfLocation *loc, Dwarf_Debug dwarf, Dwarf_Die die,
    uintptr_t low, uintptr_t high)
{
	RangeMap::iterator insert_hint;
	DwarfRange *inline_range, *outer;

	RangeMap::iterator it = LastSmallerThan(m_locations, low);
	if (it == m_locations.end())
		return;

	inline_range = new DwarfRange(*loc);
	m_ranges.push_back(inline_range);
	SharedString inlineFunc(GetSubprogramName(dwarf, die));
	for (; it != m_locations.end() && it->first < high; ++it) {
		outer = it->second->GetOutermostCaller();
		DwarfLocation &loc = outer->GetLocation();
		if (outer != inline_range)
			SetLocationFunc(loc, inlineFunc);

		it->second->SetCaller(inline_range);
	}
}

DwarfLocation *
DwarfCompileUnit::UnknownLocation()
{
	DwarfLocation *loc;

	loc = new DwarfLocation(m_image_file, "<unknown>", 0, 0);
	m_locationList.push_back(loc);
	return (loc);
}

DwarfLocation *
DwarfCompileUnit::GetInlineCaller(Dwarf_Debug dwarf, Dwarf_Die cu, Dwarf_Die die)
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

	std::string filename(filenames[fileindex]);

	/*
	 * The name of the function isn't available here.  We'll fill in the
	 * function name a bit later.
	 */
	loc = new DwarfLocation(filename, "", lineno, GetDieOffset(die));
	m_locationList.push_back(loc);
	return (loc);
}

SharedString
DwarfCompileUnit::GetSubprogramName(Dwarf_Debug dwarf, Dwarf_Die func)
{
	Dwarf_Die origin_die;
	Dwarf_Attribute origin_attr;
	Dwarf_Off ref;
	Dwarf_Error derr;
	SharedString name;
	SharedString attr_name;
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

	if (name->empty())
		return (attr_name);
	else
		return (name);
}

SharedString
DwarfCompileUnit::GetNameAttr(Dwarf_Debug dwarf, Dwarf_Die die)
{
	Dwarf_Error derr;
	const char *func;
	int error;

	error = dwarf_attrval_string(die, DW_AT_MIPS_linkage_name, &func,
	    &derr);
	if (error != DW_DLV_OK) {
		error = dwarf_attrval_string(die, DW_AT_name, &func, &derr);
		if (error != DW_DLV_OK)
			return ("");
	}

	return (func);
}

SharedString
DwarfCompileUnit::SpecSubprogramName(Dwarf_Debug dwarf, Dwarf_Die func_die)
{
	Dwarf_Die die;
	Dwarf_Attribute spec_at;
	Dwarf_Error derr;
	Dwarf_Off ref;
	SharedString funcStr;
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
DwarfCompileUnit::SetInlineCaller(Dwarf_Debug dwarf, Dwarf_Die die)
{
	Dwarf_Unsigned low_pc, high_pc;
	Dwarf_Error derr;
	int error;
	Dwarf_Off offset;
	dwarf_dieoffset(die, &offset, &derr);

	SharedString func(GetSubprogramName(dwarf, die));

	error = dwarf_attrval_unsigned(die, DW_AT_low_pc, &low_pc, &derr);
	if (error != DW_DLV_OK)
		return;

	error = dwarf_attrval_unsigned(die, DW_AT_high_pc, &high_pc,
	    &derr);
	if (error != DW_DLV_OK)
		high_pc = low_pc + 1;

	RangeMap::iterator it = LastSmallerThan(m_locations, low_pc);
	for (; it != m_locations.end() && it->first <= high_pc; ++it) {
		SetLocationFunc(it->second->GetOutermostCaller()->GetLocation(), func);
	}
}

void
DwarfCompileUnit::SetLocationFunc(DwarfLocation &loc, SharedString func)
{

	if (loc.NeedsFunc())
		loc.SetFunc(func);
}

Dwarf_Die
DwarfCompileUnit::GetDie() const
{
	Dwarf_Die die;
	Dwarf_Error derr;
	int error;
	
	while ((error = dwarf_next_cu_header(m_dwarf, NULL, NULL, NULL, NULL,
	    NULL, &derr)) == DW_DLV_OK) {
		if (dwarf_siblingof(m_dwarf, NULL, &die, &derr) == DW_DLV_OK) {
			if (GetDieOffset(die) == m_die_offset)
				return (die);
			dwarf_dealloc(m_dwarf, die, DW_DLA_DIE);
		}
	}

	return (DW_DLV_BADADDR);
}

void
DwarfCompileUnit::FreeDie(Dwarf_Die die) const
{
	Dwarf_Error derr;
	
	dwarf_dealloc(m_dwarf, die, DW_DLA_DIE);

	/* Need to keep iterating to reset the state on m_dwarf. */
	while (dwarf_next_cu_header(m_dwarf, NULL, NULL, NULL, NULL,
	    NULL, &derr) == DW_DLV_OK) {
	}
}

Dwarf_Off
DwarfCompileUnit::GetDieOffset(Dwarf_Die die)
{
	Dwarf_Error derr;
	Dwarf_Off offset;

	dwarf_dieoffset(die, &offset, &derr);
	return (offset);
}

bool
DwarfCompileUnit::Lookup(uintptr_t addr, const RangeMap &map,
    SharedString &fileStr, SharedString &funcStr, u_int &line, size_t inlineDepth)
{
	const DwarfRange *range;
	RangeMap::const_iterator it = LastSmallerThan(map, addr);
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
DwarfCompileUnit::LookupLine(uintptr_t addr, size_t depth, SharedString &file,
    SharedString &func, u_int &line)
{

	AddLocations();
	return (Lookup(addr, m_locations, file, func, line, depth));
}

size_t
DwarfCompileUnit::GetInlineDepth(uintptr_t addr)
{

	AddLocations();
	RangeMap::const_iterator it = LastSmallerThan(m_locations, addr);

	if (it != m_locations.end())
		return (it->second->GetInlineDepth());
	return (1);
}

