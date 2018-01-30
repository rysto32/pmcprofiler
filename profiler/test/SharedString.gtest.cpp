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

#include "SharedString.h"

#include "MapUtil.h"
#include "TestPrinter/SharedString.h"

#include <new>
#include <map>

typedef std::map<const void *, size_t> AllocMap;
static AllocMap ranges;

extern "C" void * mock_new(size_t sz)
{
	void * ptr = operator new(sz);;

	try {
		ranges.insert(std::make_pair(ptr, sz));
	} catch (...) {
		::operator delete(ptr);
		throw std::bad_alloc();
	}

	return ptr;
}

extern "C" void mock_delete(void * ptr)
{
	ranges.erase(ptr);
	::operator delete(ptr);
}

template<typename T>
bool IsAllocated(T * t)
{
	auto ptr = static_cast<const void*>(t);
	auto it = LastSmallerThan(ranges, ptr);
	if (it == ranges.end())
		return false;

	return it->first <= ptr && ptr < (static_cast<const char*>(it->first) + it->second);
}

TEST(SharedStringTestSuite, TestPointsTo)
{
	SharedString str("myString");

	ASSERT_EQ(str->length(), 8);
	ASSERT_TRUE(!str->empty());
	ASSERT_EQ(str->at(1), 'y');
}

TEST(SharedStringTestSuite, TestDereference)
{
	std::string origString("anotherString");
	SharedString str(origString);

	ASSERT_EQ(origString, *str);

	const std::string & ref = *str;
	ASSERT_EQ(ref.find("Str"), 7);
}

TEST(SharedStringTestSuite, TestDefaultConstructor)
{
	SharedString str;

	ASSERT_TRUE(str->empty());
	ASSERT_TRUE(str == "");
}

TEST(SharedStringTestSuite, TestCStringConstructor)
{
	const char *cstr = "this";
	SharedString str(cstr);

	ASSERT_EQ(*str, cstr);
}

TEST(SharedStringTestSuite, TestStdStringConstructor)
{
	std::string std("standard");
	SharedString str(std);

	ASSERT_EQ(*str, std);

	// Ensure that the SharedString has allocated its own memory
	ASSERT_NE(&*str, &std);
}

TEST(SharedStringTestSuite, TestSharedStringCopyConstructor)
{
	SharedString orig("origString");
	SharedString newStr(orig);

	ASSERT_EQ(orig, newStr);

	// Ensure that we did a shallow copy
	ASSERT_EQ(&*orig, &*newStr);

	const auto * origMem = &*orig;

	std::string origStr(*orig);

	// Ensure that modifiying the original doesn't affect the copy
	orig.clear();
	ASSERT_TRUE(orig->empty());
	ASSERT_TRUE(!newStr->empty());
	ASSERT_NE(orig, newStr);
	ASSERT_EQ(*newStr, origStr);
	ASSERT_TRUE(IsAllocated(origMem));
}

TEST(SharedStringTestSuite, TestSharedStringMoveConstructor)
{
	std::string orig("firstStr");
	SharedString first(orig);
	SharedString second(std::move(first));

	ASSERT_EQ(orig, *second);

	first = orig;
	ASSERT_EQ(first, second);
}

TEST(SharedStringTestSuite, TestInterning)
{
	const char *cStr = "a c string";
	std::string std(cStr);
	const char cStrArr[] = "a c string";

	SharedString first(cStr);
	SharedString second(std);

	ASSERT_EQ(first, second);
	ASSERT_EQ(&*first, &*second);

	SharedString third = cStrArr;
	ASSERT_EQ(first, third);
	ASSERT_EQ(&*first, &*third);

	const auto * origMem = &*first;

	second = "something else";

	ASSERT_NE(first, second);
	ASSERT_NE(&*first, &*second);
	ASSERT_TRUE(IsAllocated(origMem));

	// Release all references to origMem and assert that it has been
	// freed (note: this is a bit fragile -- we depend on the memory
	// not be reallocated, which is only guaranteed to happen
	// because third is a reference counted copy of second, so we
	// won't allocate more memory when third releases the reference)
	first = "";
	third = second;
	ASSERT_TRUE(!IsAllocated(origMem));
}

TEST(SharedStringTestSuite, TestCopyAssignment)
{
	const char *cStr = "frist";
	SharedString first("garbage");
	const std::string * data;

	{
		SharedString second(cStr);
		data = &*second;

		// Use first to ensure the optimizer doesn't get any
		// clever ideas and elides the object.
		ASSERT_NE(first, second);

		first = second;
		ASSERT_EQ(first, second);
		ASSERT_EQ(*first, cStr);
		ASSERT_EQ(&*first, &*second);
	}

	// Ensure that first is still valid after second was destructed
	ASSERT_EQ(*first, cStr);
	ASSERT_TRUE(IsAllocated(data));
}

TEST(SharedStringTestSuite, TestMoveAssignment)
{
	const char *cStr = "frist";
	SharedString first("garbage");
	SharedString second;
	const std::string * data;

	{
		SharedString third(cStr);
		second = third;
		data = &*second;

		// Use first to ensure the optimizer doesn't get any
		// clever ideas and elides the object.
		ASSERT_NE(first, third);

		first = std::move(third);
		ASSERT_EQ(first, second);
		ASSERT_EQ(*first, cStr);
		ASSERT_EQ(&*first, &*second);
	}

	// Ensure that first is still valid after second was destructed
	ASSERT_EQ(*first, cStr);
	ASSERT_EQ(first, second);
	ASSERT_TRUE(IsAllocated(data));
}
