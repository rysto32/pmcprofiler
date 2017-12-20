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

#include <paths.h>
#include <libgen.h>
#include <cassert>

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
FlatProfilePrinter::printProfile(const Profiler & profiler,
				 const AggregationList & aggList)
{
#if 0
	LocationList locationList;
	Process::collectAllLocations(locationList);
	Image::mapAllLocations(locationList);
	Process::mapAllFunctions(locationList, LeafProcessStrategy());
#endif

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

#if 0
	for (auto agg : aggList) {
		     fprintf(m_outfile, "\nProcess: %6u, %s, total: %u (%6.2f%%)\n", process.getPid(), process.getName().c_str(),
			     process.getSampleCount(), (process.getSampleCount() * 100.0) / profiler.getSampleCount());
		     FunctionList functionList;
		     process.getFunctionList(functionList);
		     unsigned cumulativeCount = 0;
		     fprintf(m_outfile, "       time   time-t   samples   env  file / library, line number, function\n");
		     for (FunctionList::iterator functionListIterator = functionList.begin(); functionListIterator != functionList.end(); ++functionListIterator) {
			     FunctionLocation& functionLocation(*functionListIterator);
			     char * functionName = Image::demangle(*functionLocation.getFunctionName());
			     cumulativeCount += functionLocation.getCount();
			     fprintf(m_outfile, "    %6.2f%%, %6.2f%%, %8u, %s, %s:%u, %s",
				     (functionLocation.getCount() * 100.0) / process.getSampleCount(),
				     (cumulativeCount * 100.0) / process.getSampleCount(),
				     functionLocation.getCount(),
				     functionLocation.isKernelImage() ? "kern" : "user",
				     functionLocation.getFileName()->empty() ? functionLocation.getModuleName()->c_str() : functionLocation.getFileName()->c_str(),
				     functionLocation.getLineNumber(),
				     functionName ? functionName : "<unknown>");
			     printLineNumbers(profiler, functionLocation.getLineLocationList());
			     fprintf(m_outfile, "\n");
			     free(functionName);
		     }
	     }
#endif
#if 0
	for (auto agg : aggList) {
		double percent = (agg->getSampleCount() * 100.0) / profiler.getSampleCount();
		fprintf(m_outfile, "\nProcess: %s, total: %zu (%.2f%%)\n",
		    agg->getDisplayName().c_str(), agg->getSampleCount(),
		    percent);

		agg->getCallchainList(callchainList);

		for (auto chain : callchainList) {
			std::vector<InlineFrame> frames;
			chain->flatten(frames);

			percent = (chain->getSampleCount() * 100.0) / agg->getSampleCount();
			fprintf(m_outfile, "%6.2f%% 0x%lx %s @ %s:%d\n",
			    percent, frames.front().getOffset(),
			    frames.front().getDemangled()->c_str(),
			    frames.front().getFile()->c_str(),
			    frames.front().getCodeLine());
		}
	}
#endif
}

