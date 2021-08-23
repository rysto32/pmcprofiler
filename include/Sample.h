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

#if !defined(SAMPLE_H)
#define SAMPLE_H

#include <sys/types.h>
#include <sys/param.h>
#include <pmclog.h>

#include <unordered_map>

#include <algorithm>
#include <limits>
#include <vector>

class Sample
{
	bool m_isKernel;
	pid_t m_processID;
	std::vector<uintptr_t> m_address;

	static bool IsKernel(uintptr_t pc)
	{
		return pc > std::numeric_limits<uint32_t>::max();
	}

public:
	Sample(const pmclog_ev_callchain & event)
	  : m_isKernel(IsKernel(event.pl_pc[0])),
	m_processID(event.pl_pid)
	{
		uint32_t i;
		for (i = 0; i < event.pl_npc; i++) {
			/*
			 * Callchains can now include kernel code and the
			 * backtrace of the userland code that called into
			 * the kernel.  That isn't supported yet, so cut off
			 * the backtrace at the transition from kernel to user.
			 */
			if (IsKernel(event.pl_pc[i]) != m_isKernel)
				break;
			m_address.push_back(event.pl_pc[i]-1);
		}
	}

	Sample(const pmclog_ev_pcsample & event)
	  : m_isKernel(event.pl_usermode == 0),
	m_processID(event.pl_pid)
	{
		m_address.push_back(event.pl_pc-1);
	}

	bool isKernel() const
	{
		return m_isKernel;
	}

	unsigned getProcessID() const
	{
		return m_processID;
	}

	uintptr_t getAddress(size_t index) const
	{
		return m_address[index];
	}

	int getChainDepth() const
	{
		return m_address.size();
	}

	bool operator==(const Sample & other) const
	{
		if (m_isKernel != other.m_isKernel)
			return false;

		if (m_processID != other.m_processID)
			return false;

		if (getChainDepth() != other.getChainDepth())
			return false;

		for (int i = 0; i < getChainDepth(); i++) {
			if (m_address[i] != other.m_address[i])
				return false;
		}

		return true;
	}

	struct hash
	{
		size_t operator()(const Sample & sample) const
		{
			int i;
			size_t val = sample.isKernel();
			for (i = 0; i < sample.getChainDepth(); i++)
				val += sample.getAddress(i);

			return val;
		}
	};

};

#endif // #if !defined(SAMPLE_H)
