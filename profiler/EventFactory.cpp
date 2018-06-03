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

#include "EventFactory.h"
#include "Profiler.h"
#include "ProcessState.h"
#include "Sample.h"

#include <err.h>
#include <stdio.h>
#include <sys/queue.h>
#include <pmc.h>
#include <pmclog.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

extern void usage(void);

void
EventFactory::createEvents(Profiler& profiler)
{
	pmclog_ev pmcEvent;
	void *logCookie;
	int fd;
	
	fd = open(profiler.getDataFile().c_str(), O_RDONLY);
	if (fd < 0) {
		warn("Could not open data file %s\n", 
		     profiler.getDataFile().c_str());
		usage();
		return;
	}

	logCookie = pmclog_open(fd);
	if (logCookie == NULL) {
		warn("Could not open log file!\n");
		close(fd);
		return;
	}

	while (pmclog_read(logCookie, &pmcEvent) == 0) {
		assert(pmcEvent.pl_state == PMCLOG_OK);

		switch (pmcEvent.pl_type) {
			case PMCLOG_TYPE_CLOSELOG:
			case PMCLOG_TYPE_DROPNOTIFY:
			case PMCLOG_TYPE_INITIALIZE:
			case PMCLOG_TYPE_MAPPINGCHANGE:
			case PMCLOG_TYPE_PMCALLOCATE:
			case PMCLOG_TYPE_PMCATTACH:
			case PMCLOG_TYPE_PMCDETACH:
			case PMCLOG_TYPE_PROCCSW:
			case PMCLOG_TYPE_PROCEXIT:
			case PMCLOG_TYPE_PROCFORK:
			case PMCLOG_TYPE_USERDATA:
			case PMCLOG_TYPE_SYSEXIT:
				break;

			case PMCLOG_TYPE_MAP_IN:
				profiler.processMapIn(pmcEvent.pl_u.pl_mi.pl_pid, pmcEvent.pl_u.pl_mi.pl_start,
						      pmcEvent.pl_u.pl_mi.pl_pathname);
				break;

			case PMCLOG_TYPE_MAP_OUT:
				/* XXX Should handle this type of event. */
				break;

			case PMCLOG_TYPE_PCSAMPLE:
				profiler.processEvent(Sample(pmcEvent.pl_u.pl_s));
				break;

			case PMCLOG_TYPE_CALLCHAIN:
				profiler.processEvent(Sample(pmcEvent.pl_u.pl_cc));
				break;

			case PMCLOG_TYPE_PROCEXEC:
				profiler.processEvent(ProcessExec(pmcEvent.pl_u.pl_x.pl_pid,
				    std::string(pmcEvent.pl_u.pl_x.pl_pathname),
				    pmcEvent.pl_u.pl_x.pl_entryaddr));
				break;

			default:
				warn("unknown pmc event type %d\n", pmcEvent.pl_type);

		}
	}

	pmclog_close(logCookie);
	close(fd);
}
