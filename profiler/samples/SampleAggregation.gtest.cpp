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

#include "SampleAggregation.h"

#include "Callchain.h"
#include "CallchainFactory.h"
#include "Callframe.h"
#include "CallframeMapper.h"
#include "Sample.h"

#include "mock/GlobalMock.h"

#include <gtest/gtest.h>

using namespace testing;

class MockFrameMapper : public CallframeMapper
{
public:
	MOCK_METHOD1(mapFrame, const Callframe & (TargetAddr));
	MOCK_CONST_METHOD0(getExecutableName, SharedString ());
};

class CallchainMocker : public GlobalMockBase<CallchainMocker>
{
public:
	MOCK_METHOD1(addSample, void (const Callchain *));
};

Callchain::Callchain(CallframeMapper &mapper, const Sample &sample)
  : space(mapper)
{
}

void Callchain::addSample()
{
	CallchainMocker::MockObj().addSample(this);
}

class MockCallchainFactory : public CallchainFactory
{
public:
	MOCK_METHOD2(MakeCallchain, std::unique_ptr<Callchain>(CallframeMapper &space, const Sample & sample));
};

TEST(SampleAggregationTestSuite, TestGetters)
{
	MockCallchainFactory ccFactory;
	SampleAggregation agg(ccFactory, "/usr/bin/pmcstat", 866);

	EXPECT_EQ(agg.getExecutable(), "/usr/bin/pmcstat");
	EXPECT_EQ(agg.getDisplayName(), "/usr/bin/pmcstat (866)");
	EXPECT_EQ(agg.getBaseName(), "pmcstat");
	EXPECT_EQ(agg.getPid(), 866);
}

TEST(SampleAggregationTestSuite, TestAddSingleSample)
{
	MockCallchainFactory ccFactory;
	std::string imageName("sbin/ifconfig");
	SampleAggregation agg(ccFactory, imageName, 156);
	MockFrameMapper mapper;
	GlobalMock<CallchainMocker> callchainMock;

	pmclog_ev_callchain pmc_cc{ .pl_npc = 1, .pl_pc = {0x123}};
	Sample sample(pmc_cc, 1);

	auto ccRet = std::make_unique<Callchain>(mapper, sample);
	Callchain * callchain = ccRet.get();
	EXPECT_CALL(ccFactory, MakeCallchain(Ref(mapper), sample))
	    .Times(1)
	    .WillOnce(Return(ByMove(std::move(ccRet))));
	EXPECT_CALL(*callchainMock, addSample(callchain)).Times(1);

	agg.addSample(mapper, sample);
}

TEST(SampleAggregationTestSuite, TestAddSingleSampleMultipleTimes)
{
	MockCallchainFactory ccFactory;
	std::string imageName("sbin/ifconfig");
	SampleAggregation agg(ccFactory, imageName, 156);
	MockFrameMapper mapper;
	GlobalMock<CallchainMocker> callchainMock;

	pmclog_ev_callchain pmc_cc{ .pl_npc = 1, .pl_pc = {0x123}};
	Sample sample(pmc_cc, 1);

	auto ccRet = std::make_unique<Callchain>(mapper, sample);
	Callchain * callchain = ccRet.get();
	EXPECT_CALL(ccFactory, MakeCallchain(Ref(mapper), sample))
	    .Times(1)
	    .WillOnce(Return(ByMove(std::move(ccRet))));
	EXPECT_CALL(*callchainMock, addSample(callchain)).Times(3);

	agg.addSample(mapper, sample);

	Sample sample2(sample);
	agg.addSample(mapper, sample2);

	agg.addSample(mapper, sample);
}
