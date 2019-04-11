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

#ifndef PROFILERTYPES_H
#define PROFILERTYPES_H

#include <stdint.h>
#include <stdio.h>

#include <map>
#include <memory>
#include <set>
#include <vector>

typedef uintptr_t TargetAddr;

extern bool g_includeTemplates;
extern bool g_quitOnError;

extern uint32_t g_filterFlags;
const uint32_t PROFILE_USER = 0x0001;
const uint32_t PROFILE_KERN = 0x0002;

extern size_t noDwarf, noText, disassembleFailed, findRegFailed, findVarFailed;

class Callchain;
class Callframe;
class SampleAggregation;
class SharedString;

struct AggCallChain
{
	const SampleAggregation *agg;
	Callchain *chain;

	AggCallChain(const SampleAggregation * a, Callchain * c)
	  : agg(a), chain(c)
	{
	}

	bool operator==(const AggCallChain & other) const
	{
		return agg == other.agg && chain == other.chain;
	}
};

typedef std::vector<SampleAggregation*> AggregationList;
typedef std::vector<AggCallChain> CallchainList;
typedef std::map<TargetAddr, std::unique_ptr<Callframe> > FrameMap;
typedef std::set<unsigned> LineLocationList;
typedef std::map<TargetAddr, SharedString> SymbolMap;

#ifdef LOG_ENABLED

#define LOG(args...) \
	fprintf(stderr, args)

#else

#define LOG(args...)

#endif

#endif
