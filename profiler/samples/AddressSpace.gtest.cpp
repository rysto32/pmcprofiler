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

#include "mock/GlobalMock.h"
#include "mock/MockImage.h"
#include "mock/MockImageFactory.h"
#include "mock/MockLibelf.h"
#include "mock/MockOpen.h"

#include "Callframe.h"
#include "ProcessState.h"
#include "ProfilerTypes.h"

#include "TestPrinter/SharedString.h"

#include <limits>
#include <stdio.h>

using namespace testing;

bool g_quitOnError;

class MockSyscalls : public GlobalMockBase<MockSyscalls>
{
public:
	MOCK_METHOD1(exit, void (int));
	MOCK_METHOD1(fprintf, int (FILE *));
};

extern "C" void mock_exit(int code )
{
	MockSyscalls::MockObj().exit(code);
}

extern "C" int mock_fprintf(FILE *f, const char *, ...)
{
	return MockSyscalls::MockObj().fprintf(f);
}

class AddressSpaceTestSuite : public ::testing::Test
{
public:

	void SetUp()
	{
		g_quitOnError = false;
	}

	void TearDown()
	{
	}
};

TEST_F(AddressSpaceTestSuite, TestUnknownName)
{
	MockImageFactory factory;
	CallframeList frameList;
	GlobalMockImage mockImage;

	auto * unmapImg = factory.ExpectGetUnmappedImage();
	mockImage.ExpectGetFrame(unmapImg, 0x586, frameList);

	AddressSpace space(factory);

	EXPECT_EQ(space.getExecutableName(), "<unknown>");
	EXPECT_EQ(&space.mapFrame(0x586), frameList.at(0).get());
}

TEST_F(AddressSpaceTestSuite, TestSingleMapIn)
{
	SharedString executable("/bin/init");
	MockImageFactory factory;
	CallframeList frameList;
	GlobalMockImage mockImage;

	auto * exeImg = factory.ExpectGetImage(executable);
	mockImage.ExpectGetFrame(exeImg, 0x5000, frameList);

	AddressSpace space(factory);
	space.mapIn(0x1000, executable);

	EXPECT_EQ(space.getExecutableName(), executable);
	EXPECT_EQ(&space.mapFrame(0x5000), frameList.at(0).get());
}

TEST_F(AddressSpaceTestSuite, TestMultiMapIn)
{
	SharedString exeName("/bin/dd");
	SharedString lib1Name("/lib/libc.so.7");
	SharedString lib2Name("/lib/libthr.so.3");
	SharedString lib3Name("/usr/lib/libcam.so.5");
	MockImageFactory factory;

	const TargetAddr exeLoad = 0x1000;
	const TargetAddr lib1Load = 0xfedb3210U;

	// Test a very large load address to ensure that we don't truncate any addresses
	const TargetAddr lib2Load = std::numeric_limits<TargetAddr>::max() - 0x100000;
	const TargetAddr lib3Load = 0x154a;

	CallframeList frameList;
	GlobalMockImage mockImage;

	{
		InSequence dummy;
		auto * exeImg = factory.ExpectGetImage(exeName);
		auto * lib1Img = factory.ExpectGetImage(lib1Name);
		auto * lib2Img = factory.ExpectGetImage(lib2Name);

		mockImage.ExpectGetFrame(exeImg, 0x1040, frameList);
		mockImage.ExpectGetFrame(lib2Img, 0x1955, frameList);
		mockImage.ExpectGetFrame(lib1Img, 0x293, frameList);

		// Mapping from lib3's load offset before it is loaded should
		// get a frame from the previous image (exeImg)
		mockImage.ExpectGetFrame(exeImg, lib3Load + 0x96, frameList);

		auto * lib3Img = factory.ExpectGetImage(lib3Name);

		mockImage.ExpectGetFrame(lib3Img, 0xab, frameList);

		// Test mapping frames from the boundaries between images
		// An address 1 before an image's load address should be mapped
		// out of the previous image
		mockImage.ExpectGetFrame(exeImg, lib3Load - 1, frameList);
		mockImage.ExpectGetFrame(lib3Img, 0, frameList);

		// Map frames from before all images and they should be mapped
		// from the unmapped image
		auto * unmapImg = factory.ExpectGetUnmappedImage();
		mockImage.ExpectGetFrame(unmapImg, 0x123, frameList);

		factory.ExpectGetUnmappedImage();
		mockImage.ExpectGetFrame(unmapImg, 0x321, frameList);
	}

	AddressSpace space(factory);
	space.mapIn(exeLoad, exeName);

	EXPECT_EQ(space.getExecutableName(), exeName);

	space.mapIn(lib1Load, lib1Name);
	EXPECT_EQ(space.getExecutableName(), exeName);

	space.mapIn(lib2Load, lib2Name);
	EXPECT_EQ(space.getExecutableName(), exeName);

	EXPECT_EQ(&space.mapFrame(0x1040), frameList.at(0).get());
	EXPECT_EQ(&space.mapFrame(lib2Load + 0x1955), frameList.at(1).get());
	EXPECT_EQ(&space.mapFrame(lib1Load + 0x293), frameList.at(2).get());
	EXPECT_EQ(&space.mapFrame(lib3Load + 0x96), frameList.at(3).get());

	space.mapIn(lib3Load, lib3Name);
	EXPECT_EQ(space.getExecutableName(), exeName);

	EXPECT_EQ(&space.mapFrame(lib3Load + 0xab), frameList.at(4).get());

	EXPECT_EQ(&space.mapFrame(lib3Load - 1), frameList.at(5).get());
	EXPECT_EQ(&space.mapFrame(lib3Load), frameList.at(6).get());

	EXPECT_EQ(&space.mapFrame(0x123), frameList.at(7).get());
	EXPECT_EQ(&space.mapFrame(0x321), frameList.at(8).get());
}

TEST_F(AddressSpaceTestSuite, TestFindAndMapEmptyPath)
{
	MockImageFactory factory;
	SharedString kernelName("/boot/kernel/kernel");
	SharedString kldName("hwpmc.ko");
	std::vector<std::string> modulePath;
	CallframeList frameList;
	GlobalMockImage mockImage;
	GlobalMock<MockSyscalls> mockSyscalls;

	const TargetAddr kldAddr = 0x1942;

	{
		InSequence dummy;
		auto * kernImg = factory.ExpectGetImage(kernelName);
		EXPECT_CALL(*mockSyscalls, fprintf(stderr))
		    .Times(1)
		    .WillOnce(Return(0));

		auto * unmapImg = factory.ExpectGetUnmappedImage();
		mockImage.ExpectGetFrame(kernImg, 0x100, frameList);
		mockImage.ExpectGetFrame(unmapImg, 0, frameList);
		mockImage.ExpectGetFrame(unmapImg, 0x200, frameList);
	}

	AddressSpace space(factory);
	space.mapIn(0, kernelName);
	space.findAndMap(kldAddr, modulePath, kldName);

	EXPECT_EQ(&space.mapFrame(0x100), frameList.at(0).get());
	EXPECT_EQ(&space.mapFrame(kldAddr), frameList.at(1).get());
	EXPECT_EQ(&space.mapFrame(kldAddr + 0x200), frameList.at(2).get());
}

TEST_F(AddressSpaceTestSuite, TestFindAndMap)
{
	SharedString kernelName("kernel");
	SharedString kldName1("dtrace.ko");
	SharedString kldName2("if_em.ko");
	SharedString kldName3("vendor_module.ko");
	SharedString kldName4("unknown_module.ko");
	std::vector<std::string> modulePath({"/boot/STOCK", "/boot/modules", "/boot/kernel"});

	MockImageFactory factory;
	GlobalMockImage mockImage;
	GlobalMockOpen mockOpen;
	GlobalMock<MockSyscalls> mockSyscalls;

	CallframeList frameList;

	const TargetAddr kldAddr1 = 0xf800250;
	const TargetAddr kldAddr2 = 0x3951;
	const TargetAddr kldAddr3 = 0x9871;
	const TargetAddr kldAddr4 = 0x4025;

	{
		InSequence dummy;

		int fd = 4587;
		mockOpen.ExpectOpen("/boot/STOCK/kernel", O_RDONLY, fd);
		mockOpen.ExpectClose(fd);
		auto * kernImg = factory.ExpectGetImage("/boot/STOCK/kernel");

		fd = 85;
		mockOpen.ExpectOpen("/boot/STOCK/dtrace.ko", O_RDONLY, fd);
		mockOpen.ExpectClose(fd);
		auto * kldImg1 = factory.ExpectGetImage("/boot/STOCK/dtrace.ko");

		fd = 0;
		mockOpen.ExpectOpen("/boot/STOCK/if_em.ko", O_RDONLY, -1);
		mockOpen.ExpectOpen("/boot/modules/if_em.ko", O_RDONLY, -1);
		mockOpen.ExpectOpen("/boot/kernel/if_em.ko", O_RDONLY, fd);
		mockOpen.ExpectClose(fd);
		auto * kldImg2 = factory.ExpectGetImage("/boot/kernel/if_em.ko");

		fd = INT_MAX;
		mockOpen.ExpectOpen("/boot/STOCK/vendor_module.ko", O_RDONLY, -1);
		mockOpen.ExpectOpen("/boot/modules/vendor_module.ko", O_RDONLY, fd);
		mockOpen.ExpectClose(fd);
		auto * kldImg3 = factory.ExpectGetImage("/boot/modules/vendor_module.ko");

		mockImage.ExpectGetFrame(kldImg1, 0x10, frameList);
		mockImage.ExpectGetFrame(kernImg, 0x30030, frameList);
		mockImage.ExpectGetFrame(kldImg3, 0x100, frameList);
		mockImage.ExpectGetFrame(kldImg1, 0x587, frameList);
		mockImage.ExpectGetFrame(kldImg2, 0, frameList);

		mockOpen.ExpectOpen("/boot/STOCK/unknown_module.ko", O_RDONLY, -1);
		mockOpen.ExpectOpen("/boot/modules/unknown_module.ko", O_RDONLY, -1);
		mockOpen.ExpectOpen("/boot/kernel/unknown_module.ko", O_RDONLY, -1);
		EXPECT_CALL(*mockSyscalls, fprintf(stderr))
		    .Times(1)
		    .WillOnce(Return(0));
		auto * unmapImg = factory.ExpectGetUnmappedImage();

		mockImage.ExpectGetFrame(kldImg3, 0x56, frameList);
		mockImage.ExpectGetFrame(unmapImg, 0x87, frameList);
	}

	AddressSpace space(factory);
	space.findAndMap(0x30030, modulePath, kernelName);
	space.findAndMap(kldAddr1, modulePath, kldName1);
	space.findAndMap(kldAddr2, modulePath, kldName2);
	space.findAndMap(kldAddr3, modulePath, kldName3);

	EXPECT_EQ(&space.mapFrame(kldAddr1 + 0x10), frameList.at(0).get());
	EXPECT_EQ(&space.mapFrame(0x30030), frameList.at(1).get());
	EXPECT_EQ(&space.mapFrame(kldAddr3 + 0x100), frameList.at(2).get());
	EXPECT_EQ(&space.mapFrame(kldAddr1 + 0x587), frameList.at(3).get());
	EXPECT_EQ(&space.mapFrame(kldAddr2), frameList.at(4).get());

	space.findAndMap(kldAddr4, modulePath, kldName4);

	EXPECT_EQ(&space.mapFrame(kldAddr3 + 0x56), frameList.at(5).get());
	EXPECT_EQ(&space.mapFrame(kldAddr4 + 0x87), frameList.at(6).get());
}

TEST_F(AddressSpaceTestSuite, TestProcessExecFileNotFound)
{
	MockImageFactory factory;
	ProcessExec exec(123, "/usr/bin/top", 0x53523);
	CallframeList frameList;
	GlobalMockImage mockImage;
	GlobalMockOpen mockOpen;
	const TargetAddr libLoadAddr = 0x70000;

	mockOpen.ExpectOpen("/usr/bin/top", O_RDONLY, -1);
	auto * img = factory.ExpectGetImage("/usr/bin/top");
	auto * lib = factory.ExpectGetImage("/lib/libc.so.7");
	mockImage.ExpectGetFrame(img, libLoadAddr - 1, frameList);
	mockImage.ExpectGetFrame(img, 0, frameList);
	mockImage.ExpectGetFrame(lib, 0, frameList);

	AddressSpace space(factory);
	space.processExec(exec);
	space.mapIn(libLoadAddr, "/lib/libc.so.7");
	EXPECT_EQ(&space.mapFrame(libLoadAddr - 1), frameList.at(0).get());
	EXPECT_EQ(&space.mapFrame(0), frameList.at(1).get());
	EXPECT_EQ(&space.mapFrame(libLoadAddr), frameList.at(2).get());
}

TEST_F(AddressSpaceTestSuite, TestProcessExecElfBeginFailed)
{
	MockImageFactory factory;
	ProcessExec exec(123, "/usr/bin/top", 0x53523);
	CallframeList frameList;
	GlobalMockImage mockImage;
	GlobalMock<MockLibelf> libelf;
	GlobalMockOpen mockOpen;
	const TargetAddr libLoadAddr = 0x15645;

	{
		InSequence seq;

		const int fd = 128;
		mockOpen.ExpectOpen("/usr/bin/top", O_RDONLY, 128);
		EXPECT_CALL(*libelf, elf_begin(fd, ELF_C_READ, nullptr))
		  .Times(1)
		  .WillOnce(Return(nullptr));
		EXPECT_CALL(*libelf, elf_end(nullptr))
		  .Times(1);

		auto * img = factory.ExpectGetImage("/usr/bin/top");
		auto * lib = factory.ExpectGetImage("/lib/libc.so.7");
		mockImage.ExpectGetFrame(img, libLoadAddr - 1, frameList);
		mockImage.ExpectGetFrame(img, 0, frameList);
		mockImage.ExpectGetFrame(lib, 0, frameList);
	}

	AddressSpace space(factory);
	space.processExec(exec);
	space.mapIn(libLoadAddr, "/lib/libc.so.7");
	EXPECT_EQ(&space.mapFrame(libLoadAddr - 1), frameList.at(0).get());
	EXPECT_EQ(&space.mapFrame(0), frameList.at(1).get());
	EXPECT_EQ(&space.mapFrame(libLoadAddr), frameList.at(2).get());

}

TEST_F(AddressSpaceTestSuite, TestProcessExecGetPhdrnumFailed)
{
	MockImageFactory factory;
	ProcessExec exec(123, "/usr/bin/top", 0x53523);
	CallframeList frameList;
	GlobalMockImage mockImage;
	GlobalMock<MockLibelf> libelf;
	GlobalMockOpen mockOpen;
	const TargetAddr libLoadAddr = 0x10;

	// Use an arbitrary address for the Elf cookie as it's opaque to the
	// libelf consumer.
	Elf * elf = reinterpret_cast<Elf*>(&libelf);


	{
		InSequence seq;
		const int fd = 128;
		mockOpen.ExpectOpen("/usr/bin/top", O_RDONLY, 128);
		EXPECT_CALL(*libelf, elf_begin(fd, ELF_C_READ, nullptr))
		  .Times(1)
		  .WillOnce(Return(elf));
		EXPECT_CALL(*libelf, elf_getphdrnum(elf, _))
		  .Times(1)
		  .WillOnce(Return(EINVAL));
		EXPECT_CALL(*libelf, elf_end(elf))
		  .Times(1);

		auto * img = factory.ExpectGetImage("/usr/bin/top");
		auto * lib = factory.ExpectGetImage("/lib/libc.so.7");
		mockImage.ExpectGetFrame(img, libLoadAddr - 1, frameList);
		mockImage.ExpectGetFrame(img, 0, frameList);
		mockImage.ExpectGetFrame(lib, 0, frameList);
	}

	AddressSpace space(factory);
	space.processExec(exec);
	space.mapIn(libLoadAddr, "/lib/libc.so.7");
	EXPECT_EQ(&space.mapFrame(libLoadAddr - 1), frameList.at(0).get());
	EXPECT_EQ(&space.mapFrame(0), frameList.at(1).get());
	EXPECT_EQ(&space.mapFrame(libLoadAddr), frameList.at(2).get());

}

TEST_F(AddressSpaceTestSuite, TestProcessExecZeroPhdrs)
{
	MockImageFactory factory;
	ProcessExec exec(123, "/usr/bin/top", 0x53523);
	CallframeList frameList;
	GlobalMockImage mockImage;
	GlobalMock<MockLibelf> libelf;
	GlobalMockOpen mockOpen;
	const TargetAddr libLoadAddr = std::numeric_limits<TargetAddr>::max() - 0x888;

	// Use an arbitrary address for the Elf cookie as it's opaque to the
	// libelf consumer.
	Elf * elf = reinterpret_cast<Elf*>(&libelf);


	{
		InSequence seq;
		const int fd = 128;
		mockOpen.ExpectOpen("/usr/bin/top", O_RDONLY, 128);
		EXPECT_CALL(*libelf, elf_begin(fd, ELF_C_READ, nullptr))
		  .Times(1)
		  .WillOnce(Return(elf));
		EXPECT_CALL(*libelf, elf_getphdrnum(elf, _))
		  .Times(1)
		  .WillOnce(DoAll(SetArgPointee<1>(size_t(0)), Return(0)));
		EXPECT_CALL(*libelf, elf_end(elf))
		  .Times(1);

		auto * img = factory.ExpectGetImage("/usr/bin/top");
		auto * lib = factory.ExpectGetImage("/lib/libc.so.7");
		mockImage.ExpectGetFrame(img, libLoadAddr - 1, frameList);
		mockImage.ExpectGetFrame(img, 0, frameList);
		mockImage.ExpectGetFrame(lib, 0, frameList);
	}

	AddressSpace space(factory);
	space.processExec(exec);
	space.mapIn(libLoadAddr, "/lib/libc.so.7");
	EXPECT_EQ(&space.mapFrame(libLoadAddr - 1), frameList.at(0).get());
	EXPECT_EQ(&space.mapFrame(0), frameList.at(1).get());
	EXPECT_EQ(&space.mapFrame(libLoadAddr), frameList.at(2).get());

}

TEST_F(AddressSpaceTestSuite, TestProcessExecGetPhdrFailed)
{
	MockImageFactory factory;
	ProcessExec exec(123, "/usr/bin/top", 0x53523);
	CallframeList frameList;
	GlobalMockImage mockImage;
	GlobalMock<MockLibelf> libelf;
	GlobalMockOpen mockOpen;
	const TargetAddr libLoadAddr = std::numeric_limits<TargetAddr>::min() + 0x9876;

	// Use an arbitrary address for the Elf cookie as it's opaque to the
	// libelf consumer.
	Elf * elf = reinterpret_cast<Elf*>(&libelf);


	{
		InSequence seq;
		const int fd = 128;
		mockOpen.ExpectOpen("/usr/bin/top", O_RDONLY, 128);
		EXPECT_CALL(*libelf, elf_begin(fd, ELF_C_READ, nullptr))
		  .Times(1)
		  .WillOnce(Return(elf));
		EXPECT_CALL(*libelf, elf_getphdrnum(elf, _))
		  .Times(1)
		  .WillOnce(DoAll(SetArgPointee<1>(size_t(3)), Return(0)));
		EXPECT_CALL(*libelf, gelf_getphdr(elf, 0, _))
		  .Times(1)
		  .WillOnce(Return(nullptr));
		EXPECT_CALL(*libelf, elf_end(elf))
		  .Times(1);

		auto * img = factory.ExpectGetImage("/usr/bin/top");
		auto * lib = factory.ExpectGetImage("/lib/libc.so.7");
		mockImage.ExpectGetFrame(img, libLoadAddr - 1, frameList);
		mockImage.ExpectGetFrame(img, 0, frameList);
		mockImage.ExpectGetFrame(lib, 0, frameList);
	}

	AddressSpace space(factory);
	space.processExec(exec);
	space.mapIn(libLoadAddr, "/lib/libc.so.7");
	EXPECT_EQ(&space.mapFrame(libLoadAddr - 1), frameList.at(0).get());
	EXPECT_EQ(&space.mapFrame(0), frameList.at(1).get());
	EXPECT_EQ(&space.mapFrame(libLoadAddr), frameList.at(2).get());

}

TEST_F(AddressSpaceTestSuite, TestProcessExecNoTextHdr)
{
	MockImageFactory factory;
	ProcessExec exec(123, "/usr/bin/top", 0x53523);
	CallframeList frameList;
	GlobalMockImage mockImage;
	GlobalMock<MockLibelf> libelf;
	GlobalMockOpen mockOpen;
	const TargetAddr libLoadAddr = 0x15410;

	// Use an arbitrary address for the Elf cookie as it's opaque to the
	// libelf consumer.
	Elf * elf = reinterpret_cast<Elf*>(&libelf);

	{
		InSequence seq;
		const int fd = 128;
		mockOpen.ExpectOpen("/usr/bin/top", O_RDONLY, 128);
		EXPECT_CALL(*libelf, elf_begin(fd, ELF_C_READ, nullptr))
		  .Times(1)
		  .WillOnce(Return(elf));
		EXPECT_CALL(*libelf, elf_getphdrnum(elf, _))
		  .Times(1)
		  .WillOnce(DoAll(SetArgPointee<1>(size_t(4)), Return(0)));

		GElf_Phdr phdr = (GElf_Phdr) {
			.p_type = PT_LOAD,
			.p_flags = PF_R | PF_X,
			.p_vaddr = 0x2000,
			.p_memsz = 0,

		};
		EXPECT_CALL(*libelf, gelf_getphdr(elf, 0, _))
		  .Times(1)
		  .WillOnce(DoAll(SetArgPointee<2>(phdr), ReturnArg<2>()));

		phdr = {
			.p_type = PT_DYNAMIC,
			.p_flags = PF_R | PF_X,
			.p_vaddr = 0x2000,
			.p_memsz = 0x1000,
		};
		EXPECT_CALL(*libelf, gelf_getphdr(elf, 1, _))
		  .Times(1)
		  .WillOnce(DoAll(SetArgPointee<2>(phdr), ReturnArg<2>()));

		phdr = (GElf_Phdr) {
			.p_type = PT_LOAD,
			.p_flags = PF_R | PF_W,
			.p_vaddr = 0x2000,
			.p_memsz = 0x1000,

		};
		EXPECT_CALL(*libelf, gelf_getphdr(elf, 2, _))
		  .Times(1)
		  .WillOnce(DoAll(SetArgPointee<2>(phdr), ReturnArg<2>()));

		phdr = (GElf_Phdr) {
			.p_type = PT_INTERP,
			.p_flags = PF_R | PF_X,
			.p_vaddr = 0x2000,
			.p_memsz = 0x1000,

		};
		EXPECT_CALL(*libelf, gelf_getphdr(elf, 3, _))
		  .Times(1)
		  .WillOnce(DoAll(SetArgPointee<2>(phdr), ReturnArg<2>()));

		EXPECT_CALL(*libelf, elf_end(elf))
		  .Times(1);

		auto * img = factory.ExpectGetImage("/usr/bin/top");
		auto * lib = factory.ExpectGetImage("/lib/libc.so.7");
		mockImage.ExpectGetFrame(img, libLoadAddr - 1, frameList);
		mockImage.ExpectGetFrame(img, 0, frameList);
		mockImage.ExpectGetFrame(lib, 0, frameList);
	}

	AddressSpace space(factory);
	space.processExec(exec);
	space.mapIn(libLoadAddr, "/lib/libc.so.7");
	EXPECT_EQ(&space.mapFrame(libLoadAddr - 1), frameList.at(0).get());
	EXPECT_EQ(&space.mapFrame(0), frameList.at(1).get());
	EXPECT_EQ(&space.mapFrame(libLoadAddr), frameList.at(2).get());
}

TEST_F(AddressSpaceTestSuite, TestProcessExecTextHdrPresent)
{
	MockImageFactory factory;
	ProcessExec exec(123, "/usr/bin/top", 0x53523);
	CallframeList frameList;
	GlobalMockImage mockImage;
	GlobalMock<MockLibelf> libelf;
	GlobalMockOpen mockOpen;
	const TargetAddr exeLoadAddr = 0x41000;
	const TargetAddr libLoadAddr = 0x89248;

	// Use an arbitrary address for the Elf cookie as it's opaque to the
	// libelf consumer.
	Elf * elf = reinterpret_cast<Elf*>(&libelf);

	{
		InSequence seq;
		const int fd = 128;
		mockOpen.ExpectOpen("/usr/bin/top", O_RDONLY, 128);
		EXPECT_CALL(*libelf, elf_begin(fd, ELF_C_READ, nullptr))
		  .Times(1)
		  .WillOnce(Return(elf));
		EXPECT_CALL(*libelf, elf_getphdrnum(elf, _))
		  .Times(1)
		  .WillOnce(DoAll(SetArgPointee<1>(size_t(4)), Return(0)));

		GElf_Phdr phdr = (GElf_Phdr) {
			.p_type = PT_LOAD,
			.p_flags = PF_R | PF_X,
			.p_vaddr = 0x2000,
			.p_memsz = 0,

		};
		EXPECT_CALL(*libelf, gelf_getphdr(elf, 0, _))
		  .Times(1)
		  .WillOnce(DoAll(SetArgPointee<2>(phdr), ReturnArg<2>()));

		phdr = {
			.p_type = PT_LOAD,
			.p_flags = PF_R | PF_X,
			.p_vaddr = exeLoadAddr,
			.p_memsz = 0x1000,
		};
		EXPECT_CALL(*libelf, gelf_getphdr(elf, 1, _))
		  .Times(1)
		  .WillOnce(DoAll(SetArgPointee<2>(phdr), ReturnArg<2>()));

		EXPECT_CALL(*libelf, elf_end(elf))
		  .Times(1);

		auto * img = factory.ExpectGetImage("/usr/bin/top");
		auto * lib = factory.ExpectGetImage("/lib/libc.so.7");
		mockImage.ExpectGetFrame(img, libLoadAddr - 1, frameList);
		mockImage.ExpectGetFrame(img, exeLoadAddr, frameList);

		auto * unmapped = factory.ExpectGetUnmappedImage();
		mockImage.ExpectGetFrame(unmapped, 0, frameList);

		unmapped = factory.ExpectGetUnmappedImage();
		mockImage.ExpectGetFrame(unmapped, exeLoadAddr - 1, frameList);

		mockImage.ExpectGetFrame(lib, 0, frameList);
	}

	AddressSpace space(factory);
	space.processExec(exec);
	space.mapIn(libLoadAddr, "/lib/libc.so.7");
	EXPECT_EQ(&space.mapFrame(libLoadAddr - 1), frameList.at(0).get());
	EXPECT_EQ(&space.mapFrame(exeLoadAddr), frameList.at(1).get());
	EXPECT_EQ(&space.mapFrame(0), frameList.at(2).get());
	EXPECT_EQ(&space.mapFrame(exeLoadAddr - 1), frameList.at(3).get());
	EXPECT_EQ(&space.mapFrame(libLoadAddr), frameList.at(4).get());
}
