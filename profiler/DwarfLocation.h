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

#ifndef DWARFLOCATION_H
#define DWARFLOCATION_H

#include "DwarfSrcLine.h"
#include "DwarfUtil.h"
#include "MapUtil.h"
#include "ProfilerTypes.h"
#include "SharedPtr.h"
#include "SharedString.h"

#include <libdwarf.h>

class DwarfLocation;
typedef SharedPtr<DwarfLocation> DwarfLocationPtr;

class DwarfLocation
{
private:
	TargetAddr start;
	TargetAddr end;
	SharedString file;
	SharedString callee;
	DwarfLocationPtr caller;
	DwarfDieOffset dieOffset;
	int codeLine;
	int funcLine;

public:
	DwarfLocation(TargetAddr s, TargetAddr e, SharedString f, int l,
	    SharedString callee, DwarfDieOffset dieOffset, DwarfLocationPtr c)
	  : start(s),
	    end(e),
	    file(f),
	    callee(callee),
	    caller(c),
	    dieOffset(dieOffset),
	    codeLine(l),
	    funcLine(c->funcLine)
	{
	}

	DwarfLocation(TargetAddr s, TargetAddr e, SharedString funcName, int l)
	  : start(s), end(e), callee(funcName), funcLine(l)
	{
	}

	DwarfLocation(DwarfLocation &&) = default;
	DwarfLocation &operator=(DwarfLocation &&) = default;

	DwarfLocation(const DwarfLocation &) = delete;
	DwarfLocation &operator=(const DwarfLocation &) = delete;

	TargetAddr GetStart() const
	{
		return start;
	}

	TargetAddr GetEnd() const
	{
		return end;
	}

	SharedString GetFile() const
	{
		return file;
	}

	SharedString GetCallee() const
	{
		return callee;
	}

	DwarfLocationPtr GetCaller() const
	{
		return caller;
	}

	int GetCodeLine() const
	{
		return codeLine;
	}

	int GetFuncLine() const
	{
		return funcLine;
	}

	SharedString GetFunc() const
	{
		return caller->GetCallee();
	}

	DwarfDieOffset GetDwarfDieOffset() const
	{
		return dieOffset;
	}
};

typedef std::map<TargetAddr, DwarfLocationPtr> DwarfLocationList;

template <typename... Args>
void AddDwarfSymbol(DwarfLocationList & list, TargetAddr low, TargetAddr high, Args&&... args)
{
	auto it = LastSmallerThan(list, low);
	assert (it != list.end());
	DwarfLocationPtr current = it->second;

	auto ptr = DwarfLocationPtr::make(low, high,
		args..., current);

	LOG("Add symbol %p covering %lx-%lx for %lx, current is %p\n", ptr.get(),
	    low, high, ptr->GetDwarfDieOffset(), current.get());
	list[low] = ptr;
	if (high != 0 && current->GetEnd() > high)
		list.insert({high, current});
}

#endif
