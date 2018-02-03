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

#include "Callchain.h"

#include "Callframe.h"
#include "CallframeMapper.h"
#include "InlineFrame.h"
#include "Sample.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "TestPrinter/SharedString.h"

using namespace testing;

class MockFrameMapper : public CallframeMapper
{
public:
	MOCK_METHOD1(mapFrame, const Callframe &(TargetAddr addr));
	MOCK_CONST_METHOD0(getExecutableName, SharedString ());
};

TEST(CallchainTestSuite, TestGetters)
{
	SharedString imageName("libc.so.7");
	Callframe cf(0x543, imageName);
	cf.setUnmapped();

	MockFrameMapper mapper;

	EXPECT_CALL(mapper, mapFrame(0x543)).WillOnce(ReturnRef(cf));

	pmclog_ev_pcsample event = {
		.pl_usermode = 0,
		.pl_pid = 123,
		.pl_pc = 0x544,
	};

	Sample s(event);
	Callchain chain(mapper, s);

	EXPECT_EQ(&chain.getMapper(), &mapper);
	EXPECT_EQ(chain.getAddress(), 0x543);
	EXPECT_TRUE(chain.isKernel());
	EXPECT_EQ(chain.getSampleCount(), 1);
	EXPECT_FALSE(chain.isMapped());

	const InlineFrame & ifr = chain.getLeafFrame();
	EXPECT_EQ(ifr.getFile(), imageName);
	EXPECT_FALSE(ifr.isMapped());
}

TEST(CallchainTestSuite, TestGetSelfFrame)
{
	SharedString imageName("a.out");
	TargetAddr off = 0xbad;
	Callframe cf(off, imageName);

	SharedString file("main.cpp");
	SharedString func("DoFoobar");
	SharedString demangled(func);
	int codeLine = 101;
	int funcLine = 94;
	cf.addFrame(file, func, demangled, codeLine, funcLine, 0x2896);

	MockFrameMapper mapper;

	EXPECT_CALL(mapper, mapFrame(off)).WillOnce(ReturnRef(cf));

	pmclog_ev_pcsample event = {
		.pl_usermode = 0,
		.pl_pid = 123,
		.pl_pc = off + 1,
	};

	Sample s(event);
	Callchain chain(mapper, s);
	const InlineFrame & frame = chain.getLeafFrame();

	const InlineFrame * self = chain.getSelfFrame(frame);
	ASSERT_TRUE(self);
	EXPECT_EQ(self->getFile(), file);
	EXPECT_EQ(self->getFunc(), "[self]");
	EXPECT_EQ(self->getDemangled(), "[self]");
	EXPECT_EQ(self->getCodeLine(), codeLine);
	EXPECT_EQ(self->getFuncLine(), funcLine);
	EXPECT_EQ(self->getOffset(), off);
	EXPECT_EQ(self->getImageName(), imageName);

	// The callchain should cache the self frame and re-return it if we callchain
	// this a second time.
	EXPECT_EQ(self, chain.getSelfFrame(frame));
}

TEST(CallchainTestSuite, TestFlatten)
{
	SharedString imageName("a.out");
	std::vector<Callframe> frameList;
	frameList.emplace_back(0x14881, imageName);
	frameList.emplace_back(0x2586, imageName);
	frameList.emplace_back(0x489, imageName);

	MockFrameMapper mapper;

	pmclog_ev_callchain event = {
		.pl_cpuflags = PMC_CC_F_USERSPACE,
		.pl_pid = 123,
		.pl_npc = 0,
	};

	for (const auto & cf : frameList) {
		event.pl_pc[event.pl_npc] = cf.getOffset() + 1;
		event.pl_npc++;

		EXPECT_CALL(mapper, mapFrame(cf.getOffset())).WillOnce(ReturnRef(cf));
	}

	Sample s(event, 32);
	Callchain chain(mapper, s);

	frameList.at(0).addFrame("", "", "", 1, 1, 0x11);
	frameList.at(0).addFrame("", "", "", 1, 1, 0x12);

	frameList.at(1).addFrame("", "", "", 1, 1, 0x21);

	frameList.at(2).addFrame("", "", "", 1, 1, 0x31);
	frameList.at(2).addFrame("", "", "", 1, 1, 0x32);

	std::vector<const InlineFrame*> ifl;
	chain.flatten(ifl);

	ASSERT_EQ(ifl.size(), 5);
	EXPECT_EQ(ifl.at(0)->getDieOffset(), 0x11);
	EXPECT_EQ(ifl.at(1)->getDieOffset(), 0x12);
	EXPECT_EQ(ifl.at(2)->getDieOffset(), 0x21);
	EXPECT_EQ(ifl.at(3)->getDieOffset(), 0x31);
	EXPECT_EQ(ifl.at(4)->getDieOffset(), 0x32);
}
