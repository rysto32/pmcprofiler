// Copyright (c) 2009-2014 Sandvine Incorporated.  All rights reserved.
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

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include "Image.h"

#include "Callframe.h"
#include "DwarfResolver.h"
#include "SharedString.h"

Image::Image(SharedString imageName)
  : imageFile(imageName), mapped(!imageName->empty())
{
}

Image::~Image()
{

}

const Callframe &
Image::getFrame(TargetAddr offset)
{
	auto it = frameMap.find(offset);
	if (it != frameMap.end())
		return *it->second;

	auto ptr = std::make_unique<Callframe>(offset, imageFile);
	Callframe & frame = *ptr;
	frameMap.insert(std::make_pair(offset, std::move(ptr)));
	if (!mapped)
		frame.setUnmapped();
	return frame;
}

void
Image::mapAllFrames()
{
	if (frameMap.empty())
		return;

	DwarfResolver resolver(imageFile);
	resolver.Resolve(frameMap);
}
