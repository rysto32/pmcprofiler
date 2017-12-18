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

#ifndef FUNCTIONLOCATION_H
#define FUNCTIONLOCATION_H

#include "Location.h"

#include <set>

class Image;

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
			return std::hash<std::string>()(*loc.getFunctionName());
		}
	};

	bool operator==(const FunctionLocation & other) const
	{
		return (getFunctionName() == other.getFunctionName());
	}
};

#endif
