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

#include "CallchainProfilePrinter.h"

#include "FunctionLocation.h"
#include "Profiler.h"
#include "SampleAggregation.h"
#include "StringChain.h"

#include <cstring>

template <class ProcessStrategy, class PrintStrategy>
bool
CallchainProfilePrinter<ProcessStrategy, PrintStrategy>::isCallChainBoring(
    const SampleAggregation &agg, StringChain & chain, const StringChainMap &chainMap)
{
	FuncLocPtrList functions;
	getCallers(chainMap, chain, functions);

	if (functions.size() > 1)
		return false;
	else if (functions.size() == 0)
		return true;
	else {
		const InlineFrame & frame = functions.front()->getFrame();
		chain.push_back(frame);
		bool boring = isCallChainBoring(agg, chain, chainMap);
		assert(chain.back() == frame.getDemangled());
		chain.pop_back();
		return boring;
	}
}

template <class ProcessStrategy, class PrintStrategy>
void
CallchainProfilePrinter<ProcessStrategy, PrintStrategy>::printCallChain(
    const Profiler & profiler, const SampleAggregation &agg, StringChain & chain,
    uint32_t depth, PrintStrategy &strategy, const StringChainMap &chainMap)
{
	FuncLocPtrList functions;
	size_t total_samples = getCallers(chainMap, chain, functions);

	bool isBoring = false;
	if (!printBoring && functions.size() == 1)
		isBoring = isCallChainBoring(agg, chain, chainMap);

	for (auto funcLoc : functions) {
		double parent_percent = (funcLoc->getCount() * 100.0) / total_samples;
		double total_percent = (funcLoc->getCount() * 100.0) / agg.getSampleCount();

		if (total_percent < threshold)
			continue;

		const InlineFrame & frame = funcLoc->getFrame();

		strategy.printFrame(m_outfile, depth, parent_percent, total_percent,
		    *this, profiler, *funcLoc, agg,
		    frame.getDemangled()->c_str(), chain);

		if (!isBoring) {
			chain.push_back(frame);
			printCallChain(profiler, agg, chain, depth + 1, strategy, chainMap);
			assert(chain.back() == frame.getDemangled());
			chain.pop_back();
		}
	}
}

template <class ProcessStrategy, class PrintStrategy>
void
CallchainProfilePrinter<ProcessStrategy, PrintStrategy>::printProfile(const Profiler & profiler,
    const AggregationList & aggList)
{
	PrintStrategy strategy;
	strategy.printFileHeader(m_outfile, profiler);

	for (const auto & agg : aggList) {

		FunctionLocationList functionList;
		StringChainMap chainMap;
		getFunctionLocations<ProcessStrategy>(*agg, functionList, &chainMap);

		strategy.printProcessHeader(m_outfile, profiler, *agg);
		for (const auto & functionLocation : functionList) {
			double percent = (functionLocation.getCount() * 100.0) / agg->getSampleCount();
			if (percent >= threshold) {
				const InlineFrame & frame = functionLocation.getFrame();
				StringChain chain;

				strategy.printFrame(m_outfile, 0, percent, percent, *this, profiler, functionLocation,
						    *agg, frame.getDemangled()->c_str(), chain);

				chain.push_back(frame);
				printCallChain(profiler, *agg, chain, 1, strategy, chainMap);
			}
		}
	}
}

void
PrintCallchainStrategy::printFileHeader(FILE *outfile, const Profiler &profiler) const
{
	fprintf(outfile, "Events processed: %u\n", profiler.getSampleCount());
}

void
PrintCallchainStrategy::printProcessHeader(FILE *outfile, const Profiler &profiler, const SampleAggregation &agg) const
{
	fprintf(outfile, "\nProcess: %6u, %s, total: %zu (%6.2f%%)\n", agg.getPid(), agg.getExecutable().c_str(),
		agg.getSampleCount(), (agg.getSampleCount() * 100.0) / profiler.getSampleCount());
}

void
PrintCallchainStrategy::printFrame(FILE *outfile, uint32_t depth, double processPercent, double parentPercent,
		ProfilePrinter &printer, const Profiler &profiler, const FunctionLocation& functionLocation,
		const SampleAggregation &agg, const char *functionName, StringChain & chain __unused) const
{
	for (uint32_t i = 0; i < depth; i++)
		fprintf(outfile, "  ");

	fprintf(outfile, "[%d] %.2f%% %.2f%%(%zd/%zd) %s %s %lx", depth, parentPercent, processPercent,
		functionLocation.getCount(), agg.getSampleCount(), functionName,
		functionLocation.getFrame().getImageName()->c_str(),
		functionLocation.getFrame().getOffset());
	printer.printLineNumbers(profiler, functionLocation.getLineLocationList());
	fprintf(outfile, "\n");
}

void
PrintFlameGraphStrategy::printFileHeader(FILE *outfile __unused, const Profiler &profiler __unused) const
{
}

void
PrintFlameGraphStrategy::printProcessHeader(FILE *outfile __unused, const Profiler &profiler __unused, const SampleAggregation &agg __unused) const
{
}

void
PrintFlameGraphStrategy::printFrame(FILE *outfile, uint32_t depth, double processPercent, double parentPercent,
		ProfilePrinter &printer, const Profiler &profiler, const FunctionLocation& functionLocation,
		const SampleAggregation &agg, const char *functionName, StringChain & chain) const
{
	if (strcmp(functionName, "[self]") == 0) {
		const char *sep = "";
		for (auto demangled : chain) {
			fprintf(outfile, "%s%s", sep, demangled->c_str());
			sep = ";";
		}

		fprintf(outfile, " %zd\n", functionLocation.getCount());
	}
}
