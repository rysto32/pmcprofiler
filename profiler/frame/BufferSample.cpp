// Copyright (c) 2018 Ryan Stone.
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

#include "BufferSample.h"

#include "TargetType.h"

BufferSample::BufferSample(const TargetType & type)
  : samples(type.GetSize(), 0),
    type(type),
    unknownSamples(0),
    totalSamples(0),
    firstAccessOffset(0)
{
}

void
BufferSample::AddSamples(size_t offset, size_t width, size_t numSamples)
{

	totalSamples += numSamples;

	for (size_t i = 0; i < width; ++i) {
		size_t byteOff = offset + i;
		if (byteOff >= samples.size()) {
			unknownSamples += numSamples;
			return;
		}
		samples.at(byteOff) += numSamples;
	}
}

size_t
BufferSample::GetNumSamples(size_t offset, size_t size) const
{
	size_t max = 0;
	size_t end = std::min(offset + size, samples.size());

	for (size_t i = offset; i < end; ++i) {
		size_t sample = samples.at(i);
		max = std::max(max, sample);
	}

	return max;
}
