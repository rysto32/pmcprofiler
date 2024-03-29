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
#include "Demangle.h"
#include "DwarfCompileUnitDie.h"
#include "DwarfDieStack.h"
#include "DwarfLocation.h"
#include "DwarfSrcLine.h"
#include "DwarfSrcLinesList.h"
#include "DwarfUtil.h"
#include "MapUtil.h"

DwarfSearch::DwarfSearch(Dwarf_Debug dwarf, const DwarfCompileUnitDie &cu,
    SharedString imageFile, const SymbolMap & symbols)
  : imageFile(imageFile),
    dwarf(dwarf),
    srcLines(dwarf, cu.GetDie()),
    srcIt(srcLines.begin()),
    cu(cu),
    symbols(symbols)
{
	DwarfDieStack stack(imageFile, dwarf, cu, cu.GetDie());
	stack.EnumerateSubprograms(subprograms);
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

	/* WTF LLVM? */
	if (src.GetAddr() == nextAddr)
		return;
// 	LOG("Add leaf symbol covering %lx-%lx", src.GetAddr(), nextAddr);
	AddDwarfSymbol(list, src.GetAddr(), nextAddr, src.GetFile(imageFile), src.GetLine(),
	    "", GetDieOffset(cu.GetDie()));
}

void
DwarfSearch::FillLeafSymbols(const DwarfDieRanges & ranges, DwarfLocationList &list)
{

	LOG("*** Start scan of srclines");
	while (srcIt != srcLines.end()) {
		DwarfSrcLine src(*srcIt);
		if (ranges.Succeeds(src.GetAddr())) {
			++srcIt;
			continue;
		}

		if (!ranges.Contains(src.GetAddr()))
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
		frame.setUnmapped();
		return;
	}

	SharedString func("[unmapped_function]");
	auto it = LastSmallerThan(symbols, frame.getOffset());
	if (it != symbols.end())
		func = it->second;

	frame.addFrame(file, func, func, line, line, GetDieOffset(cu.GetDie()));
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
			frame.setUnmapped();
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
		SharedString demangled = Demangle(func);
		frame.addFrame(ptr->GetFile(), func, demangled,
			ptr->GetCodeLine(), ptr->GetFuncLine(),
			ptr->GetDwarfDieOffset());
		ptr = next;
	}
}

void
DwarfSearch::MapFrames(const FrameList& frameList)
{
	FrameList assemblyFuncs;
	for (auto frame : frameList) {
		auto it = subprograms.Lookup(frame->getOffset());
		if (it == subprograms.end()) {
			LOG("Frame %lx mapped to no subroutine (assembly?)",
			    frame->getOffset());
			assemblyFuncs.push_back(frame);
			continue;
		}

		LOG("Frame %lx mapped to subprogram die %lx", frame->getOffset(),
		    GetDieOffset(*it->second.GetValue()));
		it->second.AddFrame(frame);
	}

	for (auto & [addr, value] : subprograms) {
		if (value.GetFrames().empty())
			continue;

		MapSubprogram(*value.GetValue(), value.GetFrames());
	}

	srcIt = srcLines.begin();
	for (auto frame : assemblyFuncs) {
		MapAssembly(*frame);
	}
}

void
DwarfSearch::MapSubprogram(Dwarf_Die subprogram, const FrameList& frameList)
{
	DwarfLocationList list;

	DwarfDieRanges ranges(dwarf, subprogram, cu);
	DwarfDieStack stack(imageFile, dwarf, cu, subprogram);
	stack.FillSubprogramSymbols(list, ranges);
	FillLeafSymbols(ranges, list);

	for (auto frame : frameList) {
		MapFrame(*frame, list);
	}
}
