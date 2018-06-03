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

#ifndef DEFAULT_SAMPLE_AGGREGATION_FACTORY
#define DEFAULT_SAMPLE_AGGREGATION_FACTORY

#include "SampleAggregationFactory.h"

#include <unordered_map>
#include <vector>

class CallchainFactory;

class DefaultSampleAggregationFactory : public SampleAggregationFactory
{
private:
	typedef std::vector<std::unique_ptr<SampleAggregation>> AggregationOwnerList;
	typedef std::unordered_map<pid_t, SampleAggregation*> AggregationMap;

	AggregationOwnerList aggregationOwnerList;
	AggregationMap aggregationMap;
	CallchainFactory & ccFactory;

	SampleAggregation &AddAggregation(pid_t, const std::string &);

public:
	// These need to be defined in the .cpp file to prevent consumers from
	// needing to include SampleAggregation.h
	DefaultSampleAggregationFactory(CallchainFactory &);
	~DefaultSampleAggregationFactory();

	virtual SampleAggregation &GetAggregation(const Sample &);
	virtual void GetAggregationList(AggregationList &);
	virtual void HandleExec(const ProcessExec &);
	virtual void HandleMapIn(pid_t pid, const char *path);
};

#endif
