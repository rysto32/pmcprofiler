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

#include "DwarfStackState.h"

#include "DwarfUtil.h"
#include "Image.h"

DwarfStackState::DwarfStackState(Dwarf_Debug dwarf, Dwarf_Die die,
    const DwarfCompileUnitDie &cu)
  : list(dwarf, die),
    iterator(list.begin()),
    ranges(dwarf, cu)
{
// 	fprintf(stderr, "Using funcInfo %p for die %lx tag %d\n",
// 	    funcInfo.get(), GetDieOffset(die), GetDieTag(die));
	if (iterator != list.end())
		ranges.Reinit(*iterator);
}

DwarfStackState::DwarfStackState(Dwarf_Debug dwarf, const DwarfCompileUnitDie &cu)
  : list(dwarf),
    iterator(list.end()),
    ranges(dwarf, cu)
{
}
/*
DwarfStackState::DwarfStackState(DwarfStackState &&other) noexcept
  : list(std::move(other.list)),
    iterator(std::move(other.iterator)),
    ranges(std::move(other.ranges)),
    funcInfo(std::move(other.funcInfo))
{
	if (iterator != list.end()) {
		const Dwarf_Die & die = *iterator;
		fprintf(stderr, "Took funcInfo %p in move constructor for die %lx tag %d\n",
		funcInfo.get(), GetDieOffset(die), GetDieTag(die));
	}
}

DwarfStackState &
DwarfStackState::operator=(DwarfStackState && other)
{
	list = std::move(other.list);
	iterator = std::move(other.iterator);
	ranges = std::move(other.ranges);
	funcInfo = std::move(other.funcInfo);

	if (iterator != list.end()) {
		const Dwarf_Die & die = *iterator;
		fprintf(stderr, "Took funcInfo %p in assignment for die %lx tag %d\n",
		funcInfo.get(), GetDieOffset(die), GetDieTag(die));
	}

	return *this;
}*/

void DwarfStackState::Skip()
{
// 	const Dwarf_Die & die = *iterator;
// 	fprintf(stderr, "Skip die %lx tag %d\n",
// 	    GetDieOffset(die), GetDieTag(die));
	++iterator;
	if (*this)
		ranges.Reinit(GetLeafDie());
}

bool
DwarfStackState::Advance(TargetAddr addr)
{
	bool advanced = false;
	while (!ranges.Contains(addr)) {
		advanced = true;
		++iterator;
		if (iterator == list.end())
			break;

		ranges.Reinit(*iterator);
	}

	return advanced;
}

void
DwarfStackState::Reset()
{
	iterator = list.end();
	ranges.Reset();
}

