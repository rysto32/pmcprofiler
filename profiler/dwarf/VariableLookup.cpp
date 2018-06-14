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

#include "VariableLookup.h"

#include "MapUtil.h"

VariableLookup::VariableLookup(Dwarf_Debug dw)
  : dwarf(dw)
{

}

void
VariableLookup::AddVariable(int reg, TargetAddr start, TargetAddr end, DwarfDieOffset typeOff)
{
	registers[reg].insert(std::make_pair(start, VariableDef(end, typeOff)));
}

DwarfDie
VariableLookup::FindRegType(int reg, TargetAddr addr)
{
	auto rit = registers.find(reg);
	if (rit == registers.end())
		return DwarfDie();

	auto vit = LastSmallerThan(rit->second, addr);
	if (vit == rit->second.end())
		return DwarfDie();

	const VariableDef & def = vit->second;
	if (addr >= def.liveEnd)
		return DwarfDie();

	return DwarfDie::OffDie(dwarf, def.typeOff);
}
