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

#ifndef DWARFSTACKSTATE_H
#define DWARFSTACKSTATE_H

#include "DwarfDieList.h"
#include "DwarfDieRanges.h"
#include "DwarfSubprogramInfo.h"
#include "SharedPtr.h"
#include "SharedString.h"

class DwarfCompileUnitDie;

class DwarfStackState
{
public:
	typedef DwarfDieList::const_iterator const_iterator;

private:
	DwarfDieList list;
	const_iterator iterator;
	DwarfDieRanges ranges;

public:
	DwarfStackState(Dwarf_Debug dwarf, Dwarf_Die die, const DwarfCompileUnitDie &cu);

	DwarfStackState(Dwarf_Debug dwarf, const DwarfCompileUnitDie &cu);

	DwarfStackState(DwarfStackState &&) noexcept = default;
	DwarfStackState & operator=(DwarfStackState && other) = delete;

	DwarfStackState(const DwarfStackState &) = delete;
	DwarfStackState & operator=(const DwarfStackState &) = delete;

	Dwarf_Die GetLeafDie() const
	{
		return *iterator;
	}

	DwarfDie TakeLeafDie()
	{
		return iterator.Take();
	}

	operator bool() const
	{
		return iterator != list.end();
	}

	bool Contains(TargetAddr addr) const
	{
		return ranges.Contains(addr);
	}

	bool Preceeds(TargetAddr addr) const
	{
		return ranges.Preceeds(addr);
	}

	bool Succeeds(TargetAddr addr) const
	{
		return ranges.Succeeds(addr);
	}

	bool HasRanges() const
	{
		return ranges.HasRanges();
	}

	const DwarfDieRanges & GetRanges() const
	{
		return ranges;
	}

	void Skip();

	bool Advance(TargetAddr addr);
	void Reset();
};

#endif
