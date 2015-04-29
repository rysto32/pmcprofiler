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

class Image;

class Location
{
	friend              class Image;

	uintptr_t           m_address;
	bool                m_isKernel;
	bool                m_isMapped;
	unsigned            m_count;
	pid_t               m_pid;
	std::string         m_modulename;
	std::string         m_filename;
	std::string         m_functionname;
	unsigned            m_linenumber;

public:
	Location(bool isAKernel, unsigned pid, uintptr_t address, unsigned count) :
	m_address(address),
	m_isKernel(isAKernel),
	m_isMapped(false),
	m_count(count),
	m_pid(pid),
	m_modulename(),
	m_filename(),
	m_functionname(),
	m_linenumber(-1)
	{
	}

	uintptr_t getAddress() const
	{
		return m_address;
	}

	bool isKernelImage() const
	{
		return m_isKernel;
	}

	bool isMapped() const
	{
		return m_isMapped;
	}

	void isMapped(bool mapped)
	{
		m_isMapped = mapped;
	}

	unsigned getCount() const
	{
		return m_count;
	}

	pid_t getPid() const
	{
		return m_pid;
	}

	const std::string &getModuleName() const
	{
		return (m_modulename);
	}

	const std::string &getFileName() const
	{
		return (m_filename);
	}

	const std::string &getFunctionName() const
	{
		return (m_functionname);
	}

	unsigned getLineNumber() const
	{
		return m_linenumber;
	}

	bool operator<(const Location& other) const
	{
		return m_count > other.m_count;
	}

	void setFunctionName(const char * name)
	{
		m_functionname = name;
	}
};

typedef std::set<unsigned> LineLocationList;

class FunctionLocation : public Location
{
	friend class Image;
	unsigned m_totalCount;
	LineLocationList m_lineLocationList;

public:
	FunctionLocation(const Location& location)
	  : Location(location),
	    m_totalCount(location.getCount())
	{
		m_lineLocationList.insert(location.getLineNumber());
	}

	FunctionLocation& operator+=(const Location& location)
	{
		m_totalCount += location.getCount();
		m_lineLocationList.insert(location.getLineNumber());

		return *this;
	}

	FunctionLocation& operator+=(const FunctionLocation & location)
	{
		m_totalCount += location.getCount();
		m_lineLocationList.insert(location.m_lineLocationList.begin(),
		    location.m_lineLocationList.end());

		return *this;
	}

	bool operator<(const FunctionLocation& other) const
	{
		return m_totalCount > other.m_totalCount;
	}

	unsigned getCount() const
	{
		return m_totalCount;
	}

	LineLocationList& getLineLocationList()
	{
		return m_lineLocationList;
	}

	struct hasher
	{
		size_t operator()(const FunctionLocation & loc) const
		{
			return std::hash<std::string>()(loc.getFunctionName());
		}
	};

	bool operator==(const FunctionLocation & other) const
	{
		return (getFunctionName() == other.getFunctionName());
	}
};

class ProcessExec;
class FunctionLocation;
class Location;

class Callchain
{
	std::vector<std::string> vec;
	mutable size_t hash_value;
	mutable bool hash_valid;

public:

	Callchain()
	  : hash_valid(false)
	{
	}

	size_t hash() const
	{
		if (hash_valid)
			return hash_value;

		std::vector<std::string>::const_iterator it = vec.begin();
		size_t val = 0;
		std::hash<std::string> hasher;

		/*
		 * The default hasher for strings multiplies the hash
		 * value by 5 for each iteration, so we follow that hash
		 * function here.
		 *
		 * We essentially act as if we have concatenated all of
		 * the strings in vec together and hashed that one
		 * string.
		 */
		for (; it != vec.end(); ++it)
			val = 5 * val + hasher(*it);

		hash_value = val;
		hash_valid = true;

		return val;
	}

	void push_back(const std::string& t)
	{
		vec.push_back(t);
		hash_valid = false;
	}

	void pop_back()
	{
		vec.pop_back();
		hash_valid = false;
	}

	const std::string & back()
	{
		return vec.back();
	}

	bool operator==(const Callchain & other) const
	{
		return vec == other.vec;
	}

	struct Hasher
	{
		size_t operator()(const Callchain & v) const
		{
			return v.hash();
		}
	};

	typedef std::vector<std::string>::iterator iterator;

	iterator begin()
	{
		return vec.begin();
	}

	iterator end()
	{
		return vec.end();
	}
};

typedef std::unordered_map<std::string, FunctionLocation> FunctionLocationMap;
typedef std::unordered_map<Callchain, FunctionLocationMap, Callchain::Hasher> CallchainMap;
typedef std::vector<FunctionLocation> FunctionList;
typedef std::vector<std::vector<Location> > LocationList;

class Image
{

	typedef std::map<std::string, Image*> ImageMap;
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

	const std::string & getImageFile() const;
};

#endif // #if !defined(IMAGE_H)
