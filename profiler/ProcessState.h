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

#if !defined(PROCESSSTATE_H)
#define PROCESSSTATE_H

#include "ProfilerTypes.h"

#include <string>
#include <sys/types.h>

class ProcessState
{
	pid_t m_processID;

	const std::string& m_processName;

protected:
	ProcessState(pid_t processID, const std::string& processName)
	  : m_processID(processID),
	    m_processName(processName)
	{
	}

	ProcessState(const ProcessState&) = delete;
	ProcessState& operator=(const ProcessState &) = delete;

public:
	pid_t getProcessID() const
	{
		return m_processID;
	}

	const std::string& getProcessName() const
	{
		return m_processName;
	}
};

class ProcessExec : public ProcessState
{
private:
	TargetAddr entryAddr;

public:
	ProcessExec(pid_t processID, const std::string& processName, TargetAddr addr)
	  : ProcessState(processID, processName), entryAddr(addr)
	{
	}

	TargetAddr getEntryAddr() const
	{
		return entryAddr;
	}
};

class ProcessExit : public ProcessState
{
public:
	ProcessExit(pid_t& processID)
	  : ProcessState(processID, "")
	{
	}
};

#endif // #if !defined(PROCESSSTATE_H)
