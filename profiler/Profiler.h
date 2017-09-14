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

class Process;
class ProcessExit;
class ProcessExec;
class Sample;
class ProfilePrinter;

extern std::unordered_set<pid_t> pid_filter;

class Profiler
{
private:

	unsigned m_sampleCount;

	const std::string& m_dataFile;
	bool m_showlines;

public:

	Profiler(const std::string& dataFile, bool showlines)
	: m_sampleCount(0),
	  m_dataFile(dataFile),
	  m_showlines(showlines)
	{
	}

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

	void createProfile(ProfilePrinter & printer);

	void processEvent(const ProcessExec& processExec);
	void processEvent(const ProcessExit& processExit);
	void processEvent(const Sample& sample);
	void processMapIn(pid_t pid, uintptr_t map_start, const char * image);
};

#endif // #if !defined(PROFILER_H)

