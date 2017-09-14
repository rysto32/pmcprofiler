
#include <err.h>
#include <fcntl.h>
#include <gelf.h>

#include "llvm/Support/Host.h"

using namespace llvm;

static Elf_Scn *
FindTextSection(Elf *elf)
{
	Elf_Scn *section;
	const char *name;
	GElf_Shdr shdr;
	size_t shdrstrndx;

	if (elf_getshdrstrndx(elf, &shdrstrndx) != 0)
		return;

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

static Elf_Scn *
FindSymtab(Elf *elf)
{
	Elf_Scn *section;
	GElf_Shdr shdr;

	section = NULL;
	while ((section = elf_nextscn(elf, section)) != NULL) {
		if (gelf_getshdr(section, &shdr) == NULL)
			continue;

		if (shdr.sh_type == SHT_SYMTAB)
			return (section);
	}

	errx(1, "Could not find symtab");
}

static int
FindContainingSymbol(Elf *elf, Elf_Scn *section, GElf_Sym &symbol, u_long offset)
{
	GElf_Shdr header;
	GElf_Sym next_symbol;
	Elf_Data *data;
	size_t i, count;

	if (gelf_getshdr(section, &shdr) == NULL)
		return (ENOENT);

	count = header.sh_size / header.sh_entsize;
	data = elf_getdata(section, NULL);

	/* Find the first function symbol */
	for (i = 0; i < count; i++) {
		gelf_getsym(data, i, &symbol);
		if (GELF_ST_TYPE(symbol.st_info) == STT_FUNC &&
		    symbol.st_shndx != SHN_UNDEF)
			break;
	}

	if (i == count)
		return (ENOENT);

	/* Now find the two symbols that "bracket" the offset.  The lower symbol
	 * is the one that contains the offset.
	 */
	for(; i < count; i++) {
		gelf_getsym(data, i, &next_symbol);
		if (GELF_ST_TYPE(symbol.st_info) == STT_FUNC &&
		    symbol.st_shndx != SHN_UNDEF) {
			if (next_symbol.st_value > offset)
				return (0);
		}
		memcpy(&symbol, &next_symbol, sizeof(next_symbol));
	}

	/*
	 * If we got here, the PC is contained in the final symbol, so we have
	 * succeeeded.
	 */
	return (0);
}

static Elf_Data *
GetElfData(Elf *elf, Elf_Scn *text, const GElf_Sym & symbol)
{
	Elf_Data *data;

	data = NULL;
	while ((data = elf_rawdata(text, data)) != NULL) {
		if (data->d_off <= symbol.st_value &&
		   ((data->d_off + data->d_size) > symbol.st_value))
			return (data);
	}

	return (NULL);
}

static void
FindRegister(Elf_Data *data, GElf_Sym &symbol, u_long offset)
{
	std::unique_ptr<const MCRegisterInfo> MRI(T.createMCRegInfo(Triple));
	if (!MRI)
		err(1, "error: no register info for target %s", Triple.c_str());

	std::unique_ptr<const MCAsmInfo> MAI(T.createMCAsmInfo(*MRI, Triple));
	if (!MAI)
		err(1, "error: no assembly info for target %s", Triple.c_str());

	// Set up the MCContext for creating symbols and MCExpr's.
	MCContext Ctx(MAI.get(), MRI.get(), nullptr);

	std::unique_ptr<const MCDisassembler> DisAsm(T.createMCDisassembler(STI, Ctx));
	if (!DisAsm)
		err(1, "error: no disassembler for target %s", Triple.c_str());

	uint64_t i, size;
	MCInst inst;
	for (i = symbol.st_value; i <= offset; i += size) {
		MCDisassembler::DecodeStatus S;
		const uint8_t *buf = static_cast<const uint8_t *>(data->d_buf) + (i - data->d_off);
		S = DisAsm.getInstruction(Inst, size, buf,
                              /*REMOVE*/ nulls(), nulls());
	}
}

int main(int argc, char **argv)
{
	Elf *elf;
	int error;

	if (argc < 2)
		errx(1, "usage: %s <executable> offsets...\n", argv[0]);

	int fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		err(1, "Could not open %s for reading\n", argv[1]);
	}

	elf = elf_begin(fd, ELF_C_READ, NULL);
	if (elf == NULL) {
		errx(1, "elf_begin failed: filename=%s elf_errno=%s",
		      filename.c_str(), elf_errmsg(elf_errno()));
	}

	Elf_Scn *text = FindTextSection(elf);
	Elf_Scn *symtab = FindSymtab(elf);

	std::string triple(sys::getDefaultTargetTriple());

	int i;
	for (i = 2; i < argc; i++) {
		char *endp;
		u_long offset;
		GElf_Sym symbol;

		offset = strtoul(argv[i], &endp, 0);
		if (argv[i][0] == '\0' || *endp != '\0')
			errx(1, "%s is not a number\n", argv[i]);

		error = FindContainingSymbol(elf, symtab, symbol, offset);
		if (error != 0)
			errx(1, "Could not find symbol containing %lx\n",
			     offset);

		Elf_Data *data = GetElfData(elf, text, symbol);
		if (data == NULL)
			errx(1, "Could not find text data containing %lx\n",
			     offset);
	}
}