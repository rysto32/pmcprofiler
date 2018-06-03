// Copyright (c) 2009-2014 Sandvine Incorporated.  All rights reserved.
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

#ifndef FUNCTIONLOCATION_H
#define FUNCTIONLOCATION_H

#include "Callchain.h"
#include "InlineFrame.h"
#include "ProfilerTypes.h"

#include <set>

class FunctionLocation
{
	// XXX this is a pointer to allow this class to be sortable
	const InlineFrame *frame;
	LineLocationList lineLocationList;
	size_t totalCount;
	bool kernel;

public:
	FunctionLocation(const InlineFrame& f, const Callchain &callchain)
	  : frame(&f),
	    totalCount(0),
	    kernel(callchain.isKernel())
	{
		AddSample(f, callchain.getSampleCount());
	}

	FunctionLocation(FunctionLocation && other) noexcept = default;
	FunctionLocation& operator=(FunctionLocation &&other) noexcept = default;

	FunctionLocation(const FunctionLocation&) = delete;
	FunctionLocation& operator=(const FunctionLocation &) = delete;

	void AddSample(const InlineFrame &f, size_t count)
	{
		totalCount += count;
		lineLocationList.insert(f.getCodeLine());
	}

	bool operator<(const FunctionLocation& other) const
	{
		return totalCount > other.totalCount;
	}

	size_t getCount() const
	{
		return totalCount;
	}

	const LineLocationList& getLineLocationList() const
	{
		return lineLocationList;
	}

	const InlineFrame & getFrame() const
	{
		return *frame;
	}

	bool isKernel() const
	{
		return kernel;
	}
};

struct FuncLocPtrComp
{
	bool operator()(const FunctionLocation *a, const FunctionLocation *b) const
	{
		return *a < *b;
	}
};

#endif
