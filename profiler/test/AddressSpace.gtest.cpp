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

#include "AddressSpace.h"

#include "mock/MockImage.h"
#include "mock/MockImageFactory.h"
#include "mock/MockLibelf.h"
#include "mock/MockOpen.h"

#include "Callframe.h"
#include "ProfilerTypes.h"

#include "TestPrinter/SharedString.h"

#include <stdio.h>

using namespace testing;

bool g_quitOnError;

extern "C" void mock_exit(int) {}
extern "C" int mock_fprintf(FILE *, const char *, ...) { return (0); }

class AddressSpaceTestSuite : public ::testing::Test
{
public:

	void SetUp()
	{
		MockOpen::SetUp();
		MockImage::SetUp();
	}

	void TearDown()
	{
		MockImage::TearDown();
		MockOpen::TearDown();
	}
};

TEST_F(AddressSpaceTestSuite, TestUnknownName)
{
	MockImageFactory factory;

	AddressSpace space(factory);

	ASSERT_EQ(space.getExecutableName(), "<unknown>");
}

TEST_F(AddressSpaceTestSuite, TestSingleMapIn)
{
	SharedString executable("/bin/init");
	MockImageFactory factory;
	factory.ExpectGetImage(executable);

	AddressSpace space(factory);
	space.mapIn(0x1000, executable);

	ASSERT_EQ(space.getExecutableName(), executable);
}

TEST_F(AddressSpaceTestSuite, TestMultiMapIn)
{
	SharedString exeName("/bin/dd");
	SharedString lib1Name("/lib/libc.so.7");
	SharedString lib2Name("/lib/libthr.so.3");
	SharedString lib3Name("/usr/lib/libcam.so.5");
	MockImageFactory factory;

	{
		InSequence dummy;
		factory.ExpectGetImage(exeName);
		factory.ExpectGetImage(lib1Name);
		factory.ExpectGetImage(lib2Name);
		factory.ExpectGetImage(lib3Name);
	}

	AddressSpace space(factory);
	space.mapIn(0x1000, exeName);

	ASSERT_EQ(space.getExecutableName(), exeName);

	space.mapIn(0xfedb3210U, lib1Name);
	ASSERT_EQ(space.getExecutableName(), exeName);

	space.mapIn(0xed98, lib2Name);
	ASSERT_EQ(space.getExecutableName(), exeName);

	space.mapIn(0x1554a, lib3Name);
	ASSERT_EQ(space.getExecutableName(), exeName);
}

TEST_F(AddressSpaceTestSuite, TestFindAndMapEmptyPath)
{
	MockImageFactory factory;
	SharedString kernelName("/boot/kernel/kernel");
	SharedString kldName("hwpmc.ko");
	std::vector<std::string> modulePath;

	{
		InSequence dummy;
		factory.ExpectGetImage(kernelName);
	}

	AddressSpace space(factory);
	space.mapIn(0, kernelName);
	space.findAndMap(0x1942, modulePath, kldName);
}

TEST_F(AddressSpaceTestSuite, TestFindAndMap)
{
	MockImageFactory factory;
	SharedString kernelName("kernel");
	SharedString kldName1("dtrace.ko");
	SharedString kldName2("if_em.ko");
	SharedString kldName3("vendor_module.ko");
	std::vector<std::string> modulePath({"/boot/STOCK", "/boot/modules", "/boot/kernel"});

	{
		InSequence dummy;

		int fd = 4587;
		MockOpen::ExpectOpen("/boot/STOCK/kernel", O_RDONLY, fd);
		MockOpen::ExpectClose(fd);
		factory.ExpectGetImage("/boot/STOCK/kernel");

		fd = 85;
		MockOpen::ExpectOpen("/boot/STOCK/dtrace.ko", O_RDONLY, fd);
		MockOpen::ExpectClose(fd);
		factory.ExpectGetImage("/boot/STOCK/dtrace.ko");

		fd = 1286;
		MockOpen::ExpectOpen("/boot/STOCK/if_em.ko", O_RDONLY, -1);
		MockOpen::ExpectOpen("/boot/modules/if_em.ko", O_RDONLY, -1);
		MockOpen::ExpectOpen("/boot/kernel/if_em.ko", O_RDONLY, fd);
		MockOpen::ExpectClose(fd);
		factory.ExpectGetImage("/boot/kernel/if_em.ko");

		fd = 246;
		MockOpen::ExpectOpen("/boot/STOCK/vendor_module.ko", O_RDONLY, -1);
		MockOpen::ExpectOpen("/boot/modules/vendor_module.ko", O_RDONLY, fd);
		MockOpen::ExpectClose(fd);
		factory.ExpectGetImage("/boot/modules/vendor_module.ko");
	}

	AddressSpace space(factory);
	space.findAndMap(0x30030, modulePath, kernelName);
	space.findAndMap(0xf800250, modulePath, kldName1);
	space.findAndMap(0x3951, modulePath, kldName2);
	space.findAndMap(0x9871, modulePath, kldName3);
}
