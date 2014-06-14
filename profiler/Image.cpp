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
#include "Process.h"

#include <cxxabi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <paths.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/linker.h>
#include <sys/sysctl.h>
#include <unistd.h>

void Image::parseModulePath(char * path_buf, std::vector<std::string> & vec)
{
	char * path;
	char * next = path_buf;

	vec.clear();

	while ((path = strsep(&next, ";")) != NULL) {
		if (*path == '\0')
			continue;

		vec.push_back(path);
	}
}

std::vector<std::string> Image::getModulePath()
{
	std::vector<std::string> vec;
	char * path_buf;
	size_t path_len = 0;
	int error;

	sysctlbyname("kern.module_path", NULL, &path_len, NULL, 0);
	path_buf = new char[path_len+1];

	error = sysctlbyname("kern.module_path", path_buf, &path_len, NULL, 0);
	if (error == 0)
		parseModulePath(path_buf, vec);

	delete [] path_buf;

	return vec;
}

std::string Image::KERNEL_NAME(getbootfile());
std::vector<std::string> Image::MODULE_PATH(Image::getModulePath());
const std::string Image::TEXT_SECTION_NAME(".text");

Image* Image::kernelImage = 0;

Image::ImageMap Image::imageMap;
Image::LoadableImageMap Image::kernelLoadableImageMap;

void
Image::setBootfile(const char * file)
{

	KERNEL_NAME = file;

	delete kernelImage;
	kernelImage = NULL;
}

void
Image::setModulePath(char * path)
{

	parseModulePath(path, MODULE_PATH);
}

Image&
Image::getKernel()
{

	if (Image::kernelImage == NULL)
		kernelImage = new Image(KERNEL_NAME);
	return *kernelImage;
}

void
Image::freeImages()
{
	delete kernelImage;
	kernelImage = NULL;

	kernelLoadableImageMap.clear();

	Image::ImageMap::iterator it;
	for (it = imageMap.begin(); it != imageMap.end(); ++it)
		delete it->second;

	imageMap.clear();
}

Image::Image(const std::string& imageName)
  : m_lookup(imageName)
{
}

const std::string &
Image::getImageFile() const
{

	return (m_lookup.getImageFile());
}

bool
Image::isContained(const Location& location, uintptr_t loadOffset)
{
	uintptr_t addr;

	addr = location.getAddress() - loadOffset;
	return m_lookup.isContained(addr);
}

void
Image::mapLocation(Location& location, uintptr_t loadOffset)
{
	uintptr_t address = location.getAddress() - loadOffset;

	location.isMapped(m_lookup.LookupLine(address, location.m_filename,
					      location.m_functionname, location.m_linenumber));
}

void
Image::functionStart(Location& location, uintptr_t loadOffset)
{
	uintptr_t address = location.getAddress() - loadOffset;

	location.isMapped(m_lookup.LookupFunc(address, location.m_filename,
					      location.m_functionname, location.m_linenumber));
}

void
Image::mapFunctionStart(FunctionLocation& functionLocation)
{
	uintptr_t loadOffset;
	Image *image;

	functionLocation.m_isMapped = false;

	image = findImage(functionLocation, loadOffset);
	if (image == NULL || !image->isContained(functionLocation))
		return;

	image->functionStart(functionLocation, loadOffset);
	if (functionLocation.m_isMapped) {
		if (functionLocation.m_lineLocationList.empty())
			functionLocation.m_linenumber = -1;
		else
			functionLocation.m_linenumber = *functionLocation.m_lineLocationList.begin();
	}
}

bool
Image::findKldModule(const char * kldName, std::string & kldPath)
{
	int fd;

	std::vector<std::string>::const_iterator it = MODULE_PATH.begin();
	for (; it != MODULE_PATH.end(); ++it) {
		std::string path = *it + '/' + kldName;
		fd = open(path.c_str(), O_RDONLY);

		if (fd >= 0) {
			close(fd);
			kldPath = path;
			return true;
		}
	}

	return false;
}

void
Image::loadKldImage(uintptr_t loadAddress, const char * moduleName)
{
	std::string kldPath;

	if (findKldModule(moduleName, kldPath))
		kernelLoadableImageMap[loadAddress] = kldPath;
	else {
		fprintf(stderr, "%s: unable to find kld module %s\n",
			g_quitOnError ? "error" : "warning", moduleName);
		if (g_quitOnError)
			exit(5);
	}
}

std::string
Image::getLoadableImageName(const Location& location, uintptr_t& loadOffset)
{

	loadOffset = 0;

	LoadableImageMap::iterator it = kernelLoadableImageMap.lower_bound(location.getAddress());

	if (it == kernelLoadableImageMap.begin())
		return "";

	--it;
	loadOffset = it->first;
	return it->second;
}

Image *
Image::findImage(const Location &location, uintptr_t &loadOffset)
{

	loadOffset = 0;
	if (location.m_isKernel) {
		if (getKernel().isContained(location)) {
			loadOffset = 0;
			return (&getKernel());
		} else {
			const std::string& loadableImageName(Image::getLoadableImageName(location, loadOffset));

			Image* kernelModuleImage = imageMap[loadableImageName];
			if (kernelModuleImage == NULL) {
				kernelModuleImage = new Image(loadableImageName);
				imageMap[loadableImageName] = kernelModuleImage;
			}
			return (kernelModuleImage);
		}
	} else {
		Process* process = Process::getProcess(location.m_pid);
		if (process == NULL)
			return (NULL);

		const std::string& processName(process->getName());
		Image* processImage = imageMap[processName];
		if (processImage == NULL)
			processImage = imageMap[processName] = new Image(processName);

		if (processImage->isContained(location))
			return (processImage);
		else {
			const std::string& loadableImageName(
				process->getLoadableImageName(location, loadOffset));

			Image* processLoadableImage = imageMap[loadableImageName];
			if (processLoadableImage == 0) {
				processLoadableImage = imageMap[loadableImageName] = new Image(loadableImageName);
			}

			return (processLoadableImage);
		}
	}
}

void
Image::mapAllLocations(LocationList& locationList)
{
	uintptr_t loadOffset;

	for (LocationList::iterator it = locationList.begin(); it != locationList.end(); ++it) {
		std::vector<Location> & stack(*it);
		for (std::vector<Location>::iterator jt = stack.begin(); jt != stack.end(); ++jt) {
			Location& location(*jt);

			Image *image = findImage(location, loadOffset);

			if (image == NULL)
				continue;

			if (image->isContained(location, loadOffset)) {
				image->mapLocation(location, loadOffset);
				if (location.m_isMapped)
					location.m_modulename = image->getImageFile().c_str();
			}
		}
	}
}

char*
Image::demangle(const std::string &name)
{
	char *demangled;
	size_t len;
	int status;

	/*
	 * abi::__cxa_demangle doesn't work on all non-mangled symbol names,
	 * so do a hacky test for a mangled name before trying to demangle it.
	 */
	if (name.substr(0, 2) != "_Z")
		return (strdup(name.c_str()));

	len = 0;
	demangled = (abi::__cxa_demangle(name.c_str(), NULL, &len, &status));

	if (demangled == NULL)
		return (strdup(name.c_str()));

	return (demangled);
}
