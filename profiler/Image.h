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

#include "DwarfLookup.h"
#include "ProfilerTypes.h"
#include "FunctionLocation.h"
#include "Location.h"
#include "SharedString.h"
#include "StringChain.h"

#include <string>
#include <algorithm>
#include <functional>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>

#include <dwarf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

extern bool g_quitOnError;
extern bool g_includeTemplates;

class Image;
class Process;

class ProcessExec;
class FunctionLocation;
class Location;

typedef std::unordered_map<std::string, FunctionLocation> FunctionLocationMap;
typedef std::unordered_map<Callchain, FunctionLocationMap, Callchain::Hasher> CallchainMap;
typedef std::vector<FunctionLocation> FunctionList;
typedef std::vector<std::vector<Location> > LocationList;

class Image
{

	typedef std::unordered_map<std::string, Image*> ImageMap;
	typedef std::map<uintptr_t, std::string> LoadableImageMap;

	static std::string KERNEL_NAME;
	static std::vector<std::string> MODULE_PATH;
	static const std::string TEXT_SECTION_NAME;
	static Image* kernelImage;
	static ImageMap imageMap;
	static LoadableImageMap kernelLoadableImageMap;

	DwarfLookup m_lookup;

	static Image& getKernel();

	Image(const std::string& imageName);

	bool isContained(const Location& location, uintptr_t loadOffset = 0);
	void mapLocation(const Location& location, std::vector<Location> & stack, uintptr_t loadOffset = 0);
	void functionStart(Location& location, uintptr_t loadOffset);
	static Image *findImage(const Location &location, uintptr_t &loadOffset);

	static bool findKldModule(const char * kldName, std::string & kldPath);
	static std::vector<std::string> getModulePath();
	static void parseModulePath(char * path_buf, std::vector<std::string> & vec);

public:
	static char* demangle(const std::string &name);
	static std::string getLoadableImageName(const Location& location, uintptr_t& loadOffset);
	static void mapAllLocations(LocationList& locationList);

	static void freeImages();

	static void mapFunctionStart(FunctionLocation& functionLocation);

	static void loadKldImage(uintptr_t loadAddress, const char * moduleName);

	static void setBootfile(const char * file);
	static void setModulePath(char * path);

	const SharedString & getImageFile() const;
};

#endif // #if !defined(IMAGE_H)
