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

#ifndef PROFILER_PRINTER_H
#define PROFILER_PRINTER_H

#include "ProfilerTypes.h"
#include "StringChain.h"

#include <cstdio>
#include <cassert>
#include <functional>
#include <unordered_map>
#include <vector>

class Callchain;
class FunctionLocation;
class InlineFrame;
class Profiler;
class SharedString;
class SampleAggregation;

class ProfilePrinter
{
public:
	struct FuncLocKey
	{
		SharedString file;
		SharedString func;

		FuncLocKey(SharedString file, SharedString func);

		bool operator==(const FuncLocKey & other) const;

		bool operator!=(const FuncLocKey & other) const
		{
			return !(*this == other);
		}

		struct hasher
		{
			// XXX I would like a better hash function here
			size_t operator()(const FuncLocKey & key) const;
		};
	};

	typedef std::unordered_map<FuncLocKey, FunctionLocation, FuncLocKey::hasher> FuncLocMap;
	typedef std::unordered_map<StringChain, FuncLocMap, StringChain::Hasher> StringChainMap;

private:
	void insertFuncLoc(FuncLocMap &, const InlineFrame &, const Callchain &);

	class SampleCountComp
	{
	public:
		bool operator()(const AggCallChain &, const AggCallChain &);
	};

protected:

	typedef std::vector<FunctionLocation> FunctionLocationList;

	FILE * m_outfile;

	template <typename Strategy>
	void getFunctionLocations(const SampleAggregation &agg,
	    FunctionLocationList &list, StringChainMap *map = NULL);

	typedef std::vector<const FunctionLocation*> FuncLocPtrList;
	size_t getCallers(const StringChainMap & map, const StringChain & chain,
	    FuncLocPtrList & functions);

	static std::string getBasename(const std::string &);

	static void SortCallchains(CallchainList & list);

public:

	ProfilePrinter(FILE * file)
	  : m_outfile(file)
	{
	}

	ProfilePrinter(const ProfilePrinter&) = delete;
	ProfilePrinter& operator=(const ProfilePrinter &) = delete;

	virtual void printProfile(const Profiler & profiler,
	    const AggregationList & aggList) = 0;

	virtual ~ProfilePrinter()
	{
		if (m_outfile != stdout)
			fclose(m_outfile);
	}

	void printLineNumbers(const Profiler & profiler, const LineLocationList & functionLocation);

};

class FlatProfilePrinter : public ProfilePrinter
{
public:
	FlatProfilePrinter(FILE * file)
	  : ProfilePrinter(file)
	{
	}

	virtual void printProfile(const Profiler & profiler,
	    const AggregationList & aggList);
};

struct LeafProcessStrategy
{
	typedef std::vector<const InlineFrame*>::const_iterator iterator;

	iterator begin(std::vector<const InlineFrame*> & vec) const
	{
		return vec.begin();
	}

	iterator end(std::vector<const InlineFrame*> & vec) const
	{
		return vec.end();
	}

	void insertSelfFrame(std::vector<const InlineFrame*> &, Callchain &, const InlineFrame &) const
	{
	}
};

struct RootProcessStrategy
{
	typedef std::vector<const InlineFrame*>::const_reverse_iterator iterator;

	iterator begin(std::vector<const InlineFrame*> & vec) const;
	iterator end(std::vector<const InlineFrame*> & vec) const;
	void insertSelfFrame(std::vector<const InlineFrame*> &, Callchain &, const InlineFrame &) const;
};

#endif

