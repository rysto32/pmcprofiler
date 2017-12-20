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

#include "ProfilerTypes.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <sys/types.h>

class Callframe;
class Image;
class ProcessExec;
class SharedString;

class AddressSpace
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
	typedef std::unordered_map<pid_t, AddressSpace*> AddressSpaceMap;
	typedef std::vector<std::unique_ptr<AddressSpace> > AddressSpaceList;

	static const pid_t KERNEL_PID = -1;

	static AddressSpaceMap addressSpaceMap;
	static AddressSpaceList addressSpaceList;

	static AddressSpace kernelAddressSpace;

	LoadableImageMap loadableImageMap;
	Image *executable;
	pid_t pid;

	void reset();

	bool isKernelSpace() const
	{
		return pid == KERNEL_PID;
	}

	static TargetAddr getLoadAddr(const std::string &executable);

public:
	AddressSpace(pid_t);

	AddressSpace(const AddressSpace&) = delete;
	AddressSpace& operator=(const AddressSpace &) = delete;

	static void clearAddressSpaces();
	static AddressSpace &getAddressSpace(bool kernel, pid_t pid);
	static AddressSpace &getKernelAddressSpace();
	static AddressSpace &getProcessAddressSpace(pid_t);
	static void processExec(const ProcessExec& ev);

	Image &getImage(TargetAddr addr, TargetAddr & loadOffset) const;

	void mapIn(TargetAddr start, const char * imagePath);
	void findAndMap(TargetAddr start, const std::vector<std::string> path,
	    const char *name);

	const Callframe & mapFrame(TargetAddr addr);

	SharedString getExecutableName() const;
};

#endif
