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

#ifndef DWARFRESOLVER_H
#define DWARFRESOLVER_H

#include "ProfilerTypes.h"
#include "SharedString.h"

#include <map>
#include <memory>
#include <string>

#include <libdwarf.h>
#include <libelf.h>

class DwarfResolver
{
private:
	SharedString imageFile;
	SharedString symbolFile;
	SharedString symbolFilePath;

	Elf *elf;
	Dwarf_Debug dwarf;

	SymbolMap elfSymbols;

	Elf * GetSymbolFile();
	bool HaveSymbolFile(Elf *origElf);
	Elf * OpenSymbolFile(Elf *origElf);
	int FindSymbolFile();
	void ParseDebuglink(Elf_Scn *section);

	bool DwarfValid() const;
	bool ElfValid() const;

	void ResolveDwarf(const FrameMap &frames) const;
	void ResolveElf(const FrameMap &frames) const;
	void ResolveUnmapped(const FrameMap &frames) const;

	void FillElfSymbolMap(Elf *imageElf, Elf_Scn *section);

	void ProcessCompileUnit(Dwarf_Die die, FrameMap::const_iterator &fit,
	    const FrameMap::const_iterator &end) const;
	void SearchCompileUnit(Dwarf_Die die, FrameMap::const_iterator &fit,
	    const FrameMap::const_iterator &end) const;
	void TryCompileUnitRange(Dwarf_Die die, Dwarf_Unsigned low_pc,
	    Dwarf_Unsigned high_pc, FrameMap::const_iterator &fit,
	    const FrameMap::const_iterator &end) const;
	void SearchCompileUnitRanges(Dwarf_Die die, Dwarf_Unsigned range_off,
	    FrameMap::const_iterator &fit, const FrameMap::const_iterator &end) const;
	void SearchCompileUnitFuncs(Dwarf_Die parent, FrameMap::const_iterator &fit,
	    const FrameMap::const_iterator &end) const;

public:
	explicit DwarfResolver(SharedString image);
	~DwarfResolver();

	DwarfResolver(const DwarfResolver &) = delete;
	DwarfResolver(DwarfResolver &&) = delete;
	DwarfResolver & operator=(const DwarfResolver &) = delete;
	DwarfResolver & operator=(DwarfResolver &&) = delete;

	void Resolve(const FrameMap &frames) const;
};

#endif
