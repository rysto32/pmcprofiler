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

#include "Callframe.h"
#include "ProcessState.h"
#include "Profiler.h"
#include "ProfilerTypes.h"
#include "Sample.h"

#include "mock/GlobalMock.h"
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

class ProfilerMocker : public GlobalMockBase<ProfilerMocker>
{
public:
	MOCK_METHOD1(processSample, void (const Sample &));
	MOCK_METHOD3(processMapIn, void (int, TargetAddr, std::string));
	MOCK_METHOD1(processExec, void (const ProcessExec &));
};

void
Profiler::processEvent(const ProcessExec& processExec)
{
	ProfilerMocker::MockObj().processExec(processExec);
}

void
Profiler::processEvent(const Sample& sample)
{
	ProfilerMocker::MockObj().processSample(sample);
}

void
Profiler::processMapIn(pid_t pid, TargetAddr map_start, const char * image)
{
	ProfilerMocker::MockObj().processMapIn(pid, map_start, image);
}

class LibpmcMocker : public GlobalMockBase<LibpmcMocker>
{
public:
	MOCK_METHOD1(pmclog_open, void *(int));
	MOCK_METHOD2(pmclog_read, int (void *, pmclog_ev *));
	MOCK_METHOD1(pmclog_close, void (void *));
};

class GlobalLibpmcMock : public GlobalMock<LibpmcMocker>
{
public:
	GlobalLibpmcMock()
	{
		// Ensure that the test doesn't get stuck in an infinite loop
		// if it calls us with something we don't expect.
		ON_CALL(**this, pmclog_read(_, _)).WillByDefault(Return(-1));
	}
};

void *
pmclog_open(int fd)
{
	return LibpmcMocker::MockObj().pmclog_open(fd);
}

int
pmclog_read(void *cookie, pmclog_ev *ev)
{
	return LibpmcMocker::MockObj().pmclog_read(cookie, ev);
}

void
pmclog_close(void * cookie)
{
	return LibpmcMocker::MockObj().pmclog_close(cookie);
}

// Stubs
void usage() {}
void warn(const char *, ...) {}
Image::Image(SharedString) {}
Image::~Image() {}
Callframe::~Callframe() {}

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

class EventFactoryTestSuite : public ::testing::Test
{
public:
	MockImageFactory imgFactory;
	MockAddressSpaceFactory asFactory;
	MockSampleAggregationFactory aggFactory;
	GlobalLibpmcMock libpmcMock;
	GlobalMock<ProfilerMocker> profilerMock;

#ifdef PMCLOG_TYPE_PCSAMPLE
	void
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
			ResultOf(SampleAddress<0>(), pc))))
		    .Times(1);

	}
#endif

	void
	AddCallchainExpectation(void * cookie, bool usermode,
	    pid_t pid, std::vector<TargetAddr> chain)
	{
		pmclog_ev event = {
			.pl_state = PMCLOG_OK,
			.pl_type = PMCLOG_TYPE_CALLCHAIN,
			.pl_u = {
				.pl_cc = {
					.pl_pid = static_cast<uint32_t>(pid),
					.pl_cpuflags = usermode ? PMC_CC_F_USERSPACE : 0u,
					.pl_npc = static_cast<uint32_t>(chain.size()),
				},
			},
		};

		for (int i = 0; i < chain.size(); ++i) {
			/*
			 * XXX Because kernel backtraces can now contain
			 * userland addresses, Sample will ignore addresses in
			 * kernel callchains that is less than 0x100000000.  This
			 * hack ensures that our test addresses look like kernel
			 * addresses to Sample.
			 */
			if (!usermode) {
				chain[i] |= 0xffffffff00000000ULL;
			}
			event.pl_u.pl_cc.pl_pc[i] = chain[i] + 1;
		}

		EXPECT_CALL(*libpmcMock, pmclog_read(cookie, _))
		    .Times(1)
		    .WillOnce(DoAll(
		        SetArgPointee<1>(event),
		        Return(0)
		    )
		);

		EXPECT_CALL(*profilerMock, processSample(AllOf(
			Property(&Sample::getProcessID, pid),
			Property(&Sample::getChainDepth, chain.size()),
			Property(&Sample::isKernel, !usermode),
			ResultOf(SampleCallchainMatcher(chain), true))))
		    .Times(1);

	}

	void
	AddMapInExpectation(void * cookie, pid_t pid, TargetAddr start, const char * file)
	{
		pmclog_ev event = {
			.pl_state = PMCLOG_OK,
			.pl_type = PMCLOG_TYPE_MAP_IN,
			.pl_u = {
				.pl_mi = {
					.pl_pid = pid,
					.pl_start = start,
				},
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

	void
	AddExecExpectation(void * cookie, pid_t pid, std::string execname, TargetAddr start)
	{
		pmclog_ev event = {
			.pl_state = PMCLOG_OK,
			.pl_type = PMCLOG_TYPE_PROCEXEC,
			.pl_u = {
				.pl_x = {
					.pl_pid = pid,
					.pl_dynaddr = start,
				},
			}
		};

		strlcpy(event.pl_u.pl_x.pl_pathname, execname.c_str(), sizeof(event.pl_u.pl_x.pl_pathname));

		EXPECT_CALL(*libpmcMock, pmclog_read(cookie, _))
		    .Times(1)
		    .WillOnce(DoAll(
		        SetArgPointee<1>(event),
		        Return(0)
		    )
		);

		EXPECT_CALL(*profilerMock, processExec(AllOf(
		    Property(&ProcessExec::getProcessID, pid),
		    Property(&ProcessExec::getProcessName, execname),
		    Property(&ProcessExec::getEntryAddr, start))));
	}

	void
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

	void
	AddEofExpectation(void * cookie)
	{
		pmclog_ev event = {
			.pl_state = PMCLOG_EOF,
		};

		EXPECT_CALL(*libpmcMock, pmclog_read(cookie, _))
		    .Times(1).WillOnce(DoAll(
		        SetArgPointee<1>(event),
		        Return(-1)));
	}
};

TEST_F(EventFactoryTestSuite, TestEmptyPmclog)
{
	Profiler profiler("/tmp/samples.out", false, "", asFactory, aggFactory,
	    imgFactory);
	GlobalMockOpen mockOpen;

	char cookie_val;
	{
		InSequence dummy;

		// Use an arbitrary address for the cookie -- it's opaque
		// to the user
		void * cookie = &cookie_val;

		const int fd = 12625;
		mockOpen.ExpectOpen("/tmp/samples.out", O_RDONLY, fd);

		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(cookie));

		AddEofExpectation(cookie);

		EXPECT_CALL(*libpmcMock, pmclog_close(cookie))
		    .Times(1);
		mockOpen.ExpectClose(fd, 0);
	}

	EventFactory::createEvents(profiler);
}

#ifdef PMCLOG_TYPE_PCSAMPLE
TEST_F(EventFactoryTestSuite, TestSingleSample)
{
	Profiler profiler("/root/pmc.log", false, "", asFactory, aggFactory,
	    imgFactory);
	GlobalMockOpen mockOpen;

	{
		InSequence dummy;
		char cookie_val;

		// Use an arbitrary address for the cookie -- it's opaque
		// to the user
		void * cookie = &cookie_val;

		const int fd = 9628;
		mockOpen.ExpectOpen("/root/pmc.log", O_RDONLY, fd);

		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(cookie));

		AddSampleExpectation(cookie, true, 17498, 0xbadd);

		AddEofExpectation(cookie);

		EXPECT_CALL(*libpmcMock, pmclog_close(cookie))
		    .Times(1);
		mockOpen.ExpectClose(fd, 0);
	}

	EventFactory::createEvents(profiler);

}
#endif

TEST_F(EventFactoryTestSuite, TestSingleCallchain)
{
	Profiler profiler("pmcstat.bin", false, "", asFactory, aggFactory,
	    imgFactory);
	GlobalMockOpen mockOpen;

	{
		InSequence dummy;

		// Use an arbitrary address for the cookie -- it's opaque
		// to the user
		void * cookie = &dummy;

		const int fd = 1750;
		mockOpen.ExpectOpen("pmcstat.bin", O_RDONLY, fd);

		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(cookie));

		AddCallchainExpectation(cookie, false, 90746, {0xe569d, 0x54896, 0x49564});

		AddEofExpectation(cookie);

		EXPECT_CALL(*libpmcMock, pmclog_close(cookie))
		    .Times(1);
		mockOpen.ExpectClose(fd, 0);
	}

	EventFactory::createEvents(profiler);

}

TEST_F(EventFactoryTestSuite, TestMaxDepth)
{
	Profiler profiler("pmcstat.bin", false, "", asFactory, aggFactory,
	    imgFactory);
	GlobalMockOpen mockOpen;

	{
		InSequence dummy;
		char cookie_val;

		// Use an arbitrary address for the cookie -- it's opaque
		// to the user
		void * cookie = &cookie_val;

		const int fd = INT_MAX;
		mockOpen.ExpectOpen("pmcstat.bin", O_RDONLY, fd);

		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(cookie));

		AddCallchainExpectation(cookie, false, 90746,
		    {0xf9d2, 0x296487, 0xefe892, 0x59dab});

		AddEofExpectation(cookie);

		EXPECT_CALL(*libpmcMock, pmclog_close(cookie))
		    .Times(1);
		mockOpen.ExpectClose(fd, 0);
	}

	EventFactory::createEvents(profiler);

}

TEST_F(EventFactoryTestSuite, TestMapIn)
{
	Profiler profiler("./output/callchains", false, "", asFactory, aggFactory,
	    imgFactory);
	GlobalMockOpen mockOpen;

	{
		InSequence dummy;
		const int fd = INT_MAX - 1;

		// Use an arbitrary address for the cookie -- it's opaque
		// to the user
		void *cookie = &mockOpen;

		mockOpen.ExpectOpen("./output/callchains", O_RDONLY, fd);
		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(cookie));

		AddMapInExpectation(cookie, 231, 0xfffe9398, "/lib/libthr.so.3");

		AddEofExpectation(cookie);

		EXPECT_CALL(*libpmcMock, pmclog_close(cookie))
		    .Times(1);
		mockOpen.ExpectClose(fd, 0);
	}

	EventFactory::createEvents(profiler);
}

TEST_F(EventFactoryTestSuite, TestExec)
{
	Profiler profiler("./output/callchains", false, "", asFactory, aggFactory,
	    imgFactory);
	GlobalMockOpen mockOpen;

	{
		InSequence dummy;
		const int fd = 0;

		// Use an arbitrary address for the cookie -- it's opaque
		// to the user
		void *cookie = &mockOpen;

		mockOpen.ExpectOpen("./output/callchains", O_RDONLY, fd);
		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(cookie));

		AddExecExpectation(cookie, 1654, "/usr/bin/c++", 0xfed0);

		AddEofExpectation(cookie);

		EXPECT_CALL(*libpmcMock, pmclog_close(cookie))
		    .Times(1);
		mockOpen.ExpectClose(fd, 0);
	}

	EventFactory::createEvents(profiler);

}

TEST_F(EventFactoryTestSuite, TestOpenFail)
{
	Profiler profiler("./output/callchains", false, "", asFactory, aggFactory,
	    imgFactory);
	GlobalMockOpen mockOpen;

	mockOpen.ExpectOpen("./output/callchains", O_RDONLY, -1);

	EventFactory::createEvents(profiler);
}

TEST_F(EventFactoryTestSuite, TestLogOpenFail)
{
	Profiler profiler("./output/callchains", false, "", asFactory, aggFactory,
	    imgFactory);
	GlobalMockOpen mockOpen;

	{
		InSequence dummy;
		const int fd = 4841;
		mockOpen.ExpectOpen("./output/callchains", O_RDONLY, fd);
		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(NULL));
		mockOpen.ExpectClose(fd, 0);
	}

	EventFactory::createEvents(profiler);
}

TEST_F(EventFactoryTestSuite, TestUnhandledEvents)
{
	Profiler profiler("./output/callchains", false, "", asFactory, aggFactory,
	    imgFactory);
	GlobalMockOpen mockOpen;

	// Use an arbitrary address for the cookie -- it's opaque
	// to the user
	void * cookie = &profiler;

	{
		InSequence dummy;
		const int fd = INT_MAX - 1;
		mockOpen.ExpectOpen("./output/callchains", O_RDONLY, fd);
		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(cookie));

		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_CLOSELOG);
		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_DROPNOTIFY);
		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_INITIALIZE);
#ifdef PMCLOG_TYPE_MAPPINGCHANGE
		AddUnhandledTypeExpectation(cookie, PMCLOG_TYPE_MAPPINGCHANGE);
#endif
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

		AddEofExpectation(cookie);

		EXPECT_CALL(*libpmcMock, pmclog_close(cookie))
		    .Times(1);
		mockOpen.ExpectClose(fd, 0);
	}

	EventFactory::createEvents(profiler);
}

TEST_F(EventFactoryTestSuite, TestMultipleCallchains)
{
	Profiler profiler("./output/callchains", false, "", asFactory, aggFactory,
	    imgFactory);
	GlobalMockOpen mockOpen;

	// Use an arbitrary address for the cookie -- it's opaque
	// to the user
	void * cookie = this;

	{
		InSequence dummy;
		const int fd = 25;
		mockOpen.ExpectOpen("./output/callchains", O_RDONLY, fd);
		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(cookie));


		AddMapInExpectation(cookie, -1, 0xfffe9398, "/boot/kernel/kernel");
		AddMapInExpectation(cookie, -1, 0xffff1040, "/boot/kernel/hwpmc.ko");
		AddMapInExpectation(cookie, 1, 0x1000, "/sbin/init");
		AddMapInExpectation(cookie, 2, 0x5540, "/bin/sh");
		AddMapInExpectation(cookie, 2, 0x100000, "/lib/libc.so.7");

		AddCallchainExpectation(cookie, true, 2,
		    {0xfffe7838, 0xffff2300, 0xfffe8632});
		AddCallchainExpectation(cookie, false, 2,
		    {1, 2, 3, 4});
		AddCallchainExpectation(cookie, false, 1,
		    {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		     17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32});
		AddCallchainExpectation(cookie, true, 1,
		    {0xfffe0001, 4, 2, 5, 2});
		AddExecExpectation(cookie, 3, "/bin/dd", 0x9000);
		AddCallchainExpectation(cookie, true, 3,
		    {454, 1564, 1478, 18794, 1684, 154, 14});
		AddCallchainExpectation(cookie, true, 3,
		    {454, 1564, 1478, 18794, 1684, 154, 14});
		AddCallchainExpectation(cookie, false, 1,
		    {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		     17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32});

		AddEofExpectation(cookie);

		EXPECT_CALL(*libpmcMock, pmclog_close(cookie))
		    .Times(1);
		mockOpen.ExpectClose(fd, 0);
	}

	EventFactory::createEvents(profiler);
}
