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

#ifndef DWARFFUNCINFO_H
#define DWARFFUNCINFO_H

#include <libdwarf.h>

#include "SharedString.h"

class DwarfSubprogramInfo
{
	Dwarf_Debug dwarf;
	Dwarf_Die die;
	SharedString func;
	SharedString demangled;
	int line;
	bool inited;

	void CheckInitialized();

	void InitFromAbstractOrigin(Dwarf_Attribute);
	void InitFromSpecification(Dwarf_Die);
	void InitFromLocalAttr(Dwarf_Die);

	void SetFunc(SharedString f, int l);

public:
	DwarfSubprogramInfo(Dwarf_Debug dwarf, Dwarf_Die die)
	  : dwarf(dwarf), die(die), inited(false)
	{
	}

	DwarfSubprogramInfo(const DwarfSubprogramInfo &) = delete;
	DwarfSubprogramInfo(DwarfSubprogramInfo &&) = delete;
	DwarfSubprogramInfo & operator=(const DwarfSubprogramInfo &) = delete;
	DwarfSubprogramInfo & operator=(DwarfSubprogramInfo &&) = delete;

	SharedString GetFunc();
	SharedString GetDemangled();
	int GetLine();
};

#endif
