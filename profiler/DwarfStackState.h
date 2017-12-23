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

class DwarfStackState
{
public:
	typedef DwarfDieList::const_iterator const_iterator;

private:
	DwarfDieList list;
	const_iterator iterator;
	DwarfDieRanges ranges;

	SharedPtr<DwarfSubprogramInfo> funcInfo;

public:
	DwarfStackState(Dwarf_Debug dwarf, Dwarf_Die die,
	    SharedPtr<DwarfSubprogramInfo> funcInfo);

	DwarfStackState(Dwarf_Debug dwarf);

	DwarfStackState(DwarfStackState &&) noexcept = default;
	DwarfStackState & operator=(DwarfStackState && other) = default;

	DwarfStackState(const DwarfStackState &) = delete;
	DwarfStackState & operator=(const DwarfStackState &) = delete;

	SharedPtr<DwarfSubprogramInfo> GetFuncInfo() const
	{
		return funcInfo;
	}

	SharedString GetFunc()
	{
		return funcInfo->GetFunc();
	}

	SharedString GetDemangled()
	{
		return funcInfo->GetDemangled();
	}

	int GetFuncLine()
	{
		return funcInfo->GetLine();
	}

	const Dwarf_Die & GetLeafDie()
	{
		return *iterator;
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

	void Skip();

	bool Advance(TargetAddr addr);
	void Reset();
};

#endif
