
#include <err.h>
#include <fcntl.h>
#include <gelf.h>
#include <libdwarf.h>

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
using namespace llvm::dwarf;

typedef std::map<GElf_Addr, std::shared_ptr<GElf_Sym>> SymbolMap;

class MemoryOffset
{
public:
	enum AccessType { UNDEFINED, LOAD, STORE };

private:
	int reg;
	int32_t offset;
	AccessType type;

public:
	MemoryOffset()
	  : reg(0), offset(0), type(UNDEFINED)
	{
	}

	MemoryOffset(int reg, int32_t offset, AccessType type)
	  : reg(reg), offset(offset), type(type)
	{
	}

	int GetReg() const
	{
		return (reg);
	}

	int GetOffset() const
	{
		return (offset);
	}

	AccessType GetType() const
	{
		return (type);
	}

	bool IsDefined() const
	{
		return (type != UNDEFINED);
	}

	const char * GetTypeString() const
	{
		return (type == LOAD ? "load" : "store");
	}
};

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

	//fprintf(stderr, "Nearest symbol is %lx\n", it->first);
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
		//fprintf(stderr, "sh_addr: %lx off: %lx size: %lx\n", header.sh_addr, data->d_off, data->d_size);
		GElf_Addr addr = header.sh_addr + data->d_off;
		if (addr <= symbol.st_value &&
		   ((addr + data->d_size) > symbol.st_value))
			return (data);
	}

	return (NULL);
}

static void
FindBaseReg(const MCInst &inst, const MCRegisterInfo &MRI, MemoryOffset &off)
{

	assert (!off.IsDefined());

	if (inst.getOpcode() == 2227) {
		// pop
		return;

	} else if (inst.getOpcode() == 2349) {
		// push
		return;

	}
	// All instructions with memory operands use 6 MCOperands
	else if (inst.getNumOperands() == 6) {
		const MCOperand & op0 = inst.getOperand(0);
		const MCOperand & op1 = inst.getOperand(1);
		const MCOperand & op5 = inst.getOperand(5);

		if (op0.isReg() && op1.isReg()) {
			// mov (op1,%reg,0), op0
			const MCOperand & op4 = inst.getOperand(4);
			int dwarfReg = MRI.getDwarfRegNum(op1.getReg(), false);

			off = MemoryOffset(dwarfReg, op4.getImm(), MemoryOffset::STORE);
			return;
		} else if (op0.isReg() && (op5.isReg() || op5.isImm())) {
			// mov op5, (op0, %reg, 0)
			const MCOperand & op3 = inst.getOperand(3);
			int dwarfReg = MRI.getDwarfRegNum(op0.getReg(), false);

			off = MemoryOffset(dwarfReg, op3.getImm(), MemoryOffset::LOAD);
			return;
		}
	}
}

static void
FindRegister(Elf_Scn *text, Elf_Data *data, GElf_Sym &symbol, u_long offset, MemoryOffset &memOff)
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
		    nulls(), nulls());

		if (S == MCDisassembler::Fail || S == MCDisassembler::SoftFail)
			errx(1, "Failed to disassemble at index %lx\n", i);

		assert(S == MCDisassembler::Success);
	}

	FindBaseReg(inst, *MRI, memOff);
	if (!memOff.IsDefined()) {
		fprintf(stderr, "0x%lx does not access memory\n", offset);
		inst.dump();
		return;
	}
}

class DwarfDieCollection
{
private:
	Dwarf_Debug dwarf;
	Dwarf_Die parent;

public:
	DwarfDieCollection(Dwarf_Debug dwarf, Dwarf_Die die = NULL)
	  : dwarf(dwarf), parent(die)
	{
	}

	DwarfDieCollection() = delete;

	~DwarfDieCollection();

	class const_iterator
	{
	private:
		Dwarf_Debug dwarf;
		Dwarf_Die die;

		friend class DwarfDieCollection;

		const_iterator(Dwarf_Debug dwarf, Dwarf_Die die)
		  : dwarf(dwarf), die(die)
		{
		}

	public:
		const_iterator()
		  : dwarf(NULL), die(NULL)
		{
		}

		~const_iterator()
		{
			dwarf_dealloc(dwarf, die, DW_DLA_DIE);
		}

		const Dwarf_Die & operator*() const
		{
			return (die);
		}

		const_iterator &operator++()
		{
			Dwarf_Die last_die = die;
			Dwarf_Error derr;
			int error;

			error = dwarf_siblingof(dwarf, last_die, &die, &derr);
			if (last_die != NULL)
				dwarf_dealloc(dwarf, last_die, DW_DLA_DIE);

			if (error != DW_DLV_OK)
				die = NULL;
			return (*this);
		}

		bool operator==(const const_iterator &rhs) const
		{
			if (dwarf != rhs.dwarf)
				return (false);
			return (die == rhs.die);
		}

		bool operator!=(const const_iterator &rhs) const
		{
			return !(*this == rhs);
		}

	};

	const_iterator begin() const
	{
		Dwarf_Die child;
		int error;
		Dwarf_Error derr;

		error = dwarf_child(parent, &child, &derr);
		if (error != DW_DLV_OK)
			child = NULL;

		return (const_iterator(dwarf, child));
	}

	const_iterator end() const
	{
		return (const_iterator(dwarf, NULL));
	}
};

static bool
CheckLocalVar(Dwarf_Debug dwarf, Dwarf_Die die, uint64_t offset,
    const MemoryOffset &memOff)
{
	Dwarf_Attribute attr;
	Dwarf_Locdesc **llbuf;
	Dwarf_Signed lcnt, i;
	Dwarf_Loc *loc;
	Dwarf_Error derr;
	Dwarf_Half j;
	int error;

	error = dwarf_attr(die, DW_AT_location, &attr, &derr);
	if (error != 0)
		return (false);

	error = dwarf_loclist_n(attr, &llbuf, &lcnt, &derr);
	if (error != NULL)
		return (false);

	for (i = 0; i < lcnt; i++) {
		for (j = 0; j < llbuf[i]->ld_cents; j++) {
			lr = &llbuf[i]->ld_s[j];
		}
		dwarf_dealloc(dwarf, llbuf[i]->ld_s, DW_DLA_LOC_BLOCK);
		dwarf_dealloc(dbg, llbuf[i], DW_DLA_LOCDESC);
	}
	dwarf_dealloc(dbg, llbuf, DW_DLA_LIST);
}

static void
FindCompileUnitVar(Dwarf_Debug dwarf, Dwarf_Die parent, uint64_t offset,
    const MemoryOffset &memOff)
{
	DwarfDieCollection dieList(dwarf, parent);
	Dwarf_Half tag;
	Dwarf_Error derr;
	int error;

	for (auto && die : dieList) {
		error = dwarf_tag(die, &tag, &derr);
		if (error == DW_DLV_OK) {
			if (tag == DW_TAG_formal_parameter || tag == DW_TAG_variable) {
				if (CheckLocalVar(dwarf, die, offset, memOff))
					return;
			}
		}

		FindCompileUnitVar(dwarf, die, offset, memOff);
	}
}

class DwarfRanges
{
private:
	Dwarf_Debug dwarf;
	Dwarf_Ranges *ranges;
	Dwarf_Signed count;

public:
	DwarfRanges(Dwarf_Debug dwarf, Dwarf_Unsigned off)
	  : dwarf(dwarf), ranges(NULL)
	{
		Dwarf_Error derr;
		Dwarf_Unsigned size;
		int error;

		error = dwarf_get_ranges(dwarf, off, &ranges, &count, &size, &derr);
		if (error != DW_DLV_OK)
			return;

	}

	DwarfRanges() = delete;

	~DwarfRanges()
	{
		if (ranges != NULL)
			dwarf_ranges_dealloc(dwarf, ranges, count);
	}

	bool IsValid() const
	{
		return (ranges != NULL);
	}

	class const_iterator
	{
	private:
		const DwarfRanges *parent;
		Dwarf_Signed index;

		friend class DwarfRanges;

		const_iterator(const DwarfRanges *p, int i)
		  : parent(p), index(i)
		{
		}

	public:
		const_iterator()
		  : parent(NULL)
		{
		}

		const Dwarf_Ranges & operator*() const
		{
			return (parent->ranges[index]);
		}

		const Dwarf_Ranges & operator->() const
		{
			return (parent->ranges[index]);
		}

		const_iterator &operator++()
		{
			index++;
			return (*this);
		}

		bool operator==(const const_iterator &rhs) const
		{
			if (parent != rhs.parent)
				return (false);
			return (index == rhs.index);
		}

		bool operator!=(const const_iterator &rhs) const
		{
			return !(*this == rhs);
		}
	};

	const_iterator begin() const
	{
		return const_iterator(this, 0);
	}

	const_iterator end() const
	{
		return const_iterator(this, count);
	}
};

static bool
SearchCompileUnitRanges(Dwarf_Debug dwarf, Dwarf_Die die, Dwarf_Unsigned range_off,
    uint64_t offset, const MemoryOffset &memOff)
{
	int error;
	Dwarf_Error derr;
	Dwarf_Unsigned base_addr, low_pc, high_pc;

	DwarfRanges rangeList(dwarf, range_off);
	if (!rangeList.IsValid())
		return (false);

	error = dwarf_attrval_unsigned(die, DW_AT_low_pc, &base_addr, &derr);
	if (error != 0)
		base_addr = 0;

	for (auto && range : rangeList) {
		switch (range.dwr_type) {
		case DW_RANGES_ENTRY:
			low_pc = base_addr + range.dwr_addr1;
			high_pc = base_addr + range.dwr_addr2;
			if (offset >= low_pc && offset < high_pc) {
				FindCompileUnitVar(dwarf, die, offset, memOff);
				return (true);
			}
			break;
		case DW_RANGES_ADDRESS_SELECTION:
			base_addr = range.dwr_addr2;
			break;
		case DW_RANGES_END:
			goto break_loop;
		}
	}

break_loop:
	return (false);
}

class DwarfSrcLines
{
public:
	Dwarf_Line *lbuf;
};

static bool
SearchCompileUnitSrcLines(Dwarf_Debug dwarf, Dwarf_Die die, uint64_t offset, const MemoryOffset &memOff)
{

	err(1, "src_lines information needed\n");
}

static bool
SearchCompileUnit(Dwarf_Debug dwarf, Dwarf_Die die, uint64_t offset, const MemoryOffset &memOff)
{
	Dwarf_Error derr;
	Dwarf_Unsigned range_off, low_pc, high_pc;
	int error, err_lo, err_hi;

	error = dwarf_attrval_unsigned(die, DW_AT_ranges, &range_off, &derr);
	if (error == DW_DLV_OK) {
		return (SearchCompileUnitRanges(dwarf, die, range_off, offset, memOff));
	}

	err_lo = dwarf_attrval_unsigned(die, DW_AT_low_pc, &low_pc, &derr);
	err_hi = dwarf_attrval_unsigned(die, DW_AT_high_pc, &high_pc, &derr);
	if (err_lo == DW_DLV_OK && err_hi == DW_DLV_OK) {
		if (offset >= low_pc && offset < high_pc) {
			FindCompileUnitVar(dwarf, die, offset, memOff);
			return (true);
		}
		return (false);
	}

	return (SearchCompileUnitSrcLines(dwarf, die, offset, memOff));
}

static void
ProcessCompileUnit(Dwarf_Debug dwarf, Dwarf_Die die, uint64_t offset, const MemoryOffset &memOff)
{
	Dwarf_Die last_die;
	Dwarf_Error derr;
	Dwarf_Half tag;
	int error;
	bool found = false;

	do {
		if (dwarf_tag(die, &tag, &derr) == DW_DLV_OK) {
			if (tag == DW_TAG_compile_unit)
				found = SearchCompileUnit(dwarf, die, offset, memOff);
		}

		last_die = die;
		error = dwarf_siblingof(dwarf, last_die, &die, &derr);

		dwarf_dealloc(dwarf, last_die, DW_DLA_DIE);
	} while (error == DW_DLV_OK && !found);
}

static void
FindVar(Elf *elf, uint64_t offset, const MemoryOffset &memOff)
{
	Dwarf_Debug dwarf;
	Dwarf_Die die;
	Dwarf_Error derr;
	Dwarf_Unsigned next_offset;
	int error;

	error = dwarf_elf_init(elf, DW_DLC_READ, NULL, NULL, &dwarf, &derr);
	if (error != 0)
		errx(1, "Could not read dwarf debug data\n");

	while ((error = dwarf_next_cu_header(dwarf, NULL, NULL, NULL, NULL,
	    &next_offset, &derr)) == DW_DLV_OK) {
		if (dwarf_siblingof(dwarf, NULL, &die, &derr) == DW_DLV_OK)
			ProcessCompileUnit(dwarf, die, offset, memOff);
	}
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

		MemoryOffset memOff;
		FindRegister(text, data, symbol, offset, memOff);
		if (memOff.IsDefined())
			FindVar(elf, offset, memOff);
	}
}