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
#include "DwarfLocation.h"
#include "DwarfSrcLine.h"
#include "DwarfUtil.h"
#include "Image.h"
#include "MapUtil.h"

DwarfSearch::DwarfSearch(Dwarf_Debug dwarf, Dwarf_Die cu, SharedString imageFile,
    const SymbolMap & symbols)
  : imageFile(imageFile),
    stack(imageFile, dwarf, cu),
    srcLines(dwarf, cu),
    srcIt(srcLines.begin()),
    cuDie(cu),
    symbols(symbols)
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

void
DwarfSearch::AddLeafSymbol(DwarfLocationList &list, const DwarfSrcLine & src,
    const DwarfSrcLinesList::const_iterator &nextIt)
{
	TargetAddr nextAddr;
	if (nextIt != srcLines.end()) {
		DwarfSrcLine next(*nextIt);
		nextAddr = next.GetAddr();
	} else {
		nextAddr = 0;
	}
// 	LOG("Add leaf symbol covering %lx-%lx\n", src.GetAddr(), nextAddr);
	AddDwarfSymbol(list, src.GetAddr(), nextAddr, src.GetFile(imageFile), src.GetLine(),
	    imageFile, GetDieOffset(cuDie));
}

void
DwarfSearch::FillLeafSymbols(DwarfLocationList &list)
{

	LOG("*** Start scan of srclines\n");
	while (srcIt != srcLines.end()) {
		DwarfSrcLine src(*srcIt);
		if (stack.SubprogramSucceeds(src.GetAddr())) {
			++srcIt;
			continue;
		}

		if (!stack.SubprogramContains(src.GetAddr()))
			break;

		auto nextIt = srcIt;
		++nextIt;
		AddLeafSymbol(list, src, nextIt);
		srcIt = std::move(nextIt);
	}
}

void
DwarfSearch::MapAssembly(Callframe &frame)
{
	SharedString file;
	int line;

	if (!FindLeaf(frame, file, line)) {
		frame.setUnmapped(imageFile);
		return;
	}

	SharedString func("[unmapped_function]");
	auto it = LastSmallerThan(symbols, frame.getOffset());
	if (it != symbols.end())
		func = it->second;

	frame.addFrame(file, func, func, line, -1, GetDieOffset(cuDie));
}

void
DwarfSearch::MapFrame(Callframe & frame, const DwarfLocationList &list)
{
	auto it = LastSmallerThan(list, frame.getOffset());
	assert (it != list.end());

	/*
	 * This can arise from a weird situation where a sample
	 * somehow points at a padding NOP.  Back up until we
	 * find a section covered by valid debug information.
	 */
	while (!it->second->GetCaller()) {
		if (it == list.begin()) {
			frame.setUnmapped(imageFile);
			return;
		}

		--it;
	}


	DwarfLocationPtr ptr = it->second;
	while (1) {
		DwarfLocationPtr next = ptr->GetCaller();
		if (!next)
			break;

		SharedString func = next->GetCallee();
		SharedString demangled = Image::demangle(func);
		frame.addFrame(ptr->GetFile(), func, demangled,
			ptr->GetCodeLine(), ptr->GetFuncLine(),
			ptr->GetDwarfDieOffset());
		ptr = next;
	}
}

void
DwarfSearch::AdvanceAndMap(FrameMap::const_iterator & fit,
    const FrameMap::const_iterator &fend)
{
	bool found = stack.AdvanceToSubprogram(*fit->second);
	if (!found) {
		MapAssembly(*fit->second);
		++fit;
		return;
	}

	FrameMap::const_iterator last = fit;
	++last;

	size_t count = 1;
	while (last != fend && stack.SubprogramContains(last->second->getOffset())) {
		++count;
		++last;
	}

	DwarfLocationList list;

	stack.FillSubprogramSymbols(list);
	FillLeafSymbols(list);

	while (fit != last) {
		MapFrame(*fit->second, list);
		++fit;
	}
}
