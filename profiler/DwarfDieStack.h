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

#ifndef DWARFDIESTACK_H
#define DWARFDIESTACK_H

#include "DwarfDieList.h"
#include "DwarfLocation.h"
#include "DwarfRangeLookup.h"
#include "DwarfStackState.h"
#include "ProfilerTypes.h"

#include <libdwarf.h>
#include <vector>

class Callframe;
class DwarfCompileUnitDie;

class DwarfDieStack
{
private:
	SharedString imageFile;
	Dwarf_Debug dwarf;
	Dwarf_Die topDie;
	std::vector<DwarfStackState> dieStack;
	const DwarfCompileUnitDie &cu;

	SharedString GetCallFile(Dwarf_Die die);
	int GetCallLine(Dwarf_Die die);

	void AddSubprogramSymbol(DwarfLocationList &list, const DwarfDieRanges &);
	void AddInlineSymbol(DwarfLocationList &list, Dwarf_Die die);

public:
	DwarfDieStack(SharedString imageFile, Dwarf_Debug dwarf,
	    const DwarfCompileUnitDie &cu, Dwarf_Die die);

	DwarfDieStack(const DwarfDieStack &) = delete;
	DwarfDieStack(DwarfDieStack &&) = delete;
	DwarfDieStack & operator=(const DwarfDieStack &) = delete;
	DwarfDieStack & operator=(DwarfDieStack &&) = delete;

	void EnumerateSubprograms(DwarfRangeLookup<DwarfDie> &);
	void FillSubprogramSymbols(DwarfLocationList &, const DwarfDieRanges &);
};

#endif
