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

#ifndef SAMPLEAGGREGATION_H
#define SAMPLEAGGREGATION_H

#include "ProfilerTypes.h"
#include "Sample.h"

#include <sys/types.h>

#include <sstream>
#include <vector>
#include <memory>
#include <unordered_map>

class Callchain;
class CallchainFactory;
class CallframeMapper;
class ProcessExec;

class SampleAggregation
{
private:
	typedef std::unordered_map<Sample, std::unique_ptr<Callchain>, Sample::hash> FrameMap;

	FrameMap frameMap;
	std::string executableName;
	mutable std::string baseName;
	mutable std::string displayName;
	pid_t pid;
	size_t sampleCount;
	size_t userlandSampleCount;
	CallchainFactory & factory;

	Callchain * addFrame(CallframeMapper &space, const Sample &);

public:
	SampleAggregation(CallchainFactory &, const std::string & name, pid_t);

	// Prevent consumers from getting a dependency on ~Callchain
	~SampleAggregation();

	SampleAggregation(const SampleAggregation&) = delete;
	SampleAggregation& operator=(const SampleAggregation &) = delete;

	void addSample(CallframeMapper &, const Sample &);

	void getCallchainList(CallchainList &) const;

	const std::string & getExecutable() const
	{
		return executableName;
	}

	const std::string & getDisplayName() const;
	const std::string & getBaseName() const;

	pid_t getPid() const
	{
		return pid;
	}

	size_t getSampleCount() const
	{
		return sampleCount;
	}

	class NumSampleComp
	{
	public:
		bool operator()(const SampleAggregation *, const SampleAggregation *);
	};
};

#endif
