// Copyright (c) 2018 Ryan Stone.  All rights reserved.
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

#include "DefaultSampleAggregationFactory.h"

#include "ProcessState.h"
#include "Sample.h"
#include "SampleAggregation.h"

DefaultSampleAggregationFactory::DefaultSampleAggregationFactory(CallchainFactory & factory)
  : ccFactory(factory)
{

}

DefaultSampleAggregationFactory::~DefaultSampleAggregationFactory()
{

}

SampleAggregation &
DefaultSampleAggregationFactory::GetAggregation(const Sample &sample)
{
	auto it = aggregationMap.find(sample.getProcessID());
	if (it != aggregationMap.end())
		return *it->second;

	return AddAggregation(sample.getProcessID(), "");
}

SampleAggregation &
DefaultSampleAggregationFactory::AddAggregation(pid_t pid, const std::string &name)
{
	auto ptr = std::make_unique<SampleAggregation>(ccFactory, name, pid);
	SampleAggregation & agg = *ptr;
	aggregationMap[pid] = ptr.get();
	aggregationOwnerList.push_back(std::move(ptr));

	return agg;
}

void
DefaultSampleAggregationFactory::HandleExec(const ProcessExec & exec)
{
	AddAggregation(exec.getProcessID(), exec.getProcessName());
}

void
DefaultSampleAggregationFactory::HandleMapIn(pid_t pid, const char *path)
{
	if (aggregationMap.count(pid) != 0)
		return;

	AddAggregation(pid, path);
}

void
DefaultSampleAggregationFactory::GetAggregationList(AggregationList &list)
{
	for (const auto & agg : aggregationOwnerList) {
		if (agg->getSampleCount() == 0)
			continue;
		list.push_back(agg.get());
	}

	std::sort(list.rbegin(), list.rend(), SampleAggregation::NumSampleComp());
}
