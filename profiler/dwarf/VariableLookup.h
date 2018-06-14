// Copyright (c) 2018 Ryan Stone.
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

#ifndef VARIABLELOOKUP_H
#define VARIABLELOOKUP_H

#include <libdwarf.h>
#include <map>
#include <unordered_map>

#include "DwarfDie.h"
#include "DwarfUtil.h"
#include "ProfilerTypes.h"

class VariableLookup
{
	struct VariableDef {
		TargetAddr liveEnd;
		DwarfDieOffset typeOff;

		VariableDef(TargetAddr l, DwarfDieOffset o)
		  : liveEnd(l), typeOff(o)
		{
		}
	};

	typedef std::map<TargetAddr, VariableDef> LiveRangeMap;
	typedef std::unordered_map<int, LiveRangeMap> RegisterMap;

	Dwarf_Debug dwarf;
	RegisterMap registers;

public:
	VariableLookup(Dwarf_Debug);

	void AddVariable(int reg, TargetAddr start, TargetAddr end, DwarfDieOffset typeOff);
	DwarfDie FindRegType(int reg, TargetAddr addr);
};

#endif // VARIABLELOOKUP_H
