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

#include "Image.h"

#include "Callframe.h"
#include "DefaultImageFactory.h"
#include "BufferSampleFactory.h"

#include "mock/GlobalMock.h"

#include <gtest/gtest.h>
#include <libdwarf.h>
#include <unordered_set>

using namespace testing;

class CallframeMock : public GlobalMockBase<CallframeMock>
{
public:
	MOCK_METHOD2(Construct, void (TargetAddr, SharedString));
	MOCK_METHOD1(Destruct, void (const Callframe*));
	MOCK_METHOD1(setUnmapped, void(const Callframe*));
};

Callframe::Callframe(TargetAddr addr, SharedString name)
{
	CallframeMock::MockObj().Construct(addr, name);
}

Callframe::~Callframe()
{
	CallframeMock::MockObj().Destruct(this);
}

void Callframe::setUnmapped()
{
	CallframeMock::MockObj().setUnmapped(this);
}

class DwarfResolverMock : public GlobalMockBase<DwarfResolverMock>
{
public:
	MOCK_METHOD1(Construct, void (SharedString));
	MOCK_METHOD1(Resolve, void (const FrameMap &));
};

DwarfResolver::DwarfResolver(SharedString name)
{
	DwarfResolverMock::MockObj().Construct(name);
}

// Leave this as a stub because we don't care when the resolver is destructed
DwarfResolver::~DwarfResolver() {}

void
DwarfResolver::Resolve(const FrameMap &frames)
{
	DwarfResolverMock::MockObj().Resolve(frames);
}

void
DwarfResolver::ResolveTypes(const FrameMap &frames, BufferSampleFactory &)
{
}

class BufferSampleFactoryStub : public BufferSampleFactory
{
public:
	virtual BufferSample * GetSample(Dwarf_Debug dwarf,
	    const DwarfCompileUnitParams & params, const DwarfDie & type)
	{
		abort();
	}

	virtual BufferSample * GetUnknownSample()
	{
		abort();
	}

	virtual void GetSamples(std::vector<BufferSample*> & list) const
	{
		abort();
	}
};

void dwarf_dealloc(Dwarf_Debug, Dwarf_Ptr, Dwarf_Unsigned) {}

TEST(ImageTestSuite, TestGetters)
{
	BufferSampleFactoryStub bufFactory;
	DefaultImageFactory factory(bufFactory);
	Image *img = factory.GetImage("/bin/testprog");

	EXPECT_EQ(img->GetImageFile(), "/bin/testprog");
}

TEST(ImageTestSuite, TestGetFrame)
{
	GlobalMock<CallframeMock> cfMock;
	SharedString execname("/usr/bin/dtrace");

	EXPECT_CALL(*cfMock, Construct(0x123, execname))
	  .Times(1);
	EXPECT_CALL(*cfMock, Construct(0x824, execname))
	  .Times(1);
	EXPECT_CALL(*cfMock, Construct(0, execname))
	  .Times(1);

	{
		BufferSampleFactoryStub bufFactory;
		DefaultImageFactory factory(bufFactory);
		Image *img = factory.GetImage(execname);
		std::vector<const Callframe *> frameList;

		frameList.push_back(&img->GetFrame(0x123));
		frameList.push_back(&img->GetFrame(0x824));
		frameList.push_back(&img->GetFrame(0));

		// The Callframes should be destructed as the image is destroyed,
		// when the factory goes out of scope.
		for (const Callframe * fr : frameList)
			EXPECT_CALL(*cfMock, Destruct(fr))
			    .Times(1);
	}
}

TEST(ImageTestSuite, TestGetFrameTwice)
{
	GlobalMock<CallframeMock> cfMock;
	SharedString execname("/usr/sbin/rtsold");

	EXPECT_CALL(*cfMock, Construct(0x89d, execname))
	  .Times(1);
	EXPECT_CALL(*cfMock, Construct(0xfe9, execname))
	  .Times(1);

	{
		BufferSampleFactoryStub bufFactory;
		DefaultImageFactory factory(bufFactory);
		Image *img = factory.GetImage(execname);
		std::vector<const Callframe *> frameList;

		frameList.push_back(&img->GetFrame(0x89d));
		frameList.push_back(&img->GetFrame(0xfe9));

		EXPECT_EQ(frameList.at(0), &img->GetFrame(0x89d));
		EXPECT_EQ(frameList.at(1), &img->GetFrame(0xfe9));

		// The Callframes should be destructed as the image is destroyed,
		// when the factory goes out of scope.
		for (const Callframe * fr : frameList)
			EXPECT_CALL(*cfMock, Destruct(fr))
			    .Times(1);
	}

}

// Call MapAll() on an Image with no callframes and ensure that it never wastes
// time constructing a DwarfResolver (as the constructor is quite expensive).
TEST(ImageTestSuite, TestMapEmptyImage)
{
	GlobalMock<CallframeMock> cfMock;
	GlobalMock<DwarfResolverMock> resolverMock;

	SharedString execname("./a.out");

	BufferSampleFactoryStub bufFactory;
	DefaultImageFactory factory(bufFactory);
	Image *img = factory.GetImage(execname);

	img->MapAllFrames();
}

class FrameListMatcher
{
private:
	std::unordered_set<const Callframe *> frameList;

public:
	FrameListMatcher(const std::unordered_set<const Callframe *> & list)
	  : frameList(list)
	{
	}

	typedef bool result_type;
	result_type operator()(const FrameMap & frameMap) const
	{
		for (auto & [addr, frame] : frameMap) {
			if (frameList.count(frame.get()) == 0)
				return false;
		}

		return true;
	}
};

TEST(ImageTestSuite, TestMapAll)
{
	GlobalMock<CallframeMock> cfMock;
	GlobalMock<DwarfResolverMock> resolverMock;
	SharedString execname("/usr/bin/lld");

	EXPECT_CALL(*cfMock, Construct(0x4ed82, execname))
	  .Times(1);

	{
		BufferSampleFactoryStub bufFactory;
		DefaultImageFactory factory(bufFactory);
		Image *img = factory.GetImage(execname);
		std::unordered_set<const Callframe *> frameList;

		frameList.insert(&img->GetFrame(0x4ed82));

		EXPECT_CALL(*resolverMock, Construct(execname))
		    .Times(1);
		EXPECT_CALL(*resolverMock, Resolve(
			ResultOf(FrameListMatcher(frameList), true)))
		    .Times(1);
// 		EXPECT_CALL(*resolverMock, Resolve(_))
// 		    .Times(1);

		img->MapAllFrames();

		// The Callframes should be destructed as the image is destroyed,
		// when the factory goes out of scope.
		for (const Callframe * fr : frameList)
			EXPECT_CALL(*cfMock, Destruct(fr))
			    .Times(1);
	}

}

TEST(ImageTestSuite, TestMapAllAsUnMapped)
{
	GlobalMock<CallframeMock> cfMock;
	SharedString execname("/sbin/ifconfig");

	EXPECT_CALL(*cfMock, Construct(0x4419, execname))
	  .Times(1);
	EXPECT_CALL(*cfMock, Construct(0x5850, execname))
	  .Times(1);
	EXPECT_CALL(*cfMock, Construct(0x2881, execname))
	  .Times(1);

	{
		BufferSampleFactoryStub bufFactory;
		DefaultImageFactory factory(bufFactory);
		Image *img = factory.GetImage(execname);
		std::unordered_set<const Callframe *> frameList;

		frameList.insert(&img->GetFrame(0x4419));
		frameList.insert(&img->GetFrame(0x5850));
		frameList.insert(&img->GetFrame(0x2881));

		ExpectationSet setUnmappedExpect;
		for (const Callframe * fr : frameList) {
			setUnmappedExpect += EXPECT_CALL(*cfMock, setUnmapped(fr))
			    .Times(1);
		}

		for (const Callframe * fr : frameList) {
			EXPECT_CALL(*cfMock, Destruct(fr))
			    .Times(1)
			    .After(setUnmappedExpect);
		}

		img->MapAllAsUnmapped();
	}
}
