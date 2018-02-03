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

#include "InlineFrame.h"
#include "TestPrinter/SharedString.h"


TEST(InlineFrameSuite, TestGetters)
{
	const SharedString file("foo.cpp");
	const SharedString func("bar");
	const SharedString demangled("bar");
	const TargetAddr off = 0x1234;
	const int codeLine = 883;
	const int funcLine = 880;
	const uint64_t dwarfDieOffset = 0x18749d;
	const SharedString imageName("rstone.so.4");

	InlineFrame frame(file, func, demangled, off, codeLine, funcLine,
		dwarfDieOffset, imageName);

	EXPECT_EQ(file, frame.getFile());
	EXPECT_EQ(func, frame.getFunc());
	EXPECT_EQ(demangled, frame.getDemangled());
	EXPECT_EQ(off, frame.getOffset());
	EXPECT_EQ(codeLine, frame.getCodeLine());
	EXPECT_EQ(funcLine, frame.getFuncLine());
	EXPECT_EQ(dwarfDieOffset, frame.getDieOffset());
	EXPECT_EQ(imageName, frame.getImageName());
	ASSERT_TRUE(frame.isMapped());
}

void TestMoveConstructor()
{
	const SharedString file("rstone.cpp");
	const SharedString func("main");
	const SharedString demangled("main");
	const TargetAddr off = 0x2974;
	const int codeLine = 92;
	const int funcLine = 88;
	const uint64_t dwarfDieOffset = 0xdeadc0de;
	const SharedString imageName("dd");

	InlineFrame first(file, func, demangled, off, codeLine, funcLine,
		dwarfDieOffset, imageName);

	InlineFrame frame(std::move(first));

	EXPECT_EQ(file, frame.getFile());
	EXPECT_EQ(func, frame.getFunc());
	EXPECT_EQ(demangled, frame.getDemangled());
	EXPECT_EQ(off, frame.getOffset());
	EXPECT_EQ(codeLine, frame.getCodeLine());
	EXPECT_EQ(funcLine, frame.getFuncLine());
	EXPECT_EQ(dwarfDieOffset, frame.getDieOffset());
	EXPECT_EQ(imageName, frame.getImageName());
	ASSERT_TRUE(frame.isMapped());
}

void TestUnmapped()
{
	InlineFrame frame("", "", "", 0, -1, -1, 0, "");

	ASSERT_TRUE(!frame.isMapped());
}

