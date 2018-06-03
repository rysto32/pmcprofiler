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

#ifndef MOCK_MOCK_IMAGE_H
#define MOCK_MOCK_IMAGE_H

#include <gmock/gmock.h>

#include "mock/GlobalMock.h"

#include "Callframe.h"
#include "Image.h"

typedef std::vector<std::unique_ptr<Callframe>> CallframeList;

class MockImage : public GlobalMockBase<MockImage>
{
public:
	MOCK_METHOD2(GetFrame, const Callframe & (Image *, TargetAddr offset));
};

class GlobalMockImage : public GlobalMock<MockImage>
{
public:
	void ExpectGetFrame(Image *image, TargetAddr offset, CallframeList & frameList)
	{
		frameList.emplace_back(std::make_unique<Callframe>(offset, image->GetImageFile()));
		EXPECT_CALL(**this, GetFrame(image, offset))
		  .Times(1)
		  .WillOnce(testing::ReturnRef(*frameList.back()));
	}
};

const Callframe &
Image::GetFrame(TargetAddr offset)
{
	return MockImage::MockObj().GetFrame(this, offset);
}

Image::Image(SharedString n)
  : imageFile(n)
{}

Image::~Image() {}

#endif
