// Copyright (c) 2019 Ryan Stone.  All rights reserved.
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

#include "TypeProfilePrinter.h"

#include "BufferSample.h"
#include "BufferSampleFactory.h"
#include "Callchain.h"
#include "Callframe.h"
#include "SampleAggregation.h"
#include "TargetType.h"

#include <vector>

size_t noDwarf, noText, disassembleFailed, findVarFailed;

void
TypeProfilePrinter::printProfile(const Profiler & profiler,
    const AggregationList & aggList)
{
	CallchainList callchainList;
	for (const auto & agg : aggList) {
		agg->getCallchainList(callchainList);
	}

	for (const auto & chainRec : callchainList) {
		auto chain = chainRec.chain;
		auto & frame = chain->getLeafCallframe();

		frame.AddTypeSample(chain->getSampleCount());
	}

	std::vector<BufferSample*> bufferList;
	sampleFactory.GetSamples(bufferList);

	std::sort(bufferList.begin(), bufferList.end(),
		[](const BufferSample *a, const BufferSample *b)
		{
			return a->GetTotalSamples() > b->GetTotalSamples();
		});

	fprintf(m_outfile, "No DWARF: %zd, No .text: %zd, Disassemble Failed: %zd, No Variable: %zd\n",
	    noDwarf, noText, disassembleFailed, findVarFailed);
	for (const auto * buffer : bufferList) {
		fprintf(m_outfile, "%s: %zd samples\n", buffer->GetType().GetName()->c_str(), buffer->GetTotalSamples());
	}
}
