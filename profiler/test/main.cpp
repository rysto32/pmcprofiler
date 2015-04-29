
#include "DwarfLookup.h"

#include <err.h>
#include <iostream>
#include <stdlib.h>

int
main(int argc, char **argv)
{
	uintptr_t addr;
	char *endp;
	std::string file;
	std::string func;
	u_int line;
	bool success;

	if (argc != 3)
		errx(1, "Usage: %s <filename> <addr>\n", argv[0]);

	addr = strtoul(argv[2], &endp, 0);
	if (argv[2][0] == '\0' || *endp != '\0')
		errx(1, "%s is not a number\n", argv[2]);
	
	if (elf_version(EV_CURRENT) == EV_NONE)
		err(1, "libelf incompatible");
	
	DwarfLookup lookup(argv[1]);

	size_t i, depth;
	depth = lookup.GetInlineDepth(addr);
	std::cout << "Depth=" << depth << std::endl;
	for (i = 0; i < depth; i++) {
		file = "";
		func = "";
		line = -1;
		success = lookup.LookupLine(addr, i, file, func, line);
		std::cout << func << std::endl;
		std::cout << file << ":" << line << std::endl;
	}

	file = "";
	func = "";
	line = -1;
	success = lookup.LookupFunc(addr, file, func, line);
	std::cout << "Func: " << func << " @ " << file << ":" << line << std::endl;

	return (0);
}
