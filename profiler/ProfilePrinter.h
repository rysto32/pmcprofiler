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

protected:

	typedef std::vector<FunctionLocation> FunctionLocationList;

	FILE * m_outfile;
	uint32_t m_maxDepth;

	template <typename Strategy>
	void getFunctionLocations(const SampleAggregation &agg, FunctionLocationList &list, StringChainMap *map = NULL);

	typedef std::vector<const FunctionLocation*> FuncLocPtrList;
	size_t getCallers(const StringChainMap & map, const StringChain & chain,
	    FuncLocPtrList & functions);

	static std::string getBasename(const std::string &);

public:

	ProfilePrinter(FILE * file, uint32_t maximumDepth)
	  : m_outfile(file),
	m_maxDepth(maximumDepth)
	{
	}

	ProfilePrinter(const ProfilePrinter&) = delete;
	ProfilePrinter& operator=(const ProfilePrinter &) = delete;

	uint32_t getMaxDepth() const
	{
		return m_maxDepth;
	}

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
	  : ProfilePrinter(file, 1)
	{
	}

	virtual void printProfile(const Profiler & profiler,
	    const AggregationList & aggList);
};

#if 0
struct PrintFlameGraphStrategy
{
	void printFileHeader(FILE *outfile __unused, const Profiler &profiler __unused)
	{
	}

	void printProcessHeader(FILE *outfile __unused, const Profiler &profiler __unused, Process &process __unused)
	{
	}

	void printFrame(FILE *outfile, int depth __unused, double processPercent __unused, double parentPercent __unused,
			ProfilePrinter &printer __unused, const Profiler &profiler __unused, FunctionLocation& functionLocation,
		 Process &process __unused, const char *functionName, StringChain & chain)
	{
		if (strcmp(functionName, "[self]") == 0) {
			const char *sep = "";
			for (StringChain::iterator it = chain.begin(); it != chain.end(); ++it) {
				char *demangled = Image::demangle(**it);
				fprintf(outfile, "%s%s", sep, demangled);
				free(demangled);
				sep = ";";
			}

			fprintf(outfile, " %d\n", functionLocation.getCount());
		}
	}
};
#endif

struct LeafProcessStrategy
{
	typedef std::vector<const InlineFrame*>::iterator iterator;

	iterator begin(std::vector<const InlineFrame*> & vec)
	{
		return vec.begin();
	}

	iterator end(std::vector<const InlineFrame*> & vec)
	{
		return vec.end();
	}

	void processEnd(std::vector<const InlineFrame*> & vec __unused, ProfilePrinter::StringChainMap * callchainMap __unused, const StringChain & callchain __unused)
	{
	}
};

#if 0
struct RootProcessStrategy
{
	typedef std::vector<Location>::reverse_iterator iterator;

	iterator begin(std::vector<Location> & vec)
	{
		iterator it = vec.rbegin();

		while (it != end(vec) && !it->isMapped()) {
			++it;
		}

		return it;
	}

	iterator end(std::vector<Location> & vec)
	{
		return vec.rend();
	}

	void processEnd(std::vector<Location> & vec, CallchainMap & callchainMap, const StringChain & callchain)
	{
		std::pair<CallchainMap::iterator, bool> mapInserted =
		callchainMap.insert(CallchainMap::value_type(callchain, FunctionLocationMap(1)));
		std::pair<FunctionLocationMap::iterator, bool> inserted =
		mapInserted.first->second.insert(FunctionLocationMap::value_type("[self]", FunctionLocation(vec.front())));
		if (!inserted.second)
			inserted.first->second += vec.front();
		else
			inserted.first->second.setFunctionName("[self]");
	}
};
#endif

#endif

