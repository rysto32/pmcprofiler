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
#include "DwarfCompileUnit.h"
#include "DwarfCompileUnitDie.h"
#include "DwarfDieList.h"
#include "DwarfRangeLookup.h"
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

DwarfResolver::DwarfResolver(SharedString image)
  : imageFile(image),
    elf(NULL),
    dwarf(nullptr)
{
	Dwarf_Error derr;

	if (imageFile->empty())
		return;

	elf = GetSymbolFile();
	if (elf == NULL)
		return;

	LOG("imageFile=%s symbolsFile=%s", imageFile->c_str(),
	    symbolFile->c_str());

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
		return (origElf);

	fd = FindSymbolFile();
	if (fd < 0)
		return (origElf);

	debug_elf = elf_begin(fd, ELF_C_READ, NULL);
	if (debug_elf == NULL)
		return (origElf);

	elf_end(origElf);
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
			if (strcmp(name, ".gnu_debuglink") == 0)
				ParseDebuglink(section);
		}
	}

	return !symbolFile->empty();
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
	return dwarf != nullptr;
}

bool
DwarfResolver::ElfValid() const
{
	return elf != NULL;
}

void
DwarfResolver::Resolve(const FrameMap &frameMap)
{
	if (DwarfValid())
		ResolveDwarf(frameMap);
	else if (ElfValid())
		ResolveElf(frameMap);
	else
		ResolveUnmapped(frameMap);
}

void
DwarfResolver::ResolveDwarf(const FrameMap &frameMap)
{

	LOG("DwarfResolve %s", imageFile->c_str());

	CompileUnitLookup cuLookup;

	EnumerateCompileUnits(cuLookup);
	MapFramesToCompileUnits(frameMap, cuLookup);
	MapFrames(cuLookup);
}

void
DwarfResolver::EnumerateCompileUnits(CompileUnitLookup & cuLookup)
{

	try {
		auto cu(DwarfCompileUnit::GetFirstCU(dwarf));
		while (cu) {
			ProcessCompileUnit(cu, cuLookup);
			cu.AdvanceToSibling();
		}
	} catch (DwarfException &)
	{
		// swallow non-fatal dwarf exception
	}
}

void
DwarfResolver::MapFramesToCompileUnits(const FrameMap &frameMap,
    CompileUnitLookup & cuLookup)
{
	auto fit = frameMap.begin();

	while (fit != frameMap.end()) {
		try {
			TargetAddr offset = fit->second->getOffset();
			auto cit = cuLookup.Lookup(offset);
			if (cit == cuLookup.end() || !cit->second.Contains(offset)) {
				fit->second->setUnmapped();
				++fit;
				continue;
			}

			auto & lookupVal = cit->second;
			do {
				lookupVal.AddFrame(fit->second.get());
				++fit;
			} while (fit != frameMap.end() &&
			    lookupVal.Contains(fit->second->getOffset()));
		} catch (DwarfException &) {
			fit->second->setUnmapped();
			++fit;
		}
	}
}

void
DwarfResolver::MapFrames(CompileUnitLookup & cuLookup)
{
	for (auto & [addr, value] : cuLookup) {
		if (value.GetFrames().empty())
			continue;

		try {
			DwarfSearch search(dwarf, value.GetValue(), imageFile, elfSymbols);
			search.MapFrames(value.GetFrames());
		} catch (DwarfException &) {
			for (auto frame : value.GetFrames()) {
				frame->setUnmapped();
			}
		}
	}
}

void
DwarfResolver::ProcessCompileUnit(const DwarfCompileUnit & cu,
    CompileUnitLookup & cuLookup)
{
	Dwarf_Half tag;
	auto die = cu.GetDie();

	do {
		tag = GetDieTag(die->GetDie());
		if (tag == DW_TAG_compile_unit) {
			try {
				SearchCompileUnit(die, cuLookup);
			} catch (const DwarfException & msg)
			{
				// swallow it and continue.  libdwarf
				// errors are not fatal.
				LOG("DwarfException: %s", msg.what());
			}
		}

		die = die->GetSibling();
	} while (die);
}

void
DwarfResolver::SearchCompileUnit(SharedPtr<DwarfCompileUnitDie> cu,
    CompileUnitLookup & cuLookup)
{
	Dwarf_Error derr;
	Dwarf_Unsigned range_off, low_pc, high_pc;
	int error, err_lo, err_hi;

	error = dwarf_attrval_unsigned(cu->GetDie(), DW_AT_ranges, &range_off, &derr);
	if (error == DW_DLV_OK) {
		return (SearchCompileUnitRanges(cu, range_off, cuLookup));
	}

	LOG("No ranges: %s", dwarf_errmsg(derr));

	err_lo = dwarf_attrval_unsigned(cu->GetDie(), DW_AT_low_pc, &low_pc, &derr);
	err_hi = dwarf_attrval_unsigned(cu->GetDie(), DW_AT_high_pc, &high_pc, &derr);
	if (err_lo == DW_DLV_OK && err_hi == DW_DLV_OK) {
		Dwarf_Attribute high_attr;
		error = dwarf_attr(cu->GetDie(), DW_AT_high_pc, &high_attr, &derr);
		if (error == DW_DLV_OK) {
			Dwarf_Half form;
			error = dwarf_whatform(high_attr, &form, &derr);
			if (error == DW_DLV_OK) {
				switch (form) {
				case DW_FORM_data1:
				case DW_FORM_data2:
				case DW_FORM_data4:
				case DW_FORM_data8:
					high_pc = low_pc + high_pc;
					break;
				}
			}
		}
		AddCompileUnitRange(cu, low_pc, high_pc, cuLookup);
		return;
	}

	/*
	 * LLVM 3.3 seems to only describe CUs via statement lists.  I really
	 * don't know why they chose that structure, but fall back on that
	 * information if there are no ranges or lopc/hipc attributes.
	 */
	AddCompileUnitSrcLines(cu, cuLookup);

	// This likely indicates a CU with no code, so skip it
}

void
DwarfResolver::AddCompileUnitRange(SharedPtr<DwarfCompileUnitDie> cu,
    Dwarf_Unsigned low_pc, Dwarf_Unsigned high_pc, CompileUnitLookup & cuLookup)
{
	LOG("%lx+%lx: low/high pc = %lx/%lx", GetCUOffsetRange(cu->GetDie()), GetCUOffset(cu->GetDie()), low_pc, high_pc);
	cuLookup.insert(low_pc, high_pc, cu);
}

void
DwarfResolver::SearchCompileUnitRanges(SharedPtr<DwarfCompileUnitDie> cu,
    Dwarf_Unsigned range_off, CompileUnitLookup & cuLookup)
{
	Dwarf_Unsigned base_addr, low_pc, high_pc;

	DwarfRangeList rangeList(dwarf, range_off);

	base_addr = cu->GetBaseAddr();
	for (auto & range : rangeList) {
		switch (range.dwr_type) {
		case DW_RANGES_ENTRY:
			low_pc = base_addr + range.dwr_addr1;
			high_pc = base_addr + range.dwr_addr2;
			AddCompileUnitRange(cu, low_pc, high_pc, cuLookup);
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
DwarfResolver::AddCompileUnitSrcLines(SharedPtr<DwarfCompileUnitDie> cu,
    CompileUnitLookup & cuLookup)
{
	DwarfSrcLinesList srcLines(dwarf, cu->GetDie());

	auto srcIt = srcLines.begin();
	if (srcIt == srcLines.end())
		return;

	DwarfSrcLine src(*srcIt);
	Dwarf_Unsigned lo, hi;

	/*
	 * The srclines data structures aren't able to describe non-contiguous
	 * areas, so just iterate over the list to find the beginning and ending
	 * address for this CU.
	 */
	lo = src.GetAddr();
	hi = lo;
	while (srcIt != srcLines.end()) {
		DwarfSrcLine src(*srcIt);
		Dwarf_Unsigned addr = src.GetAddr();
		if (addr > hi) {
			hi = addr;
		}
		++srcIt;
	}
	AddCompileUnitRange(cu, lo, hi, cuLookup);
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
		fit->second->setUnmapped();
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
			    old_sit->second, -1, -1, 0);
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
		pair.second->setUnmapped();
}
