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

#ifndef DWARFSEARCH_H
#define DWARFSEARCH_H

#include <libdwarf.h>

#include "DwarfSrcLinesList.h"
#include "DwarfDieStack.h"
#include "SharedString.h"

class Callframe;

class DwarfSearch
{
private:
	SharedString imageFile;
	DwarfDieStack stack;
	DwarfSrcLinesList srcLines;
	DwarfSrcLinesList::const_iterator srcIt;

	bool FindLeaf(const Callframe & frame, SharedString &file, int &line);

public:
	DwarfSearch(Dwarf_Debug, Dwarf_Die, SharedString, const SymbolMap &);

	DwarfSearch(const DwarfSearch &) = delete;
	DwarfSearch(DwarfSearch &&) = delete;

	DwarfSearch & operator=(const DwarfSearch &) = delete;
	DwarfSearch & operator=(DwarfSearch &&) = delete;

	bool AdvanceAndMap(Callframe &);
};

#endif
