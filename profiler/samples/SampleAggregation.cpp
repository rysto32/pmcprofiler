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

#include "SampleAggregation.h"

#include "AddressSpace.h"
#include "Callchain.h"
#include "CallchainFactory.h"
#include "ProcessState.h"
#include "Sample.h"

#include <algorithm>
#include <memory>
#include <sstream>

SampleAggregation::SampleAggregation(CallchainFactory & f, const std::string &name, pid_t pid)
 : executableName(name),
   pid(pid),
   sampleCount(0),
   userlandSampleCount(0),
   factory(f)
{
}

SampleAggregation::~SampleAggregation()
{

}

Callchain *
SampleAggregation::addFrame(CallframeMapper &space, const Sample & sample)
{
	auto ptr = factory.MakeCallchain(space, sample);
	Callchain * cc = ptr.get();
	frameMap.insert(std::make_pair(sample, std::move(ptr)));

	return cc;
}

void
SampleAggregation::addSample(CallframeMapper &space, const Sample &sample)
{
	auto it = frameMap.find(sample);

	Callchain *cc;
	if (it == frameMap.end())
		cc = addFrame(space, sample);
	else
		cc = it->second.get();

	cc->addSample();
	sampleCount++;
	if (!sample.isKernel())
		userlandSampleCount++;
}

void
SampleAggregation::getCallchainList(CallchainList &list) const
{
	for (const auto & pair : frameMap) {
		list.emplace_back(this, pair.second.get());
	}
}

bool
SampleAggregation::NumSampleComp::operator()(const SampleAggregation* l, const SampleAggregation* r)
{
	return l->sampleCount < r->sampleCount;
}

const std::string &
SampleAggregation::getBaseName() const
{
	if (baseName.empty()) {
		if (!executableName.empty()) {
			auto it = executableName.find_last_of('/');
			if (it != std::string::npos)
				baseName = executableName.substr(it + 1);
			else
				baseName = executableName;
		} else if (userlandSampleCount == 0) {
			baseName = "kernel";
		} else {
			baseName = "unknown_file";
		}
	}
	return baseName;
}

const std::string &
SampleAggregation::getDisplayName() const
{
	if (!displayName.empty())
		return displayName;

	const char *procName;
	if (!executableName.empty())
		procName = executableName.c_str();
	else if (userlandSampleCount == 0)
		procName = "kproc";
	else
		procName = "<unknown>";

	std::ostringstream out;
	out << procName << " (" << pid << ")";
	displayName = out.str();

	return displayName;
}
