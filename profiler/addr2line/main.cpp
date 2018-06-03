
#include "Callframe.h"
#include "DefaultImageFactory.h"
#include "Image.h"
#include "SharedString.h"

#include <err.h>
#include <libelf.h>
#include <iostream>
#include <stdlib.h>

bool g_includeTemplates = false;

int
main(int argc, char **argv)
{
	TargetAddr addr;
	char *endp;
	SharedString file;
	SharedString func;

	if (argc < 3)
		errx(1, "Usage: %s <filename> <addr>....\n", argv[0]);
	
	if (elf_version(EV_CURRENT) == EV_NONE)
		err(1, "libelf incompatible");

	DefaultImageFactory factory;
	Image *image = factory.GetImage(argv[1]);
	if (image->GetImageFile()->empty())
		errx(1, "Could not open %s", argv[1]);

	std::vector<const Callframe *> frameList;
	for (int i = 2; i < argc; ++i) {
		addr = strtoul(argv[i], &endp, 0);
		if (argv[i][0] == '\0' || *endp != '\0')
			errx(1, "%s is not a number\n", argv[i]);

		frameList.push_back(&image->GetFrame(addr));
	}

	factory.MapAll();

	for (auto frame : frameList) {
		const auto & inlines = frame->getInlineFrames();
		std::cout << "Offset=" << std::hex << frame->getOffset() << " Depth=" << std::dec << inlines.size() << std::endl;
		for (const auto & inFrame : inlines) {
			std::cout << *inFrame.getDemangled() << "(die=" << std::hex << inFrame.getDieOffset() << std::dec << ")\n";
			std::cout << *inFrame.getFile() << ":" << inFrame.getCodeLine() << "\n";
		}

		const auto & leafFrame = inlines.back();
		std::cout << "Func: " << *leafFrame.getDemangled()
		    << " @ " << *leafFrame.getFile() << ":" <<
		    leafFrame.getFuncLine() << "\n\n";
	}

	return (0);
}
