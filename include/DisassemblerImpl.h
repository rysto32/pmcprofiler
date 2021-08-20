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

#ifndef DISASSEMBLER_IMPL_H
#define DISASSEMBLER_IMPL_H


#include "llvm/ADT/Triple.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"

#include <libelf.h>
#include <gelf.h>
#include <memory>

#include "Disassembler.h"
#include "MemoryOffset.h"
#include "ProfilerTypes.h"

class DisassemblerImpl : public Disassembler
{
	std::string llvmTripleStr;
	const llvm::Target *target;
	std::unique_ptr<llvm::MCSubtargetInfo> sti;
	std::unique_ptr<const llvm::MCRegisterInfo> mri;
	std::unique_ptr<const llvm::MCAsmInfo> mai;
	std::unique_ptr<llvm::MCContext> ctx;
	std::unique_ptr<llvm::MCInstrInfo> mcii;
	std::unique_ptr<llvm::MCInstPrinter> mcip;
	std::unique_ptr<const llvm::MCDisassembler> disasm;

	llvm::ArrayRef<uint8_t> textBuf;
	TargetAddr textOffset;

	llvm::MCInst inst;
	TargetAddr nextInstOffset;

	void DisassembleNext();
	MemoryOffset DecodeInst();

	int LlvmRegToDwarf(int);

public:
	DisassemblerImpl(const GElf_Shdr & textHdr, Elf_Data *data);

	void InitFunc(TargetAddr funcStart) override;
	MemoryOffset GetInsnOffset(TargetAddr addr) override;
};

#endif // DISASSEMBLER_H
