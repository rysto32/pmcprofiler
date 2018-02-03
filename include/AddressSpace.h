// Copyright (c) 2009-2014 Sandvine Incorporated.  All rights reserved.
// Copyright (c) 2017 Ryan Stone.  All rights reserved.
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

#ifndef ADDRESSSPACE_H
#define ADDRESSSPACE_H

#include "CallframeMapper.h"
#include "ProfilerTypes.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <sys/types.h>

class Callframe;
class Image;
class ImageFactory;
class ProcessExec;
class SharedString;

class AddressSpace : public CallframeMapper
{
private:
	struct LoadedImage
	{
		Image *image;
		TargetAddr loadOffset;

		LoadedImage(Image *i, TargetAddr off)
		  : image(i), loadOffset(off)
		{
		}
	};

	typedef std::map<TargetAddr, LoadedImage> LoadableImageMap;

	ImageFactory &imgFactory;
	LoadableImageMap loadableImageMap;
	Image *executable;

	static TargetAddr getLoadAddr(const std::string &executable);

	Image &getImage(TargetAddr addr, TargetAddr & loadOffset) const;
	void mapImage(TargetAddr addr, Image *image);

public:
	AddressSpace(ImageFactory &imgFactory);
	virtual ~AddressSpace() = default;

	AddressSpace(const AddressSpace&) = delete;
	AddressSpace& operator=(const AddressSpace &) = delete;

	void mapIn(TargetAddr start, SharedString imagePath);
	void findAndMap(TargetAddr start, const std::vector<std::string> path,
	    SharedString name);

	const Callframe & mapFrame(TargetAddr addr);
	void processExec(const ProcessExec& ev);

	SharedString getExecutableName() const;
};

#endif
