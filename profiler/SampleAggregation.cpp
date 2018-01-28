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
#include "ProcessState.h"
#include "Sample.h"

#include <algorithm>
#include <memory>
#include <sstream>

SampleAggregation::AggregationMap SampleAggregation::aggregationMap;
SampleAggregation::AggregationOwnerList SampleAggregation::aggregationOwnerList;

SampleAggregation &
SampleAggregation::getAggregation(const Sample &sample)
{
	auto it = aggregationMap.find(sample.getProcessID());
	if (it != aggregationMap.end())
		return *it->second;

	return addAggregation(sample.getProcessID(), "");
}

SampleAggregation &
SampleAggregation::addAggregation(pid_t pid, const std::string &name)
{
	auto ptr = std::make_unique<SampleAggregation>(name, pid);
	SampleAggregation & agg = *ptr;
	aggregationMap[pid] = ptr.get();
	aggregationOwnerList.push_back(std::move(ptr));

	return agg;
}

void
SampleAggregation::processMapIn(pid_t pid, const char *path)
{
	if (aggregationMap.count(pid) != 0)
		return;

	addAggregation(pid, path);
}

void
SampleAggregation::processExec(const ProcessExec & exec)
{
	addAggregation(exec.getProcessID(), exec.getProcessName());
}

void
SampleAggregation::clearAggregations()
{
	aggregationMap.clear();
	aggregationOwnerList.clear();
}

SampleAggregation::SampleAggregation(const std::string &name, pid_t pid)
 : executableName(name), pid(pid), sampleCount(0), userlandSampleCount(0)
{
}

void
SampleAggregation::getAggregationList(AggregationList &list)
{
	for (const auto & agg : aggregationOwnerList) {
		if (agg->sampleCount == 0)
			continue;
		list.push_back(agg.get());
	}

	std::sort(list.rbegin(), list.rend(), SampleAggregation::NumSampleComp());
}

void
SampleAggregation::addFrame(AddressSpace &space, const Sample & sample)
{
	frameMap.insert(std::make_pair(sample,
	    std::make_unique<Callchain>(space, sample)));
}

void
SampleAggregation::addSample(AddressSpace &space, const Sample &sample)
{
	auto it = frameMap.find(sample);

	if (it == frameMap.end())
		addFrame(space, sample);
	else
		it->second->addSample();

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
