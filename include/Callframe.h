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

#ifndef CALLFRAME_H
#define CALLFRAME_H

#include <vector>

#include "InlineFrame.h"
#include "ProfilerTypes.h"
#include "SharedString.h"

class Callframe
{
	TargetAddr offset;
	SharedString imageName;
	std::vector<InlineFrame> inlineFrames;
	bool unmapped;

public:
	Callframe(TargetAddr off, SharedString imageName);
	~Callframe();

	Callframe(const Callframe&) = delete;
	Callframe& operator=(const Callframe &) = delete;

	Callframe(Callframe &&) noexcept = default;

	void addFrame(SharedString file, SharedString func,
	    SharedString demangled, int codeLine, int funcLine,
	    uint64_t dwarfDieOffset);
	void setUnmapped();

	TargetAddr getOffset() const
	{
		return offset;
	}

	const std::vector<InlineFrame> & getInlineFrames() const
	{
		return inlineFrames;
	}

	bool isUnmapped() const
	{
		return unmapped;
	}
};

#endif
