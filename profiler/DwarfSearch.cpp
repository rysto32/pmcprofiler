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

#include "DwarfSearch.h"

#include "Callframe.h"
#include "DwarfSrcLine.h"

DwarfSearch::DwarfSearch(Dwarf_Debug dwarf, Dwarf_Die cu, SharedString imageFile,
    const SymbolMap & symbols)
  : imageFile(imageFile),
    stack(imageFile, dwarf, cu, symbols),
    srcLines(dwarf, cu),
    srcIt(srcLines.begin())
{

}

bool
DwarfSearch::FindLeaf(const Callframe & frame, SharedString &file, int &line)
{

	while (srcIt != srcLines.end()) {
		auto nextIt = srcIt;
		++nextIt;

		DwarfSrcLine next(*nextIt);

		if (frame.getOffset() < next.GetAddr()) {
			DwarfSrcLine src(*srcIt);
			file = src.GetFile(imageFile);
			line = src.GetLine();
			return true;
		}

		srcIt = nextIt;
	}

	return false;
}

bool
DwarfSearch::AdvanceAndMap(Callframe &frame)
{
	SharedString file;
	int line;

	if (FindLeaf(frame, file, line)) {
		return stack.AdvanceAndMap(frame, file, line);
	} else {
		frame.setUnmapped(imageFile);
	}

	return false;
}
