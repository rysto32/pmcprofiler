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

#ifndef MOCK_GLOBAL_MOCK_H
#define MOCK_GLOBAL_MOCK_H

#include <gmock/gmock.h>

#include <memory>

template <typename Mock>
class GlobalMock
{
public:
	GlobalMock()
	{
		Mock::SetUp();
	}

	~GlobalMock()
	{
		Mock::TearDown();
	}

	auto & operator*()
	{
		return Mock::MockObj();
	}
};

template <typename Derived>
class GlobalMockBase
{
private:
	typedef std::unique_ptr<testing::StrictMock<Derived>> MockPtr;
	static MockPtr mockobj;

	static void SetUp()
	{
		mockobj = std::make_unique<testing::StrictMock<Derived>>();
	}

	static void TearDown()
	{
		mockobj.reset();
	}

	friend class GlobalMock<Derived>;

public:

	static auto & MockObj()
	{
		return *mockobj;
	}

};

template <typename T>
typename GlobalMockBase<T>::MockPtr GlobalMockBase<T>::mockobj;

#endif
