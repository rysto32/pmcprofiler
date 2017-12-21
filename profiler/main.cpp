// Copyright (c) 2009-2014 Sandvine Incorporated.  All rights reserved.
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

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include "Image.h"
#include "Profiler.h"
#include "ProfilePrinter.h"
#include "CallchainProfilePrinter.h"
#include "SharedString.h"

#include <err.h>
#include <sys/param.h>
#include <pmclog.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory>

void usage(void);

std::string samplefile("/tmp/samples.out");

std::unordered_set<pid_t> pid_filter;

// this global (sorry) indicates that the process should exit upon certain
// errors.  Initially it is being used to exit if a .so file cannot be
// mapped
bool g_quitOnError = false;

bool g_includeTemplates = false;

FILE * openOutFile(const char * path)
{
	FILE * file;
	if (strcmp(path, "-") == 0)
		file = stdout;
	else {
		file = fopen(path, "w");

		if (!file) {
			fprintf(stderr, "Could not open %s for writing\n", path);
			usage();
		}
	}

	return file;
}

int
main(int argc, char *argv[])
{
	int ch;
	bool showlines = false;
	bool printBoring = true;
	uint32_t maxDepth = PMC_CALLCHAIN_DEPTH_MAX;
	int threshold = 0;
	g_quitOnError = false;
	FILE * file;
	char * temp;
	std::vector<std::unique_ptr<ProfilePrinter> > printers;
	const char *modulePath = NULL;
	pid_t pid;

	if (elf_version(EV_CURRENT) == EV_NONE)
		err(1, "libelf incompatible");

	/* Workaround for libdwarf crash when processing some KLD modules. */
	dwarf_set_reloc_application(0);

	while ((ch = getopt(argc, argv, "qlG:bf:F:d:o:p:t:r:m:T")) != -1) {
		switch (ch) {
			case 'f':
				samplefile = optarg;
				break;
			case 'F':
				file = openOutFile(optarg);
				printers.push_back(std::make_unique<FlameGraphProfilerPrinter>(file, PMC_CALLCHAIN_DEPTH_MAX, threshold, true));
				break;
			case 'l':
				showlines = true;
				break;
			case 'q':
				g_quitOnError = true;
				break;
			case 'G':
				file = openOutFile(optarg);
				printers.push_back(std::make_unique<LeafProfilePrinter>(file, maxDepth, threshold, printBoring));
				break;
			case 'r':
				file = openOutFile(optarg);
				printers.push_back(std::make_unique<RootProfilePrinter>(file, PMC_CALLCHAIN_DEPTH_MAX, threshold, true));
				break;
			case 'o':
				file = openOutFile(optarg);
				printers.push_back(std::make_unique<FlatProfilePrinter>(file));
				break;
			case 'p':
				pid = strtoul(optarg, &temp, 0);

				if (*temp != '\0' || pid < 1)
					usage();
				pid_filter.insert(pid);
				break;

			case 'b':
				printBoring = false;
				break;
			case 't':
				threshold = strtol(optarg, &temp, 0);

				if (*temp != '\0' || threshold < 0 || threshold > 100)
					usage();

				break;
			case 'd':
				char * temp;
				maxDepth = strtoul(optarg, &temp, 0);

				if (*temp != '\0')
					usage();

				break;
			case 'm':
				modulePath = optarg;
				break;
			case 'T':
				g_includeTemplates = true;
				break;
			case '?':
			default:
				usage();
				break;
		}
	}
	argc -= optind;
	argv += optind;

	if (printers.empty())
		printers.push_back(std::make_unique<FlatProfilePrinter>(stdout));

	Profiler profiler(samplefile, showlines, modulePath);
	for (const auto & printer : printers)
		profiler.createProfile(*printer);

	Image::freeImages();
	return 0;
}

void
usage()
{
	fprintf(stderr,
		"usage: pmcprofiler [-lqb] [-f samplefile] [-o flat_output] [-G leaf_output]\n"
		"[-r root_output] [-d <max depth>] [-t theshold] \n"
		"    l - show line numbers\n"
		"    q - quit on error\n"
		"    b - exclude \"boring\" call frames in subsequent leaf-up profiles\n"
		"    o - file to print flat profile information to(- for stdout)\n"
		"    F - file to print FlameGraph output to(- for stdout)\n"
		"    G - file to print leaf-up callchain profile to(- for stdout)\n"
		"    r - file to print root-down callchain profile to(- for stdout)\n"
		"    d - maximum depth to go to in subsequent leaf-up callchain profiles\n"
		"    t - print only entries greater than threshold in subsequent profiles\n"
		"    default samplefile is /tmp/samples.out\n"
		"    default output is flat profile to standard out\n");
	exit(1);
}

