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

#ifndef MOCK_MOCK_OPEN_H
#define MOCK_MOCK_OPEN_H

#include <memory>

#include <gmock/gmock.h>

class MockOpen
{
public:
	MOCK_METHOD2(Open, int(std::string name, int flags));
	MOCK_METHOD1(Close, int(int fd));

	static std::unique_ptr<testing::StrictMock<MockOpen>> openMock;

	static void SetUp()
	{
		openMock = std::make_unique<testing::StrictMock<MockOpen>>();
	}

	static void TearDown()
	{
		openMock.reset();
	}

	static void ExpectOpen(const std::string & n, int flags, int fd)
	{
		EXPECT_CALL(*openMock, Open(n, flags))
		    .Times(1).WillOnce(testing::Return(fd));
	}

	static void ExpectClose(int fd, int ret)
	{
		EXPECT_CALL(*openMock, Close(fd))
		    .Times(1).WillOnce(testing::Return(ret));
	}
};

std::unique_ptr<testing::StrictMock<MockOpen>> MockOpen::openMock;

extern "C" int
mock_open(const char * file, int flags)
{
	return MockOpen::openMock->Open(file, flags);
}

extern "C" int
mock_close(int fd)
{
	return MockOpen::openMock->Close(fd);
}

#endif
