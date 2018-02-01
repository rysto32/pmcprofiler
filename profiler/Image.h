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

#if !defined(IMAGE_H)
#define IMAGE_H

#include "DwarfResolver.h"
#include "ProfilerTypes.h"
#include "SharedString.h"

#include <string>
#include <algorithm>
#include <functional>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

class Callframe;
class ProcessExec;
class FunctionLocation;
class ImageFactory;
class Location;
class SharedString;

class Image
{
private:
	SharedString imageFile;
	FrameMap frameMap;

	explicit Image(SharedString imageName);

	Image() = delete;
	Image(const Image&) = delete;
	Image(Image&&) = delete;
	Image& operator=(const Image &) = delete;
	Image& operator=(Image &&) = delete;

	friend class ImageFactory;

public:
	~Image();

	const SharedString & GetImageFile() const
	{
		return imageFile;
	}

	const Callframe & GetFrame(TargetAddr offset);
	void MapAllFrames();
	void MapAllAsUnmapped();
};

#endif // #if !defined(IMAGE_H)
