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
#include "mock/MockSampleAggregationFactory.h"

#include <err.h>
#include <fcntl.h>
#include <pmc.h>
#include <pmclog.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>

using namespace testing;

class OpenMocker
{
public:
	MOCK_METHOD2(Open, int(std::string name, int flags));
	MOCK_METHOD1(Close, int(int fd));
};

static std::unique_ptr<OpenMocker> openMock;

extern "C" int
mock_open(const char * file, int flags)
{
	return openMock->Open(file, flags);
}

extern "C" int
mock_close(int fd)
{
	return openMock->Close(fd);
}

class ProfilerMocker
{
public:
	MOCK_METHOD1(processSample, void (const Sample &));
	MOCK_METHOD3(processMapIn, void (int, TargetAddr, std::string));
	MOCK_METHOD1(processExec, void (const ProcessExec &));
};

static std::unique_ptr<ProfilerMocker> profilerMock;

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

static std::unique_ptr<LibpmcMocker> libpmcMock;

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

class EventFactoryTestSuite : public ::testing::Test
{
public:
	MockImageFactory imgFactory;
	MockAddressSpaceFactory asFactory;
	MockSampleAggregationFactory aggFactory;

	void SetUp()
	{
		openMock = std::make_unique<OpenMocker>();
		profilerMock = std::make_unique<ProfilerMocker>();
		libpmcMock = std::make_unique<LibpmcMocker>();
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

	char cookie;
	{
		InSequence dummy;

		const int fd = 12625;
		EXPECT_CALL(*openMock, Open("/tmp/samples.out", O_RDONLY))
		    .Times(1).WillOnce(Return(fd));

		EXPECT_CALL(*libpmcMock, pmclog_open(fd))
		    .Times(1).WillOnce(Return(&cookie));

		EXPECT_CALL(*libpmcMock, pmclog_read(&cookie, _))
		    .Times(1).WillOnce(Return(-1));

		EXPECT_CALL(*libpmcMock, pmclog_close(&cookie))
		    .Times(1);
		EXPECT_CALL(*openMock, Close(fd))
		    .Times(1).WillOnce(Return(0));
	}

	EventFactory::createEvents(profiler, 1);
}
