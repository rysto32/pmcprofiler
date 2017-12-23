// Copyright (c) 2014-2015 Sandvine Incorporated.  All rights reserved.
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

#include "DwarfResolver.h"

#include "Callframe.h"
#include "DwarfDieList.h"
#include "DwarfException.h"
#include "DwarfRangeList.h"
#include "DwarfSearch.h"
#include "DwarfSrcLine.h"
#include "DwarfSrcLinesList.h"
#include "DwarfUtil.h"
#include "MapUtil.h"

#include <dwarf.h>
#include <err.h>
#include <fcntl.h>
#include <gelf.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>

#include <cstring>

#ifdef LOG_ENABLED

#define LOG(args...) \
	fprintf(stderr, args)

#else

#define LOG(args...)

#endif

DwarfResolver::DwarfResolver(SharedString image)
  : imageFile(image),
    elf(NULL),
    dwarf(DW_DLV_BADADDR)
{
	Dwarf_Error derr;

	if (imageFile->empty())
		return;

	elf = GetSymbolFile();
	if (elf == NULL)
		return;

	/*
	 * It is not fatal if this fails: we'll do out best without debug
	 * symbols.
	 */
	dwarf_elf_init(elf, DW_DLC_READ, NULL, NULL, &dwarf, &derr);
}

DwarfResolver::~DwarfResolver()
{
	Dwarf_Error derr;

	if (DwarfValid())
		dwarf_finish(dwarf, &derr);

	elf_end(elf);
}

Elf *
DwarfResolver::GetSymbolFile()
{
	int fd = open(imageFile->c_str(), O_RDONLY);
	if (fd < 0) {
		warnx("unable to open file %s", imageFile->c_str());
		return (NULL);
	}

	Elf * origElf = elf_begin(fd, ELF_C_READ, NULL);
	if (origElf == NULL) {
		warnx("elf_begin failed: filename=%s elf_errno=%s",
		    imageFile->c_str(), elf_errmsg(elf_errno()));
		return (NULL);
	}

	if (HaveSymbolFile(origElf))
		return OpenSymbolFile(origElf);
	else
		return origElf;
}

Elf *
DwarfResolver::OpenSymbolFile(Elf* origElf)
{
	Elf * debug_elf;
	int fd;

	if (symbolFile->empty())
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
DwarfResolver::FindSymbolFile()
{
	std::string file;
	std::string image_dir;
	char *dir;
	int fd;

	dir = strdup(imageFile->c_str());
	image_dir = std::string(dirname(dir));
	free(dir);

	file = image_dir + "/" + *symbolFile;
	fd = open(file.c_str(), O_RDONLY);
	if (fd >= 0)
		return (fd);

	file = image_dir + "/.debug/" + *symbolFile;
	fd = open(file.c_str(), O_RDONLY);
	if (fd >= 0)
		return (fd);

	file = "/usr/lib/debug/" + image_dir + "/" + *symbolFile;
	fd = open(file.c_str(), O_RDONLY);
	if (fd >= 0)
		return (fd);

	return (-1);
}

bool
DwarfResolver::HaveSymbolFile(Elf *origElf)
{
	Elf_Scn *section;
	const char *name;
	GElf_Shdr shdr;
	size_t shdrstrndx;

	if (elf_getshdrstrndx(origElf, &shdrstrndx) != 0)
		return false;

	section = NULL;
	while ((section = elf_nextscn(origElf, section)) != NULL) {
		if (gelf_getshdr(section, &shdr) == NULL)
			continue;

		gelf_getshdr(section, &shdr);

		if (shdr.sh_type == SHT_SYMTAB || shdr.sh_type == SHT_DYNSYM)
			FillElfSymbolMap(origElf, section);

		name = elf_strptr(origElf, shdrstrndx, shdr.sh_name);
		if (name != NULL) {
			if (strcmp(name, ".gnu_debuglink") == 0) {
				ParseDebuglink(section);
				return !symbolFile->empty();
			}
		}
	}

	return false;
}

void
DwarfResolver::ParseDebuglink(Elf_Scn *section)
{
	Elf_Data *data;

	data = elf_rawdata(section, NULL);
	if (data != NULL && data->d_buf != NULL)
		symbolFile = static_cast<const char *>(data->d_buf);
}

bool
DwarfResolver::DwarfValid() const
{
	return dwarf != DW_DLV_BADADDR;
}

bool
DwarfResolver::ElfValid() const
{
	return elf != NULL;
}

void
DwarfResolver::Resolve(const FrameMap &frameMap) const
{
	if (DwarfValid())
		ResolveDwarf(frameMap);
	else if (ElfValid())
		ResolveElf(frameMap);
	else
		ResolveUnmapped(frameMap);
}

void
DwarfResolver::ResolveDwarf(const FrameMap &frameMap) const
{
	Dwarf_Die die;
	Dwarf_Error derr;
	Dwarf_Unsigned next_offset;
	int error;

	auto fit = frameMap.begin();

	while (fit != frameMap.end()) {
		error = dwarf_next_cu_header(dwarf, NULL, NULL, NULL, NULL,
		    &next_offset, &derr);
		if (error != DW_DLV_OK) {
			LOG("dwarf_next_cu_header: Got error %d\n", error);
			break;
		}

		error = dwarf_siblingof(dwarf, NULL, &die, &derr);
		if (error == DW_DLV_OK)
			ProcessCompileUnit(die, fit, frameMap.end());
		else
			LOG("dwarf_siblingof: Got error %d\n", error);
	}

	while (fit != frameMap.end()) {
		fit->second->setUnmapped(imageFile);
		fit++;
	}
}

void
DwarfResolver::ProcessCompileUnit(Dwarf_Die die, FrameMap::const_iterator &fit,
    const FrameMap::const_iterator &end) const
{
	Dwarf_Die last_die;
	Dwarf_Error derr;
	Dwarf_Half tag;
	int error;

// 	LOG("%s: process %lx\n", imageFile->c_str(),
// 	    GetDieOffset(die));

	do {
		if (dwarf_tag(die, &tag, &derr) == DW_DLV_OK) {
			if (tag == DW_TAG_compile_unit) {
				try {
					SearchCompileUnit(die, fit, end);
				} catch (const DwarfException &)
				{
					// swallow it and continue.  libdwarf
					// errors are not fatal.
				}
			}
		}

		last_die = die;
		error = dwarf_siblingof(dwarf, last_die, &die, &derr);

		dwarf_dealloc(dwarf, last_die, DW_DLA_DIE);
	} while (error == DW_DLV_OK && fit != end);
}

void
DwarfResolver::SearchCompileUnit(Dwarf_Die cu, FrameMap::const_iterator &fit,
    const FrameMap::const_iterator &end) const
{
	Dwarf_Error derr;
	Dwarf_Unsigned range_off, low_pc, high_pc;
	int error, err_lo, err_hi;

	error = dwarf_attrval_unsigned(cu, DW_AT_ranges, &range_off, &derr);
	if (error == DW_DLV_OK) {
		return (SearchCompileUnitRanges(cu, range_off, fit, end));
	}

	err_lo = dwarf_attrval_unsigned(cu, DW_AT_low_pc, &low_pc, &derr);
	err_hi = dwarf_attrval_unsigned(cu, DW_AT_high_pc, &high_pc, &derr);
	if (err_lo == DW_DLV_OK && err_hi == DW_DLV_OK) {
		TryCompileUnitRange(cu, low_pc, high_pc, fit, end);
		return;
	}

	// We can't tell whether fit is covered by this CU.  Try the srclines
	// data and see if we find a match.
	SearchCompileUnitFuncs(cu, fit, end);
}

void
DwarfResolver::TryCompileUnitRange(Dwarf_Die cu, Dwarf_Unsigned low_pc,
    Dwarf_Unsigned high_pc, FrameMap::const_iterator &fit,
    const FrameMap::const_iterator &end) const
{
	do {
		TargetAddr offset = fit->second->getOffset();
// 		LOG("0x%lx: Try cu %lx [0x%lx, 0x%lx)\n",
// 		    offset, GetDieOffset(cu), low_pc, high_pc);
		if (offset < low_pc){
			fit->second->setUnmapped(imageFile);
			++fit;
		}
		else if (offset < high_pc) {
			SearchCompileUnitFuncs(cu, fit, end);
		} else {
			break;
		}
	} while (fit != end);
}

void
DwarfResolver::SearchCompileUnitRanges(Dwarf_Die cu, Dwarf_Unsigned range_off,
    FrameMap::const_iterator &fit, const FrameMap::const_iterator &end) const
{
	int error;
	Dwarf_Error derr;
	Dwarf_Unsigned base_addr, low_pc, high_pc;

	DwarfRangeList rangeList(dwarf, range_off);

	error = dwarf_attrval_unsigned(cu, DW_AT_low_pc, &base_addr, &derr);
	if (error != 0)
		base_addr = 0;

	for (auto & range : rangeList) {
		switch (range.dwr_type) {
		case DW_RANGES_ENTRY:
			low_pc = base_addr + range.dwr_addr1;
			high_pc = base_addr + range.dwr_addr2;
			TryCompileUnitRange(cu, low_pc, high_pc, fit, end);
			break;
		case DW_RANGES_ADDRESS_SELECTION:
			base_addr = range.dwr_addr2;
			break;
		case DW_RANGES_END:
			goto break_loop;
		}
	}

break_loop:
	return;
}

void
DwarfResolver::SearchCompileUnitFuncs(Dwarf_Die cu, FrameMap::const_iterator &fit,
    const FrameMap::const_iterator &fend) const
{
	DwarfSearch search(dwarf, cu, imageFile, elfSymbols);

	LOG("%s: map %lx in %lx\n", imageFile->c_str(),
	    fit->second->getOffset(), GetDieOffset(cu));

	search.AdvanceAndMap(*fit->second);
	++fit;
}

void
DwarfResolver::ResolveElf(const FrameMap &frameMap) const
{
	auto fit = frameMap.begin();
	auto sit = elfSymbols.begin();

	/*
	 * Any frames mapped before the start of our symbols have to be
	 * set unmapped.
	 */
	while (fit != frameMap.end() && (sit == elfSymbols.end() ||
	    fit->second->getOffset() < sit->first)) {
		fit->second->setUnmapped(imageFile);
		fit++;
	}

	/*
	 * Iterate over the frames and symbols in tandem.  When we find a point
	 * where old_symbol_offset <= frame_offset < next_symbol_offset, then we
	 * map the frame to old_symbol.
	 */
	while (fit != frameMap.end()) {
		assert (sit->first <= fit->second->getOffset());

		auto old_sit = sit;
		++sit;

		while (fit != frameMap.end() && (sit == elfSymbols.end() ||
		    fit->second->getOffset() > sit->first)) {
			fit->second->addFrame(imageFile, old_sit->second,
			    old_sit->second, -1, -1);
			++fit;
		}
	}
}

void
DwarfResolver::FillElfSymbolMap(Elf *imageElf, Elf_Scn *section)
{
	GElf_Shdr header;
	GElf_Sym symbol;
	Elf_Data *data;
	size_t i, count;

	if (gelf_getshdr(section, &header) == NULL)
		return;

	count = header.sh_size / header.sh_entsize;
	data = elf_getdata(section, NULL);

	for (i = 0; i < count; i++) {
		gelf_getsym(data, i, &symbol);
		if (GELF_ST_TYPE(symbol.st_info) == STT_FUNC &&
		    symbol.st_shndx != SHN_UNDEF) {
			const char *name = elf_strptr(imageElf, header.sh_link, symbol.st_name);
			elfSymbols.insert(std::make_pair(symbol.st_value,
			    name));
		}
	}
}

void
DwarfResolver::ResolveUnmapped(const FrameMap &frameMap) const
{
	for (auto & pair : frameMap)
		pair.second->setUnmapped(imageFile);
}
