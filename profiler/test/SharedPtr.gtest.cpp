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

#include <gtest/gtest.h>

#include "SharedPtr.h"

#include "TestPrinter/SharedPtr.h"

#include <map>

class TargetClass
{
private:
	int x;
	long y;

	typedef std::map<const void *, size_t> AllocMap;
	static AllocMap allocMap;

	void Register()
	{
		const auto * ptr = static_cast<const void *>(this);
		allocMap.insert(std::make_pair(ptr, sizeof(*this)));
	}

	void Deregister()
	{
		const auto * ptr = static_cast<const void *>(this);
		allocMap.erase(ptr);
	}

public:
	TargetClass(int x, long y)
	  : x(x), y(y)
	{
		Register();
	}

	TargetClass(TargetClass && other)
	  : x(other.x), y(other.y)
	{
		Register();
	}

	TargetClass(const TargetClass &) = delete;

	~TargetClass()
	{
		Deregister();
	}

	TargetClass & operator=(TargetClass &&) = delete;
	TargetClass & operator=(const TargetClass &) = delete;

	int getX() const
	{
		return x;
	}

	long getY() const
	{
		return y;
	}

	static bool IsAllocated(const TargetClass * t)
	{
		const auto * ptr = static_cast<const void *>(t);
		return allocMap.count(ptr) != 0;
	}
};

TargetClass::AllocMap TargetClass::allocMap;

typedef SharedPtr<TargetClass> TargetPtr;

TEST(SharedPtrTestSuite, TestDefaultConstructor)
{
	TargetPtr ptr;

	ASSERT_TRUE(!ptr);
	ASSERT_EQ(ptr.get(), nullptr);
}

TEST(SharedPtrTestSuite, TestMakeEmplace)
{
	TargetPtr ptr(TargetPtr::make(0, 1));

	ASSERT_TRUE(ptr);
	ASSERT_EQ(ptr->getX(), 0);
	ASSERT_EQ(ptr->getY(), 1L);

	const auto * actual = ptr.get();
	ASSERT_TRUE(TargetClass::IsAllocated(actual));

	ptr.clear();
	ASSERT_TRUE(!TargetClass::IsAllocated(actual));
}

TEST(SharedPtrTestSuite, TestMakeMove)
{
	const TargetClass *actual;
	{
		TargetPtr ptr(TargetPtr::make(TargetClass(2, 10)));

		ASSERT_TRUE(ptr);
		ASSERT_EQ(ptr->getX(), 2);
		ASSERT_EQ(ptr->getY(), 10L);

		actual = ptr.get();
		ASSERT_TRUE(TargetClass::IsAllocated(actual));
	}
	ASSERT_TRUE(!TargetClass::IsAllocated(actual));
}

TEST(SharedPtrTestSuite, TestCopyConstructor)
{
	TargetPtr first(TargetPtr::make(0, 1));
	TargetPtr second(first);

	ASSERT_TRUE(second);
	ASSERT_EQ(first, second);
	ASSERT_EQ(first.get(), second.get());

	const auto * actual = first.get();

	first.clear();
	ASSERT_TRUE(second);
	ASSERT_TRUE(TargetClass::IsAllocated(actual));

	second = TargetPtr::make(1, 0);
	ASSERT_TRUE(!TargetClass::IsAllocated(actual));
}

TEST(SharedPtrTestSuite, TestMoveConstructor)
{
	TargetPtr first(TargetPtr::make(0, 1));
	TargetPtr second = first;
	TargetPtr third(std::move(second));

	ASSERT_TRUE(third);
	ASSERT_EQ(first, third);
	ASSERT_EQ(first.get(), third.get());

	const auto * actual = first.get();
	ASSERT_TRUE(TargetClass::IsAllocated(actual));

	// Clear only first and third and ensure that second doesn't
	// hold a stray reference.
	first.clear();
	ASSERT_TRUE(TargetClass::IsAllocated(actual));

	third.clear();
	ASSERT_TRUE(!TargetClass::IsAllocated(actual));
}

TEST(SharedPtrTestSuite, TestDereference)
{
	TargetPtr first(TargetPtr::make(1000, 4186));

	ASSERT_EQ((*first).getX(), 1000);
	ASSERT_EQ((*first).getY(), 4186);
}

TEST(SharedPtrTestSuite, TestPointsTo)
{
	TargetPtr first(TargetPtr::make(1652, 26));

	ASSERT_EQ(first->getX(), 1652);
	ASSERT_EQ(first->getY(), 26);
}

TEST(SharedPtrTestSuite, TestCopyAssign)
{
	TargetPtr first(TargetPtr::make(8, 7));
	TargetPtr second;

	ASSERT_NE(first, second);

	second = first;

	ASSERT_EQ(first, second);
	ASSERT_EQ(first.get(), second.get());

	auto * ptr = first.get();
	ASSERT_TRUE(TargetClass::IsAllocated(ptr));

	first.clear();
	ASSERT_TRUE(TargetClass::IsAllocated(ptr));

	TargetPtr third(TargetPtr::make(4, 5));
	second = third;
	ASSERT_EQ(third, second);
	ASSERT_EQ(third.get(), second.get());
	ASSERT_TRUE(!TargetClass::IsAllocated(ptr));
}

// assign from a null TargetPtr
TEST(SharedPtrTestSuite, TestCopyAssignFromNull)
{
	TargetPtr first;
	TargetPtr second(TargetPtr::make(65, 61));

	auto * ptr = second.get();
	ASSERT_TRUE(TargetClass::IsAllocated(ptr));

	// assign null to a valid pointer and check that we freed mem
	second = first;
	ASSERT_TRUE(!TargetClass::IsAllocated(ptr));
	ASSERT_TRUE(!second);

	// assign null to a null pointer and check that it is still null
	TargetPtr third;
	third = first;
	ASSERT_TRUE(!third);
}

TEST(SharedPtrTestSuite, TestMoveAssign)
{
	TargetPtr first(TargetPtr::make(68, 44));
	TargetPtr second(first);
	TargetPtr third(TargetPtr::make(19, 11));

	auto * ptr = third.get();
	ASSERT_TRUE(TargetClass::IsAllocated(ptr));

	// assign to a valid pointer and check that we freed mem
	third = std::move(second);
	ASSERT_TRUE(!TargetClass::IsAllocated(ptr));
	ASSERT_EQ(first, third);
	ASSERT_EQ(first.get(), third.get());
	ASSERT_NE(third.get(), ptr);

	TargetPtr fourth;

	fourth = std::move(first);
	ASSERT_EQ(fourth, third);
	ASSERT_EQ(fourth.get(), third.get());

	ptr = third.get();
	ASSERT_TRUE(TargetClass::IsAllocated(ptr));

	third.clear();
	ASSERT_TRUE(TargetClass::IsAllocated(ptr));

	fourth.clear();
	ASSERT_TRUE(!TargetClass::IsAllocated(ptr));
}

TEST(SharedPtrTestSuite, TestMoveAssignFromNull)
{
	TargetPtr first;
	TargetPtr second(TargetPtr::make(32, 18));
	TargetPtr third;

	ASSERT_NE(first, second);
	ASSERT_NE(second, third);

	auto * ptr = second.get();
	ASSERT_TRUE(TargetClass::IsAllocated(ptr));

	second = std::move(first);
	ASSERT_TRUE(!TargetClass::IsAllocated(ptr));
	ASSERT_EQ(first, second);
	ASSERT_TRUE(!second);

	third = TargetPtr();
	ASSERT_TRUE(!third);
}
