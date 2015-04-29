// Copyright (c) 2014 Sandvine Incorporated.  All rights reserved.
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

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include "DwarfRange.h"

#include <assert.h>
#include <stdlib.h>

DwarfRange::DwarfRange(DwarfLocation &loc)
  : m_location(loc),
    m_caller(NULL),
    m_inline_depth(1)
{
}

int
DwarfRange::GetInlineDepth() const
{

	return (m_inline_depth);
}

DwarfLocation &
DwarfRange::GetLocation() const
{

	return (m_location);
}

DwarfRange *
DwarfRange::GetCaller() const
{

	return (m_caller);
}

DwarfRange *
DwarfRange::GetOutermostCaller()
{
	DwarfRange *range, *next;

	range = this;
	while(1) {
		next = range->GetCaller();
		if (next == NULL)
			return (range);
		range = next;
	}
}

void
DwarfRange::SetCaller(DwarfRange *caller)
{

	/*
	 * A previous call may have already set caller as the caller for
	 * this range.  If so, skip setting it a second time to avoid
	 * introducing a cycle in the call graph here.
	 */
	if (caller == this)
		return;
	else if (m_caller == NULL)
		m_caller = caller;
	else
		m_caller->SetCaller(caller);
	m_inline_depth = m_caller->GetInlineDepth() + 1;
}

