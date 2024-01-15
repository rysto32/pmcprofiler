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

#include "AddressSpace.h"

#include "Callframe.h"
#include "Image.h"
#include "ImageFactory.h"
#include "MapUtil.h"
#include "ProcessState.h"

#include <elf.h>
#include <gelf.h>
#include <fcntl.h>
#include <unistd.h>

AddressSpace::AddressSpace(ImageFactory &imgFactory)
  : imgFactory(imgFactory), executable(NULL)
{
}

TargetAddr
AddressSpace::getLoadAddr(const std::string &executable)
{
	Elf *elf = NULL;
	TargetAddr addr;
	int fd, error;

	addr = 0;

	fd = open(executable.c_str(), O_RDONLY);
	if (fd < 0)
		return (0);

	elf = elf_begin(fd, ELF_C_READ, NULL);
	if (elf == NULL)
		goto cleanup;

	size_t phnum;
	error = elf_getphdrnum(elf, &phnum);
	if (error != 0)
		goto cleanup;

	for (size_t i = 0; i < phnum; ++i) {
		GElf_Phdr phdr;
		if (gelf_getphdr(elf, i, &phdr) == NULL)
			goto cleanup;

		if (phdr.p_type != PT_LOAD)
			continue;

		if (phdr.p_memsz == 0)
			continue;

		if ((phdr.p_flags & PF_X) == 0)
			continue;

		/*
		 * XXX detect PIE and work out the load address
		 * the load address should be computable by looking at
		 * the entry point in the ELF header and comparing that
		 * to the actual entry point given to hwpmc
		 */
		addr = phdr.p_vaddr & (-phdr.p_align);
		goto cleanup;
	}

cleanup:
	elf_end(elf);
	return (addr);
}

void
AddressSpace::processExec(const ProcessExec& ev)
{
	TargetAddr loadAddr = getLoadAddr(ev.getProcessName());
	mapIn(loadAddr, ev.getProcessName().c_str());
}

Image &
AddressSpace::getImage(TargetAddr addr, TargetAddr & loadOffset) const
{
	auto it = LastSmallerThan(loadableImageMap, addr);
	if (it == loadableImageMap.end()) {
		loadOffset = 0;
		return imgFactory.GetUnmappedImage();
	}

	loadOffset = it->second.loadOffset;
	return *it->second.image;
}

void
AddressSpace::mapIn(TargetAddr start, SharedString imagePath)
{
	Image *image = imgFactory.GetImage(imagePath);
	mapImage(start, image);
}

void
AddressSpace::mapImage(TargetAddr start, Image* image)
{
	TargetAddr loadOffset;

	/*
	 * The first image that we need in every address space
	 * should be the executable.  They are mapped differently
	 * from libraries/modules and we have to use an offset of
	 * 0 to find symbols in the executable.
	 */
	if (loadableImageMap.empty()) {
		loadOffset = 0;
		executable = image;
	} else {
		loadOffset = start - getLoadAddr(*image->GetImageFile());
	}

	LOG("%p: Loaded %s at offset %lx", this, image->GetImageFile()->c_str(), start);
	loadableImageMap.insert(std::make_pair(start,
	    LoadedImage(image, loadOffset)));
}

void
AddressSpace::findAndMap(TargetAddr start, const std::vector<std::string> path,
    SharedString name)
{
	for (const auto & pathComp : path) {
		std::string path(pathComp);
		path += '/';
		path += *name;

		int fd = open(path.c_str(), O_RDONLY);
		if (fd >= 0) {
			close(fd);
			mapIn(start, path.c_str());
			return;
		}
	}

	fprintf(stderr, "%s: unable to find file %s\n",
		g_quitOnError ? "error" : "warning", name->c_str());
	if (g_quitOnError)
		exit(5);

	mapImage(start, &imgFactory.GetUnmappedImage());
}


const Callframe &
AddressSpace::mapFrame(TargetAddr addr)
{
	TargetAddr loadOffset, imageOffset;

	Image &image = getImage(addr, loadOffset);

	imageOffset = addr - loadOffset;

	return image.GetFrame(imageOffset);
}

SharedString
AddressSpace::getExecutableName() const
{
	if (executable == NULL)
		return "<unknown>";
	else
		return executable->GetImageFile();
}
