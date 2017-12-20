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

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include "ProfilePrinter.h"

#include "AddressSpace.h"
#include "Callchain.h"
#include "FunctionLocation.h"
#include "InlineFrame.h"
#include "Profiler.h"
#include "SampleAggregation.h"
#include "SharedString.h"

#include <paths.h>
#include <libgen.h>

#include <cassert>
#include <functional>
#include <unordered_map>

ProfilePrinter::FuncLocKey::FuncLocKey(SharedString file, SharedString func)
: file(file), func(func)
{
}

bool
ProfilePrinter::FuncLocKey::operator==(const FuncLocKey & other) const
{
	return file == other.file && func == other.func;
}

// XXX I would like a better hash function here
size_t
ProfilePrinter::FuncLocKey::hasher::operator()(const FuncLocKey & key) const
{
	size_t hash = std::hash<std::string>{}(*key.file);
	hash *= 5;
	hash += std::hash<std::string>{}(*key.func);

	return hash;
}

void
ProfilePrinter::printLineNumbers(const Profiler & profiler, const LineLocationList& lineLocationList)
{
	if (profiler.showLines()) {
		fprintf(m_outfile, " lines:");
		for (LineLocationList::const_iterator it = lineLocationList.begin();
		     it != lineLocationList.end(); ++it)
			     fprintf(m_outfile, " %u", *it);
	}
}

std::string
ProfilePrinter::getBasename(const std::string&file)
{
	auto it = file.find_last_of('/');
	if (it != std::string::npos)
		return file.substr(it + 1);
	else
		return file;
}

void
ProfilePrinter::insertFuncLoc(FuncLocMap &locMap, const InlineFrame &frame, const Callchain &chain)
{
	FuncLocKey key(frame.getFile(), frame.getFunc());
	auto insertPos = locMap.insert(std::make_pair(key,
	    FunctionLocation(frame, chain)));
	if (!insertPos.second)
		insertPos.first->second.AddSample(frame, chain.getSampleCount());
}

template <typename Strategy>
void
ProfilePrinter::getFunctionLocations(const SampleAggregation &agg,
    FunctionLocationList &list)
{
	CallchainList callchainList;
	agg.getCallchainList(callchainList);

	size_t mapSize = 4 * callchainList.size() / 3;
	FuncLocMap locMap(mapSize);
	StringChainMap chainMap(mapSize);

	Strategy strategy;

	for (auto chain : callchainList) {
		std::vector<const InlineFrame*> frameList;
		chain->flatten(frameList);

		auto jt = strategy.begin(frameList);
		auto jt_end = strategy.end(frameList);

		if (jt == jt_end)
			continue;

		insertFuncLoc(locMap, **jt, *chain);

		StringChain strChain;
		strChain.push_back((*jt)->getDemangled());

		for (++jt; jt != jt_end; ++jt) {
			const auto & frame = **jt;

			auto insert = chainMap.insert(std::make_pair(strChain, FuncLocMap(1)));
			insertFuncLoc(insert.first->second, frame, *chain);

			strChain.push_back(frame.getDemangled());
		}

		strategy.processEnd(frameList, chainMap, strChain);
	}

	for (auto & pair : locMap) {
		list.emplace_back(std::move(pair.second));
	}

	std::sort(list.begin(), list.end());
}

void
FlatProfilePrinter::printProfile(const Profiler & profiler,
				 const AggregationList & aggList)
{
	fprintf(m_outfile, "Events processed: %u\n\n", profiler.getSampleCount());

	CallchainList callchainList;
	for (auto agg : aggList)
		agg->getCallchainList(callchainList);

	SortCallchains(callchainList);

	unsigned cumulative = 0;
	for (auto chain : callchainList) {

		const auto & agg = chain->getAggregation();
		const auto & space = chain->getAddressSpace();
		auto frame = chain->front();

		cumulative += chain->getSampleCount();
		fprintf(m_outfile, "%6.2f%% %6.2f%% %s, %6u, %10s, %6u, 0x%08lx, %s, %s, %s:%u %s\n",
			(chain->getSampleCount() * 100.0) / profiler.getSampleCount(),
			(cumulative * 100.0) / profiler.getSampleCount(),
			chain->isKernel() ? "kern" : "user",
			agg.getPid(),
			getBasename(*space.getExecutableName()).c_str(),
			chain->getSampleCount(),
			chain->getAddress(),
			chain->isMapped() ? "mapped  " : "unmapped",
			space.getExecutableName()->c_str(),
			frame.getFile()->c_str(),
			frame.getCodeLine(),
			frame.getDemangled()->c_str());
	}

	for (auto agg : aggList) {
		fprintf(m_outfile, "\nProcess: %6u, %s, total: %zu (%6.2f%%)\n", agg->getPid(),
		    agg->getExecutable().c_str(), agg->getSampleCount(),
		    (agg->getSampleCount() * 100.0) / profiler.getSampleCount());

		     FunctionLocationList functionList;
		     getFunctionLocations<LeafProcessStrategy>(*agg, functionList);

		     unsigned cumulativeCount = 0;
		     fprintf(m_outfile, "       time   time-t   samples   env  file / library, line number, function\n");
		     for (const auto & functionLocation : functionList) {
			    cumulativeCount += functionLocation.getCount();
			    const InlineFrame & frame = functionLocation.getFrame();
			     fprintf(m_outfile, "    %6.2f%%, %6.2f%%, %8u, %s, %s:%u, %s",
				     (functionLocation.getCount() * 100.0) / agg->getSampleCount(),
				     (cumulativeCount * 100.0) / agg->getSampleCount(),
				     functionLocation.getCount(),
				     functionLocation.isKernel() ? "kern" : "user",
				     frame.getFile()->c_str(),
				     frame.getFuncLine(),
				     frame.getDemangled()->c_str());
			     printLineNumbers(profiler, functionLocation.getLineLocationList());
			     fprintf(m_outfile, "\n");
		     }
	     }
}

