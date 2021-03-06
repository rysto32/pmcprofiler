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

#if !defined(PROFILER_H)
#define PROFILER_H

#include <string>
#include <vector>
#include <stdint.h>

#include <sys/types.h>

#include <unordered_set>

#include "ProfilerTypes.h"

class AddressSpace;
class AddressSpaceFactory;
class ImageFactory;
class Process;
class ProcessExit;
class ProcessExec;
class Sample;
class SampleAggregationFactory;
class ProfilePrinter;

extern std::unordered_set<pid_t> pid_filter;

class Profiler
{
private:
	unsigned m_sampleCount;

	const std::string m_dataFile;
	bool m_showlines;

	std::string kernelFile;
	std::vector<std::string> modulePath;
	AddressSpaceFactory &asFactory;
	SampleAggregationFactory &aggFactory;
	ImageFactory &imgFactory;

	void parseModulePath(char * path_buf, std::vector<std::string> & vec);
	void getLocalModulePath();
	void overrideModulePath(const char *modulePathStr);

	AddressSpace & GetAddressSpace(bool kernel, pid_t pid);

public:

	Profiler(const std::string& dataFile, bool showlines,
	    const char *modulePathStr, AddressSpaceFactory &,
	    SampleAggregationFactory &, ImageFactory &);

	Profiler(const Profiler&) = delete;
	Profiler& operator=(const Profiler &) = delete;

	const std::string& getDataFile() const
	{
		return m_dataFile;
	}

	unsigned getSampleCount() const
	{
		return m_sampleCount;
	}

	bool showLines() const
	{
		return m_showlines;
	}

	void MapSamples();
	void createProfile(ProfilePrinter & printer);

	void processEvent(const ProcessExec& processExec);
	void processEvent(const Sample& sample);
	void processMapIn(pid_t pid, TargetAddr map_start, const char * image);
};

#endif // #if !defined(PROFILER_H)

