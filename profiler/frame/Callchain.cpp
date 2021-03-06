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

#include "Callchain.h"

#include "AddressSpace.h"
#include "Callframe.h"
#include "CallframeMapper.h"
#include "Sample.h"

Callchain::Callchain(CallframeMapper & space, const Sample& sample)
  : space(space), sampleCount(0), kernel(sample.isKernel())
{
	for (int i = 0; i < sample.getChainDepth(); ++i) {
		TargetAddr addr = sample.getAddress(i);
		const Callframe &fr = space.mapFrame(addr);

		callframes.push_back(CallchainRecord(addr, fr));
	}
}

void
Callchain::addSample()
{
	sampleCount++;
}

void
Callchain::flatten(std::vector<const InlineFrame*> &frameList) const
{
	for (const auto & rec : callframes) {
		for (const auto & frame : rec.frame.getInlineFrames()) {
			frameList.push_back(&frame);
		}
	}
}

const InlineFrame&
Callchain::getLeafFrame() const
{
	return callframes.front().frame.getInlineFrames().front();
}

bool
Callchain::isMapped() const
{
	return !callframes.front().frame.isUnmapped();
}

const InlineFrame *
Callchain::getSelfFrame(const InlineFrame & prototype)
{
	if (selfFrame)
		return selfFrame.get();

	SharedString self("[self]");
	selfFrame = std::make_unique<InlineFrame>(prototype.getFile(),
	    self, self, prototype.getOffset(), prototype.getCodeLine(),
	    prototype.getFuncLine(), 0, getLeafFrame().getImageName());

	return selfFrame.get();
}
