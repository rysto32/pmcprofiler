
#include <err.h>
#include <fcntl.h>
#include <gelf.h>

#include <iostream>
#include <map>

#include "llvm/ADT/Triple.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"

#include "MapUtil.h"

using namespace llvm;

typedef std::map<GElf_Addr, std::shared_ptr<GElf_Sym>> SymbolMap;

static Elf_Scn *
FindTextSection(Elf *elf)
{
	Elf_Scn *section;
	const char *name;
	GElf_Shdr shdr;
	size_t shdrstrndx;

	if (elf_getshdrstrndx(elf, &shdrstrndx) != 0)
		return (NULL);

	section = NULL;
	while ((section = elf_nextscn(elf, section)) != NULL) {
		if (gelf_getshdr(section, &shdr) == NULL)
			continue;

		gelf_getshdr(section, &shdr);
		name = elf_strptr(elf, shdrstrndx, shdr.sh_name);
		if (name != NULL) {
			if (strcmp(name, ".text") == 0)
				return (section);
		}
	}

	errx(1, "Could not find .text section");
}

static void
FillSymtab(Elf *elf, Elf_Scn *section, SymbolMap &symbols)
{
	GElf_Shdr header;
	GElf_Sym symbol;
	Elf_Data *data;
	size_t i, count;

	if (gelf_getshdr(section, &header) == NULL)
		return;

	count = header.sh_size / header.sh_entsize;
	data = elf_getdata(section, NULL);

	for (i = 0; i < count; i++) {
		gelf_getsym(data, i, &symbol);
		if (GELF_ST_TYPE(symbol.st_info) == STT_FUNC &&
		    symbol.st_shndx != SHN_UNDEF) {
			symbols.insert(std::make_pair(symbol.st_value,
			    std::shared_ptr<GElf_Sym>(new GElf_Sym(symbol))));
		}
	}
}

static void
InitSymtab(Elf *elf, SymbolMap &symbols)
{
	Elf_Scn *section;
	GElf_Shdr shdr;

	section = NULL;
	while ((section = elf_nextscn(elf, section)) != NULL) {
		if (gelf_getshdr(section, &shdr) == NULL)
			continue;

		if (shdr.sh_type == SHT_SYMTAB)
			FillSymtab(elf, section, symbols);
	}
}

static int
FindContainingSymbol(Elf *elf, const SymbolMap &symbols, GElf_Sym &symbol, u_long offset)
{

	SymbolMap::const_iterator it = LastSmallerThan(symbols, offset);
	if (it == symbols.end())
		return (ENOENT);

	fprintf(stderr, "Nearest symbol is %lx\n", it->first);
	symbol = *it->second;
	return (0);
}

static Elf_Data *
GetElfData(Elf *elf, Elf_Scn *text, const GElf_Sym & symbol)
{
	GElf_Shdr header;
	Elf_Data *data;

	if (gelf_getshdr(text, &header) == NULL)
		return (NULL);

	data = NULL;
	while ((data = elf_rawdata(text, data)) != NULL) {
		fprintf(stderr, "sh_addr: %lx off: %lx size: %lx\n", header.sh_addr, data->d_off, data->d_size);
		GElf_Addr addr = header.sh_addr + data->d_off;
		if (addr <= symbol.st_value &&
		   ((addr + data->d_size) > symbol.st_value))
			return (data);
	}

	return (NULL);
}

static void
FindRegister(Elf_Scn *text, Elf_Data *data, GElf_Sym &symbol, u_long offset)
{
	std::string TripleStr(sys::getDefaultTargetTriple());
	Triple TheTriple(TripleStr);
	std::string Error;
	const Target *T = TargetRegistry::lookupTarget("", TheTriple,
                                                         Error);
	if (T == NULL)
		err(1, "error: could not find target %s\n", TripleStr.c_str());

	std::unique_ptr<MCSubtargetInfo> STI(
	    T->createMCSubtargetInfo(TripleStr, "", ""));
	if (!STI)
		err(1, "error: could not create subtarget for %s\n", TripleStr.c_str());

	std::unique_ptr<const MCRegisterInfo> MRI(T->createMCRegInfo(TripleStr));
	if (!MRI)
		err(1, "error: no register info for target %s", TripleStr.c_str());

	std::unique_ptr<const MCAsmInfo> MAI(T->createMCAsmInfo(*MRI, TripleStr));
	if (!MAI)
		err(1, "error: no assembly info for target %s", TripleStr.c_str());

	// Set up the MCContext for creating symbols and MCExpr's.
	MCContext Ctx(MAI.get(), MRI.get(), nullptr);

	std::unique_ptr<const MCDisassembler> DisAsm(T->createMCDisassembler(*STI, Ctx));
	if (!DisAsm)
		err(1, "error: no disassembler for target %s", TripleStr.c_str());

	GElf_Shdr header;
	if (gelf_getshdr(text, &header) == NULL)
		err(1, "Could not get shdr");

	uint64_t i, size;
	MCInst inst;
	ArrayRef<uint8_t> buf(static_cast<const uint8_t *>(data->d_buf), data->d_size);
	for (i = symbol.st_value; i <= offset; i += size) {
		uint64_t index = i - header.sh_addr - data->d_off;
		MCDisassembler::DecodeStatus S;
		S = DisAsm->getInstruction(inst, size, buf.slice(index), index,
                              /*REMOVE*/ nulls(), nulls());

		if (S == MCDisassembler::Fail || S == MCDisassembler::SoftFail)
			errx(1, "Failed to disassemble at index %lx\n", i);

		assert(S == MCDisassembler::Success);
	}

	printf("Num Operands: %d i: %lx\n", inst.getNumOperands(), i);
	/*for (u_int j = 0; j < inst.getNumOperands(); j++) {
		const MCOperand &op = inst.getOperand(j);
		if (op.isReg())
			printf("\t%d: Reg op\n", j);
		if (op.isImm())
			printf("\t%d: Imm op\n", j);
		if (op.isFPImm())
			printf("\t%d: Reg op\n", j);
		if (op.isExpr())
			printf("\t%d: Expr op\n", j);
		if (op.isInst())
			printf("\t%d: Inst op\n", j);
		op.dump();
	}*/
	inst.dump();
}

int main(int argc, char **argv)
{
	Elf *elf;
	int error;

	if (argc < 2)
		errx(1, "usage: %s <executable> offsets...\n", argv[0]);

	if (elf_version(EV_CURRENT) == EV_NONE)
		err(1, "libelf incompatible");

	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllDisassemblers();

	int fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		err(1, "Could not open %s for reading\n", argv[1]);
	}

	elf = elf_begin(fd, ELF_C_READ, NULL);
	if (elf == NULL) {
		errx(1, "elf_begin failed: filename=%s elf_errno=%s",
		      argv[1], elf_errmsg(elf_errno()));
	}

	Elf_Scn *text = FindTextSection(elf);

	SymbolMap symbols;
	InitSymtab(elf, symbols);

	std::string triple(sys::getDefaultTargetTriple());

	int i;
	for (i = 2; i < argc; i++) {
		char *endp;
		u_long offset;
		GElf_Sym symbol;

		offset = strtoul(argv[i], &endp, 0);
		if (argv[i][0] == '\0' || *endp != '\0')
			errx(1, "%s is not a number\n", argv[i]);

		error = FindContainingSymbol(elf, symbols, symbol, offset);
		if (error != 0)
			errx(1, "Could not find symbol containing %lx\n",
			     offset);

		Elf_Data *data = GetElfData(elf, text, symbol);
		if (data == NULL)
			errx(1, "Could not find text data containing %lx\n",
			     offset);

		FindRegister(text, data, symbol, offset);
	}
}