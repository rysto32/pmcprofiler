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

#include "DisassemblerImpl.h"

#include "DwarfException.h"

#include <err.h>

#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

Disassembler::~Disassembler()
{
}

std::unique_ptr<Disassembler> 
Disassembler::Create(const GElf_Shdr & textHdr, Elf_Data *data)
{

	return std::make_unique<DisassemblerImpl>(textHdr, data);
}

DisassemblerImpl::DisassemblerImpl(const GElf_Shdr &textHdr, Elf_Data *data)
  : llvmTripleStr(llvm::sys::getDefaultTargetTriple()), // XXX get triple for target
    textBuf(static_cast<const uint8_t *>(data->d_buf), data->d_size),
    textOffset(textHdr.sh_addr + data->d_off)
{
	std::string llvmError;

//	fprintf(stderr, "Initialize disasm: text start=%lx data off=%lx textOffset=%lx\n",
//	    textHdr.sh_addr, data->d_off, textOffset);

	llvm::Triple TheTriple(llvmTripleStr);
	llvm::MCTargetOptions options;

	target = llvm::TargetRegistry::lookupTarget("", TheTriple, llvmError);
	if (target == NULL)
		throw DwarfException("error: could not find target");

	sti = std::unique_ptr<llvm::MCSubtargetInfo>(target->createMCSubtargetInfo(llvmTripleStr, "", ""));
	if (!sti)
		throw DwarfException("error: could not create subtarget");

	mri = std::unique_ptr<const llvm::MCRegisterInfo>(target->createMCRegInfo(llvmTripleStr));
	if (!mri)
		throw DwarfException("error: no register info");

	mai = std::unique_ptr<const llvm::MCAsmInfo>(target->createMCAsmInfo(*mri, llvmTripleStr,
	    options));
	if (!mai)
		throw DwarfException("error: no assembly info");

	// Set up the MCContext for creating symbols and MCExpr's.
	ctx = std::make_unique<llvm::MCContext>(mai.get(), mri.get(), nullptr);

	mcii = std::unique_ptr<llvm::MCInstrInfo>(target->createMCInstrInfo());
	if (!mcii)
		throw DwarfException("error: no instr info");

	mcip = std::unique_ptr<llvm::MCInstPrinter>(target->createMCInstPrinter(TheTriple, mai->getAssemblerDialect(),
	    *mai, *mcii, *mri));
	if (!mcip)
		throw DwarfException("error: no instr printer");

	disasm = std::unique_ptr<const llvm::MCDisassembler>(target->createMCDisassembler(*sti, *ctx));
	if (!disasm)
		throw DwarfException("error: no disassembler");
}

void
DisassemblerImpl::InitFunc(TargetAddr funcStart)
{
	nextInstOffset = funcStart - textOffset;
}

MemoryOffset
DisassemblerImpl::GetInsnOffset(TargetAddr addr)
{
	TargetAddr targetOff = addr - textOffset;

	while (targetOff >= nextInstOffset)
		DisassembleNext();

	return DecodeInst();
}

void
DisassemblerImpl::DisassembleNext()
{
	llvm::MCDisassembler::DecodeStatus status;

	uint64_t instSize;
	status = disasm->getInstruction(inst, instSize, textBuf.slice(nextInstOffset),
	    nextInstOffset, llvm::nulls());
	if (status == llvm::MCDisassembler::Fail || status == llvm::MCDisassembler::SoftFail)
		throw DwarfException("Disasm failed");

	nextInstOffset += instSize;
}

MemoryOffset
DisassemblerImpl::DecodeInst()
{
	if (inst.getOpcode() == 2227) {
		// pop
		fprintf(stderr, "Unhandled pop\n");
		return MemoryOffset();
	} else if (inst.getOpcode() == 2349) {
		// push
		fprintf(stderr, "Unhandled push\n");
		return MemoryOffset();
	}
	// All instructions with memory operands use 6 MCOperands
	else if (inst.getNumOperands() == 6) {
		const llvm::MCOperand & op0 = inst.getOperand(0);
		const llvm::MCOperand & op1 = inst.getOperand(1);
		const llvm::MCOperand & op5 = inst.getOperand(5);

		if (op0.isReg() && op1.isReg()) {
			// mov (op1,%reg,0), op0
			const llvm::MCOperand & op4 = inst.getOperand(4);
			int dwarfReg = LlvmRegToDwarf(op1.getReg());
			return MemoryOffset(dwarfReg, op4.getImm(), MemoryOffset::STORE);
		} else if (op0.isReg() && (op5.isReg() || op5.isImm())) {
			// mov op5, (op0, %reg, 0)
			const llvm::MCOperand & op3 = inst.getOperand(3);
			int dwarfReg = LlvmRegToDwarf(op0.getReg());

			return MemoryOffset(dwarfReg, op3.getImm(), MemoryOffset::LOAD);
		}
	}

	std::string str;
	llvm::raw_string_ostream os(str);

	inst.dump_pretty(os, mcip.get());

	fprintf(stderr, "Unhandled opcode %d (%s)\n", inst.getOpcode(), os.str().c_str());
	return MemoryOffset();
}

int
DisassemblerImpl::LlvmRegToDwarf(int llvmReg)
{
	int dwarfRegNum = mri->getDwarfRegNum(llvmReg, false);

	if (dwarfRegNum < 32)
		return llvm::dwarf::DW_OP_reg0 + dwarfRegNum;
	else
		errx(1, "Can't handle DW_OP_regx");
}
