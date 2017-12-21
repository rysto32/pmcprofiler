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

#ifndef CALLCHAINPROFILEPRINTER_H
#define CALLCHAINPROFILEPRINTER_H

#include "ProfilePrinter.h"

template <class ProcessStrategy, class PrintStrategy>
class CallchainProfilePrinter : public ProfilePrinter
{
	int threshold;
	bool printBoring;

	void printCallChain(const Profiler & profiler, const SampleAggregation& agg,
	    StringChain & chain, int depth, PrintStrategy &strategy,
	    const StringChainMap &chainMap);
	bool isCallChainBoring(const SampleAggregation& agg, StringChain & chain,
	    const StringChainMap &chainMap);

public:
	CallchainProfilePrinter(FILE * file, uint32_t maximumDepth, int threshold, bool printBoring)
	  : ProfilePrinter(file, maximumDepth),
	    threshold(threshold),
	    printBoring(printBoring)
	{
	}

	virtual void printProfile(const Profiler & profiler,
	    const AggregationList & aggList);
};


struct PrintCallchainStrategy
{
	void printFileHeader(FILE *outfile, const Profiler &profiler) const;
	void printProcessHeader(FILE *outfile, const Profiler &profiler, const SampleAggregation &agg) const;
	void printFrame(FILE *outfile, int depth, double processPercent, double parentPercent,
	    ProfilePrinter &printer, const Profiler &profiler, const FunctionLocation& functionLocation,
	    const SampleAggregation &agg, const char *functionName, StringChain & chain) const;
};


struct PrintFlameGraphStrategy
{
	void printFileHeader(FILE *outfile, const Profiler &profiler) const;
	void printProcessHeader(FILE *outfile, const Profiler &profiler, const SampleAggregation &agg) const;
	void printFrame(FILE *outfile, int depth, double processPercent, double parentPercent,
		ProfilePrinter &printer, const Profiler &profiler, const FunctionLocation& functionLocation,
		const SampleAggregation &agg, const char *functionName, StringChain & chain) const;
};

#define DEFINE_PRINTER(procStra, printStra, name) \
    template class CallchainProfilePrinter<procStra, printStra>; \
    typedef CallchainProfilePrinter<procStra, printStra> name

DEFINE_PRINTER(LeafProcessStrategy, PrintCallchainStrategy, LeafProfilePrinter);
DEFINE_PRINTER(RootProcessStrategy, PrintCallchainStrategy, RootProfilePrinter);
DEFINE_PRINTER(RootProcessStrategy, PrintFlameGraphStrategy, FlameGraphProfilerPrinter);

#undef DEFINE_PRINTER

#endif
