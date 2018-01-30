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

#include "EventFactory.h"

#include "ProcessState.h"
#include "Profiler.h"
#include "ProfilerTypes.h"
#include "Sample.h"

#include "mock/MockAddressSpaceFactory.h"
#include "mock/MockImageFactory.h"
#include "mock/MockOpen.h"
#include "mock/MockSampleAggregationFactory.h"

#include <err.h>
#include <fcntl.h>
#include <pmc.h>
#include <pmclog.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>

using namespace testing;

class ProfilerMocker
{
public:
	MOCK_METHOD1(processSample, void (const Sample &));
	MOCK_METHOD3(processMapIn, void (int, TargetAddr, std::string));
	MOCK_METHOD1(processExec, void (const ProcessExec &));
};

static std::unique_ptr<StrictMock<ProfilerMocker>> profilerMock;

void
Profiler::processEvent(const ProcessExec& processExec)
{
	profilerMock->processExec(processExec);
}

void
Profiler::processEvent(const Sample& sample)
{
	profilerMock->processSample(sample);
}

void
Profiler::processMapIn(pid_t pid, TargetAddr map_start, const char * image)
{
	profilerMock->processMapIn(pid, map_start, image);
}

class LibpmcMocker
{
public:
	MOCK_METHOD1(pmclog_open, void *(int));
	MOCK_METHOD2(pmclog_read, int (void *, pmclog_ev *));
	MOCK_METHOD1(pmclog_close, void (void *));
};

static std::unique_ptr<StrictMock<LibpmcMocker>> libpmcMock;

void *
pmclog_open(int fd)
{
	return libpmcMock->pmclog_open(fd);
}

int
pmclog_read(void *cookie, pmclog_ev *ev)
{
	return libpmcMock->pmclog_read(cookie, ev);
}

void
pmclog_close(void * cookie)
{
	return libpmcMock->pmclog_close(cookie);
}

// Stubs
void usage() {}
void warn(const char *, ...) {}

Profiler::Profiler(const std::string& dataFile, bool showlines,
    const char* modulePathStr, AddressSpaceFactory & asFactory,
    SampleAggregationFactory & aggFactory, ImageFactory & imgFactory)
  : m_dataFile(dataFile),
    asFactory(asFactory),
    aggFactory(aggFactory),
    imgFactory(imgFactory)
{
}

// Support code for individual test cases
template <int depth>
class SampleAddress
{
public:
	typedef uintptr_t result_type;

	result_type operator()(const Sample & s) const
	{
		return s.getAddress(depth);
	}
};

class SampleCallchainMatcher
{
private:
	std::vector<TargetAddr> chain;

public:
	SampleCallchainMatcher(const std::vector<TargetAddr> & v)
	  : chain(v)
	{
	}

	typedef bool result_type;
	result_type operator()(const Sample & s) const
	{
		if (chain.size() != s.getChainDepth())
			return false;

		for (int i = 0; i < chain.size(); ++i) {
			if (chain.at(i) != s.getAddress(i))
				return false;
		}

		return true;
	}
};

static void
AddSampleExpectation(void * cookie, bool usermode, pid_t pid, TargetAddr pc)
{
	pmclog_ev event = {
		.pl_state = PMCLOG_OK,
		.pl_type = PMCLOG_TYPE_PCSAMPLE,
		.pl_u.pl_s = {
			.pl_usermode = usermode,
			.pl_pid = pid,
			.pl_pc = pc + 1,
		},
	};
	EXPECT_CALL(*libpmcMock, pmclog_read(cookie, _))
	    .Times(1)
	    .WillOnce(DoAll(
	        SetArgPointee<1>(event),
	        Return(0)
	    )
	);

	EXPECT_CALL(*profilerMock, processSample(AllOf(
		Property(&Sample::getProcessID, pid),
		Property(&Sample::getChainDepth, 1),
		Property(&Sample::isKernel, !usermode),
		ResultOf(SampleAddress<0>(), pc)
	)));

}

static void
AddCallchainExpectation(void * cookie, bool usermode, pid_t pid,
    size_t max_depth, std::vector<TargetAddr> chain)
{
	pmclog_ev event = {
		.pl_state = PMCLOG_OK,
		.pl_type = PMCLOG_TYPE_CALLCHAIN,
		.pl_u.pl_cc = {
			.pl_cpuflags = usermode ? PMC_CC_F_USERSPACE : 0u,
			.pl_pid = static_cast<uint32_t>(pid),
			.pl_npc = static_cast<uint32_t>(chain.size()),
		},
	};

	int i = 0;
	for (TargetAddr pc : chain) {
		event.pl_u.pl_cc.pl_pc[i] = pc + 1;
		i++;
	}

	EXPECT_CALL(*libpmcMock, pmclog_read(cookie, _))
	    .Times(1)
	    .WillOnce(DoAll(
	        SetArgPointee<1>(event),
	        Return(0)
	    )
	);

	int depth = std::min(max_depth, chain.size());
	chain.resize(depth);

	EXPECT_CALL(*profilerMock, processSample(AllOf(
		Property(&Sample::getProcessID, pid),
		Property(&Sample::getChainDepth, depth),
		Property(&Sample::isKernel, !usermode),
		ResultOf(SampleCallchainMatcher(chain), true)
	)));

}

static void
AddMapInExpectation(void * cookie, pid_t pid, TargetAddr start, const char * file)
{
	pmclog_ev event = {
		.pl_state = PMCLOG_OK,
		.pl_type = PMCLOG_TYPE_MAP_IN,
		.pl_u.pl_mi = {
			.pl_pid = pid,
			.pl_start = start,
		}
	};

	strlcpy(event.pl_u.pl_mi.pl_pathname, file, sizeof(event.pl_u.pl_mi.pl_pathname));

	EXPECT_CALL(*libpmcMock, pmclog_read(cookie, _))
	    .Times(1)
	    .WillOnce(DoAll(
	        SetArgPointee<1>(event),
	        Return(0)
	    )
	);

	EXPECT_CALL(*profilerMock, processMapIn(pid, start, StrEq(file)));
}

static void
AddUnhandledTypeExpectation(void * cookie, pmclog_type type)
{
	pmclog_ev event = {
		.pl_state = PMCLOG_OK,
		.pl_type = type,
	};

	EXPECT_CALL(*libpmcMock, pmclog_read(cookie, _))
	    .Times(1)
	    .WillOnce(DoAll(
	        SetArgPointee<1>(event),
	        Return(0)
	    ));
}

class EventFactoryTestSuite : public ::testing::Test
{
public:
	MockImageFactory imgFactory;
	MockAddressSpaceFactory asFactory;
	MockSampleAggregationFactory aggFactory;

	void SetUp()
	{
		openMock = std::make_unique<StrictMock<OpenMocker>>();
		profilerMock = std::make_unique<StrictMock<ProfilerMocker>>();
		libpmcMock = std::make_unique<StrictMock<LibpmcMocker>>();

		// Ensure that the test doesn't get stuck in an infinite loop
		// if it calls us with something we don't expect.
		ON_CALL(*libpmcMock, pmclog_read(_, _)).WillByDefault(Return(-1));
	}

	void TearDown()
	{
		openMock.reset();
		profilerMock.reset();
		libpmcMock.reset();
	}
};

TEST_F(EventFactoryTestSuite, TestEmptyPmclog)
{
	Profiler profiler("/tmp/samples.out", false, "", asFactory, aggFactory,
	    imgFactory);

	char cookie_val;
	{
		InSequence dummy;

		// Use an arbitrary address for the cookie -- it's opaque
		// to the user
		void * cookie = &cookie_val;

		const int fd = 12625;
		EXPECT_CALL(*openMock, Open("/tmp/samples.out", O_RDONLY))
		    .Times(1).WillOnce(Return(fd));

		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(cookie));

		EXPECT_CALL(*libpmcMock, pmclog_read(cookie, _))
		    .Times(1).WillOnce(Return(-1));

		EXPECT_CALL(*libpmcMock, pmclog_close(cookie))
		    .Times(1);
		EXPECT_CALL(*openMock, Close(fd))
		    .Times(1).WillOnce(Return(0));
	}

	EventFactory::createEvents(profiler, 1);
}

TEST_F(EventFactoryTestSuite, TestSingleSample)
{
	Profiler profiler("/root/pmc.log", false, "", asFactory, aggFactory,
	    imgFactory);

	{
		InSequence dummy;
		char cookie_val;

		// Use an arbitrary address for the cookie -- it's opaque
		// to the user
		void * cookie = &cookie_val;

		const int fd = 9628;
		EXPECT_CALL(*openMock, Open("/root/pmc.log", O_RDONLY))
		    .Times(1).WillOnce(Return(fd));

		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(cookie));

		AddSampleExpectation(cookie, true, 17498, 0xbadd);

		EXPECT_CALL(*libpmcMock, pmclog_read(cookie, _))
		    .Times(1).WillOnce(Return(-1));

		EXPECT_CALL(*libpmcMock, pmclog_close(cookie))
		    .Times(1);
		EXPECT_CALL(*openMock, Close(fd))
		    .Times(1).WillOnce(Return(0));
	}

	EventFactory::createEvents(profiler, 1);

}

TEST_F(EventFactoryTestSuite, TestSingleCallchain)
{
	Profiler profiler("pmcstat.bin", false, "", asFactory, aggFactory,
	    imgFactory);

	uint32_t max_depth = 32;

	{
		InSequence dummy;

		// Use an arbitrary address for the cookie -- it's opaque
		// to the user
		void * cookie = openMock.get();

		const int fd = 1750;
		EXPECT_CALL(*openMock, Open("pmcstat.bin", O_RDONLY))
		    .Times(1).WillOnce(Return(fd));

		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(cookie));

		AddCallchainExpectation(cookie, false, 90746, max_depth, {0xe569d, 0x54896, 0x49564});

		EXPECT_CALL(*libpmcMock, pmclog_read(cookie, _))
		    .Times(1).WillOnce(Return(-1));

		EXPECT_CALL(*libpmcMock, pmclog_close(cookie))
		    .Times(1);
		EXPECT_CALL(*openMock, Close(fd))
		    .Times(1).WillOnce(Return(0));
	}

	EventFactory::createEvents(profiler, max_depth);

}

TEST_F(EventFactoryTestSuite, TestMaxDepth)
{
	Profiler profiler("pmcstat.bin", false, "", asFactory, aggFactory,
	    imgFactory);

	uint32_t max_depth = 3;

	{
		InSequence dummy;
		char cookie_val;

		// Use an arbitrary address for the cookie -- it's opaque
		// to the user
		void * cookie = &cookie_val;

		const int fd = INT_MAX;
		EXPECT_CALL(*openMock, Open("pmcstat.bin", O_RDONLY))
		    .Times(1).WillOnce(Return(fd));

		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(cookie));

		AddCallchainExpectation(cookie, false, 90746, max_depth,
		    {0xf9d2, 0x296487, 0xefe892, 0x59dab});

		EXPECT_CALL(*libpmcMock, pmclog_read(cookie, _))
		    .Times(1).WillOnce(Return(-1));

		EXPECT_CALL(*libpmcMock, pmclog_close(cookie))
		    .Times(1);
		EXPECT_CALL(*openMock, Close(fd))
		    .Times(1).WillOnce(Return(0));
	}

	EventFactory::createEvents(profiler, max_depth);

}

TEST_F(EventFactoryTestSuite, TestMapIn)
{
	Profiler profiler("./output/callchains", false, "", asFactory, aggFactory,
	    imgFactory);

	uint32_t max_depth = 3;

	{
		InSequence dummy;
		const int fd = INT_MAX - 1;

		// Use an arbitrary address for the cookie -- it's opaque
		// to the user
		void *cookie = &max_depth;

		EXPECT_CALL(*openMock, Open("./output/callchains", O_RDONLY))
		    .Times(1).WillOnce(Return(fd));
		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(cookie));

		AddMapInExpectation(&max_depth, 231, 0xfffe9398, "/lib/libthr.so.3");

		EXPECT_CALL(*libpmcMock, pmclog_read(cookie, _))
		    .Times(1).WillOnce(Return(-1));

		EXPECT_CALL(*libpmcMock, pmclog_close(cookie))
		    .Times(1);
		EXPECT_CALL(*openMock, Close(fd))
		    .Times(1).WillOnce(Return(0));
	}

	EventFactory::createEvents(profiler, max_depth);

}

TEST_F(EventFactoryTestSuite, TestOpenFail)
{
	Profiler profiler("./output/callchains", false, "", asFactory, aggFactory,
	    imgFactory);
	EXPECT_CALL(*openMock, Open("./output/callchains", O_RDONLY))
	    .Times(1).WillOnce(Return(-1));

	EventFactory::createEvents(profiler, 32);
}

TEST_F(EventFactoryTestSuite, TestLogOpenFail)
{
	Profiler profiler("./output/callchains", false, "", asFactory, aggFactory,
	    imgFactory);

	uint32_t max_depth = 3;

	{
		InSequence dummy;
		const int fd = 4841;
		EXPECT_CALL(*openMock, Open("./output/callchains", O_RDONLY))
		    .Times(1).WillOnce(Return(fd));
		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(NULL));
		EXPECT_CALL(*openMock, Close(fd))
		    .Times(1).WillOnce(Return(0));
	}

	EventFactory::createEvents(profiler, 32);
}

TEST_F(EventFactoryTestSuite, TestUnhandledEvents)
{
	Profiler profiler("./output/callchains", false, "", asFactory, aggFactory,
	    imgFactory);

	uint32_t max_depth = 3;

	// Use an arbitrary address for the cookie -- it's opaque
	// to the user
	void * cookie = &profiler;

	{
		InSequence dummy;
		const int fd = INT_MAX - 1;
		EXPECT_CALL(*openMock, Open("./output/callchains", O_RDONLY))
		    .Times(1).WillOnce(Return(fd));
		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(cookie));

		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_CLOSELOG);
		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_DROPNOTIFY);
		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_INITIALIZE);
		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_MAPPINGCHANGE);
		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_PMCALLOCATE);
		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_PMCATTACH);
		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_PMCDETACH);
		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_PROCCSW);
		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_PROCEXIT);
		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_PROCFORK);
		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_SYSEXIT);
		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_USERDATA);
		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_MAP_OUT);
		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_PMCALLOCATEDYN);
		AddUnhandledTypeExpectation(cookie, static_cast<pmclog_type>(1000));

		EXPECT_CALL(*libpmcMock, pmclog_read(cookie, _))
		    .Times(1).WillOnce(Return(-1));

		EXPECT_CALL(*libpmcMock, pmclog_close(cookie))
		    .Times(1);
		EXPECT_CALL(*openMock, Close(fd))
		    .Times(1).WillOnce(Return(0));
	}

	EventFactory::createEvents(profiler, max_depth);
}
