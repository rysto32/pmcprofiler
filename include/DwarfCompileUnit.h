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

#ifndef DWARFCOMPILEUNIT_H
#define DWARFCOMPILEUNIT_H

#include <libdwarf.h>

#include <memory>

#include "DwarfCompileUnitParams.h"
#include "DwarfDie.h"
#include "ProfilerTypes.h"
#include "SharedPtr.h"
#include "SharedString.h"

class DwarfCompileUnitDie;

class DwarfCompileUnit
{
private:
	Dwarf_Debug dwarf;
	SharedPtr<DwarfCompileUnitParams> params;
	Dwarf_Bool is_info;
	bool complete;

	DwarfCompileUnit(Dwarf_Debug dwarf, Dwarf_Bool is_info);

public:
	DwarfCompileUnit() = delete;
	DwarfCompileUnit(const DwarfCompileUnit &) = delete;
	DwarfCompileUnit(DwarfCompileUnit &&) = default;

	DwarfCompileUnit & operator=(const DwarfCompileUnit &) = delete;
	DwarfCompileUnit & operator=(DwarfCompileUnit &&) = default;

	static DwarfCompileUnit GetFirstCU(Dwarf_Debug, Dwarf_Bool is_info = 1);

	void AdvanceToSibling();

	operator bool() const
	{
		return !complete;
	}

	bool operator!() const
	{
		return !this->operator bool();
	}

	SharedPtr<DwarfCompileUnitDie> GetDie() const;
};

#endif
