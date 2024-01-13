// Copyright (c) 2021 Ryan Stone.  All rights reserved.
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

#include "ProfilePrinter.h"

#include <gtest/gtest.h>

#include <memory>

using namespace testing;


TEST(FuncLocKeyTestSuite, TestEquals)
{
	using FuncLocKey = ProfilePrinter::FuncLocKey;

	SharedString str1("str1");
	SharedString str2("str2");

	FuncLocKey key1(str1, str2);
	FuncLocKey key2(str1, str2);

	EXPECT_EQ(key1, key2);

	FuncLocKey key3("str1", "str2");

	EXPECT_EQ(key1, key3);

	FuncLocKey key4("str1", "str3");

	EXPECT_NE(key1, key4);
}


TEST(FuncLocKeyTestSuite, TestHash)
{
	using FuncLocKey = ProfilePrinter::FuncLocKey;
	FuncLocKey::hasher hash;

	SharedString str1("str1");
	SharedString str2("str2");

	FuncLocKey key1(str1, str2);
	FuncLocKey key2(str1, str2);

	EXPECT_EQ(hash(key1), hash(key2));

	FuncLocKey key3("str1", "str2");

	EXPECT_EQ(hash(key1), hash(key3));

	FuncLocKey key4("str1", "str3");

	EXPECT_NE(hash(key1), hash(key4));
}
