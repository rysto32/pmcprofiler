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

#include "Disassembler.h"

#include "BufferSampleFactory.h"
#include "Callframe.h"
#include "Demangle.h"
#include "DwarfCompileUnitDie.h"
#include "DwarfDieStack.h"
#include "DwarfLocation.h"
#include "DwarfSrcLine.h"
#include "DwarfSrcLinesList.h"
#include "DwarfUtil.h"
#include "MapUtil.h"
#include "VariableLookup.h"

#include <gelf.h>

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
// 	LOG("Add leaf symbol covering %lx-%lx\n", src.GetAddr(), nextAddr);
	AddDwarfSymbol(list, src.GetAddr(), nextAddr, src.GetFile(imageFile), src.GetLine(),
	    "", GetDieOffset(cu.GetDie()));
}

void
DwarfSearch::FillLeafSymbols(const DwarfDieRanges & ranges, DwarfLocationList &list)
{

	LOG("*** Start scan of srclines\n");
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
DwarfSearch::MapFramesToSubprograms(const FrameList & frameList, FrameList &unmapped)
{
	for (auto frame : frameList) {
		auto it = subprograms.Lookup(frame->getOffset());
		if (it == subprograms.end()) {
			LOG("Frame %lx mapped to no subroutine (assembly?)\n",
			    frame->getOffset());
			unmapped.push_back(frame);
			continue;
		}

		LOG("Frame %lx mapped to subprogram die %lx\n", frame->getOffset(),
		    GetDieOffset(*it->second.GetValue()));
		it->second.AddFrame(frame);
	}

}

void
DwarfSearch::MapFrames(const FrameList& frameList)
{
	FrameList assemblyFuncs;
	MapFramesToSubprograms(frameList, assemblyFuncs);

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

void
DwarfSearch::MapTypes(BufferSampleFactory & factory, Elf_Scn *textSection,
    const GElf_Shdr & textHdr, const FrameList & frameList)
{
	FrameList unmapped;
	MapFramesToSubprograms(frameList, unmapped);

	for (auto & [addr, value] : subprograms) {
		if (value.GetFrames().empty())
			continue;

		MapSubprogramTypes(factory, textSection, textHdr, addr,
		    *value.GetValue(), value.GetFrames());
	}
}

void
DwarfSearch::MapSubprogramTypes(BufferSampleFactory & factory, Elf_Scn *textSection,
    const GElf_Shdr & textHdr, TargetAddr symAddr, Dwarf_Die subprogram,
    const FrameList& frameList)
{
	VariableLookup vars(dwarf);

	DwarfDieStack stack(imageFile, dwarf, cu, subprogram);
	stack.FillSubprogramVars(vars);

	const DwarfCompileUnitParams & params = cu.GetParams();
	Elf_Data * textData = FindElfData(textSection, symAddr);

	if (textData == nullptr) {
		for (Callframe * fr : frameList) {
			fr->SetBufferSample(factory.GetUnknownSample(), 0, 1);
		}
		return;
	}

	Disassembler disasm(textHdr, textData);

	disasm.InitFunc(symAddr);

	for (Callframe * frame : frameList) {
		MemoryOffset off(disasm.GetInsnOffset(frame->getOffset()));
		if (!off.IsDefined()) {
			frame->SetBufferSample(factory.GetUnknownSample(), 0, 1);
			continue;
		}

		DwarfDie typeDie = vars.FindRegType(off.GetReg(), frame->getOffset());

		if (!typeDie) {
			frame->SetBufferSample(factory.GetUnknownSample(), 0, 1);
			continue;
		}

		BufferSample * sample = factory.GetSample(dwarf, params, typeDie);
		frame->SetBufferSample(sample, off.GetOffset(), off.GetAccessSize());
	}
}

Elf_Data *
DwarfSearch::FindElfData(Elf_Scn *textSection, TargetAddr symAddr)
{
	GElf_Shdr header;
	Elf_Data *data;

	if (gelf_getshdr(textSection, &header) == NULL)
		return (NULL);

	data = NULL;
	while ((data = elf_rawdata(textSection, data)) != NULL) {
		//fprintf(stderr, "sh_addr: %lx off: %lx size: %lx\n", header.sh_addr, data->d_off, data->d_size);
		GElf_Addr addr = header.sh_addr + data->d_off;
		if (addr <= symAddr &&
		   ((addr + data->d_size) > symAddr))
			return (data);
	}

	return (nullptr);
}
