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

#ifndef DWARFDIERANGES_H
#define DWARFDIERANGES_H

#include <libdwarf.h>

#include <vector>

#include "ProfilerTypes.h"

class DwarfCompileUnitDie;

class DwarfDieRanges
{
public:
	struct Range
	{
		TargetAddr low;
		TargetAddr high;

		Range(TargetAddr l, TargetAddr h)
		  : low(l), high(h)
		{
		}

		bool Contains(TargetAddr a) const
		{
			return low <= a && a < high;
		}

		// It is guaranteed that no ranges in a given range list will
		// overlap, so we don't need to compare high as well.
		bool operator<(const Range & other) const
		{
			return low < other.low;
		}
	};

private:
	Dwarf_Debug dwarf;
	std::vector<Range> ranges;
	const DwarfCompileUnitDie & compileUnit;
	Dwarf_Die die;

	void InitFromRanges(Dwarf_Die, Dwarf_Unsigned);
	void AddRange(TargetAddr low, TargetAddr high);

	int GetHighPc(Dwarf_Die, Dwarf_Unsigned lopc, Dwarf_Unsigned &hipc);

public:
	DwarfDieRanges(Dwarf_Debug dwarf, const DwarfCompileUnitDie &);
	DwarfDieRanges(Dwarf_Debug dwarf, Dwarf_Die die, const DwarfCompileUnitDie &);

	DwarfDieRanges(DwarfDieRanges &&) noexcept = default;
	DwarfDieRanges & operator=(DwarfDieRanges &&) = delete;

	DwarfDieRanges(const DwarfDieRanges &) = delete;
	DwarfDieRanges & operator=(const DwarfDieRanges &) = delete;

	static std::optional<Dwarf_Unsigned> LookupRangesOffset(Dwarf_Die die, Dwarf_Error * derr);

	void Reinit(Dwarf_Die die);
	void Reset();

	bool Contains(TargetAddr a) const;
	bool Preceeds(TargetAddr a) const;
	bool Succeeds(TargetAddr a) const;

	bool HasRanges() const
	{
		return !ranges.empty();
	}

	std::vector<Range>::const_iterator begin() const
	{
		return ranges.begin();
	}

	std::vector<Range>::const_iterator end() const
	{
		return ranges.end();
	}
};

#endif
