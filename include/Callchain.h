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

#ifndef CALLCHAIN_H
#define CALLCHAIN_H

#include <vector>

#include "ProfilerTypes.h"
#include "InlineFrame.h"

class Callframe;
class CallframeMapper;
class Sample;

class Callchain
{
public:
	class CallchainRecord
	{
		TargetAddr addr;
		const Callframe &frame;

		CallchainRecord(TargetAddr a, const Callframe &f)
		  : addr(a), frame(f)
		{
		}

		friend class Callchain;
	};

	typedef std::vector<CallchainRecord> RecordChain;

private:
	const CallframeMapper &space;
	RecordChain callframes;
	std::unique_ptr<InlineFrame> selfFrame;
	size_t sampleCount;
	bool kernel;

public:
	Callchain(CallframeMapper &, const Sample &);

	Callchain(const Callchain&) = delete;
	Callchain& operator=(const Callchain &) = delete;

	Callchain(Callchain&& other) noexcept = default;

	void addSample();

	const CallframeMapper & getMapper() const
	{
		return space;
	}

	size_t getSampleCount() const
	{
		return sampleCount;
	}

	TargetAddr getAddress() const
	{
		return callframes.front().addr;
	}

	bool isKernel() const
	{
		return kernel;
	}

	const InlineFrame * getSelfFrame(const InlineFrame &);

	bool isMapped() const;

	const InlineFrame & getLeafFrame() const;
	const Callframe & getLeafCallframe() const;

	void flatten(std::vector<const InlineFrame*> &) const;
};

#endif
