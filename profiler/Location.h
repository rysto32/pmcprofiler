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

#ifndef LOCATION_H
#define LOCATION_H

#include "ProfilerTypes.h"
#include "SharedString.h"

class Image;
class Process;

class Location
{
	friend class Image;

	TargetAddr m_address;
	bool m_isKernel;
	bool m_isMapped;
	unsigned m_count;
	SharedString m_modulename;
	SharedString m_filename;
	SharedString m_functionname;
	unsigned m_linenumber;

	Process *m_process;

public:
	Location(bool isAKernel, TargetAddr address, unsigned count,
	    Process *process) :
	m_address(address),
	m_isKernel(isAKernel),
	m_isMapped(false),
	m_count(count),
	m_modulename(),
	m_filename(),
	m_functionname(),
	m_linenumber(-1),
	m_process(process)
	{
	}

	TargetAddr getAddress() const
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

	Process * getProcess() const
	{
		return m_process;
	}

	const SharedString &getModuleName() const
	{
		return (m_modulename);
	}

	const SharedString &getFileName() const
	{
		return (m_filename);
	}

	const SharedString &getFunctionName() const
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

	void setFunctionName(SharedString name)
	{
		m_functionname = name;
	}
};

#endif
