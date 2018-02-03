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

#include "Callframe.h"

#include "TestPrinter/SharedString.h"

TEST(CallframeTestSuite, GetOffset)
{
	Callframe fr(10, "arg");
	ASSERT_EQ(fr.getOffset(), 10);

	Callframe fr2(0xffffffff8069d5feUL, "arg2");
	ASSERT_EQ(fr2.getOffset(), 0xffffffff8069d5feUL);
}

TEST(CallframeTestSuite, Unmapped)
{
	Callframe fr(34, "libc.so.7");
	fr.setUnmapped();

	ASSERT_TRUE(fr.isUnmapped());

	const auto & frames = fr.getInlineFrames();
	ASSERT_EQ(frames.size(), 1);
	ASSERT_EQ(frames[0].getFile(), "libc.so.7");
	ASSERT_EQ(frames[0].getOffset(), 34);
	ASSERT_TRUE(!frames[0].isMapped());
}

TEST(CallframeTestSuite, AddSingleFrame)
{
	Callframe cf(0xffffffff80cba260UL, "/boot/kernel/kernel");

	cf.addFrame("netinet/tcp_input.c", "tcp_input", "tcp_input",
		3015, 2693, 0xa45d2);

	ASSERT_TRUE(!cf.isUnmapped());

	const auto & frames = cf.getInlineFrames();
	ASSERT_EQ(frames.size(), 1);

	const auto & fr = frames.at(0);
	ASSERT_EQ(fr.getFile(), "netinet/tcp_input.c");
	ASSERT_EQ(fr.getFunc(), "tcp_input");
	ASSERT_EQ(fr.getDemangled(), "tcp_input");
	ASSERT_EQ(fr.getCodeLine(), 3015);
	ASSERT_EQ(fr.getFuncLine(), 2693);
	ASSERT_EQ(fr.getDieOffset(), 0xa45d2);
	ASSERT_EQ(fr.getOffset(), 0xffffffff80cba260UL);
	ASSERT_TRUE(fr.isMapped());
}

TEST(CallframeTestSuite, AddMultipleFrames)
{
	Callframe cf(0xa893b, "pmcprofiler");

	cf.addFrame("/usr/include/c++/map",
		"_ZNKSt3__116__tree_node_baseIPvE15__parent_unsafeEv",
		"std::__1::__tree_node_base<void*>::__parent_unsafe() const",
		751, 742, 0xf3a7);
	cf.addFrame("/usr/include/c++/map",
		"_ZNSt3__127__tree_balance_after_insertIPNS_16__tree_node_baseIPvEEEEvT_S5_",
		"void std::__1::__tree_balance_after_insert<std::__1::__tree_node_base<void*>*>(std::__1::__tree_node_base<void*>*, std::__1::__tree_node_base<void*>*)",
		284, 279, 0x7932);
	cf.addFrame("Image.cpp", "_ZN5Image6mapAllEv", "Image::mapAll()",
		230, 217, 0x68718);

	ASSERT_TRUE(!cf.isUnmapped());

	const auto & frames = cf.getInlineFrames();
	ASSERT_EQ(frames.size(), 3);

	const auto * fr = &frames.at(0);
	ASSERT_EQ(fr->getFile(), "/usr/include/c++/map");
	ASSERT_EQ(fr->getFunc(), "_ZNKSt3__116__tree_node_baseIPvE15__parent_unsafeEv");
	ASSERT_EQ(fr->getDemangled(), "std::__1::__tree_node_base<void*>::__parent_unsafe() const");
	ASSERT_EQ(fr->getCodeLine(), 751);
	ASSERT_EQ(fr->getFuncLine(), 742);
	ASSERT_EQ(fr->getDieOffset(), 0xf3a7);
	ASSERT_EQ(fr->getOffset(), 0xa893b);
	ASSERT_TRUE(fr->isMapped());

	fr = &frames.at(1);
	ASSERT_EQ(fr->getFile(), "/usr/include/c++/map");
	ASSERT_EQ(fr->getFunc(), "_ZNSt3__127__tree_balance_after_insertIPNS_16__tree_node_baseIPvEEEEvT_S5_");
	ASSERT_EQ(fr->getDemangled(), "void std::__1::__tree_balance_after_insert<std::__1::__tree_node_base<void*>*>(std::__1::__tree_node_base<void*>*, std::__1::__tree_node_base<void*>*)");
	ASSERT_EQ(fr->getCodeLine(), 284);
	ASSERT_EQ(fr->getFuncLine(), 279);
	ASSERT_EQ(fr->getDieOffset(), 0x7932);
	ASSERT_EQ(fr->getOffset(), 0xa893b);
	ASSERT_TRUE(fr->isMapped());

	fr = &frames.at(2);
	ASSERT_EQ(fr->getFile(), "Image.cpp");
	ASSERT_EQ(fr->getFunc(), "_ZN5Image6mapAllEv");
	ASSERT_EQ(fr->getDemangled(), "Image::mapAll()");
	ASSERT_EQ(fr->getCodeLine(), 230);
	ASSERT_EQ(fr->getFuncLine(), 217);
	ASSERT_EQ(fr->getDieOffset(), 0x68718);
	ASSERT_EQ(fr->getOffset(), 0xa893b);
	ASSERT_TRUE(fr->isMapped());
}
