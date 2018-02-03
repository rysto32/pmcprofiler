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

#include "Callframe.h"

#include "InlineFrame.h"
#include "SharedString.h"

Callframe::Callframe(TargetAddr off, SharedString imageName)
  : offset(off), imageName(imageName), unmapped(false)
{
}

// This is defined for the benefit of a unit test that wants to intercept
// calls to ~Callframe() to track Callframe lifecycles
Callframe::~Callframe()
{
}

void
Callframe::addFrame(SharedString file, SharedString func,
     SharedString demangled, int codeLine, int funcLine, uint64_t dwarfDieOffset)
{
	inlineFrames.emplace_back(file, func, demangled, offset, codeLine,
	    funcLine, dwarfDieOffset, imageName);
}


void
Callframe::setUnmapped()
{
	SharedString unmapped_function("[unmapped_function]");
	inlineFrames.clear();
	inlineFrames.emplace_back(imageName, unmapped_function,
	    unmapped_function, offset, -1, -1, 0, imageName);

	unmapped = true;
}
