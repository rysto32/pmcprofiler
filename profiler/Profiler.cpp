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

#include "Profiler.h"

#include "AddressSpace.h"
#include "EventFactory.h"
#include "Image.h"
#include "ProcessState.h"
#include "Sample.h"
#include "ProfilePrinter.h"
#include "SampleAggregation.h"

#include <cassert>
#include <cstring>
#include <memory>

#include <paths.h>
#include <libgen.h>
#include <sys/sysctl.h>

Profiler::Profiler(const std::string& dataFile, bool showlines,
    const char* modulePathStr)
  : m_sampleCount(0),
    m_dataFile(dataFile),
    m_showlines(showlines)
{
	if (modulePathStr != NULL)
		overrideModulePath(modulePathStr);
	else
		getLocalModulePath();
}

void
Profiler::MapSamples(uint32_t maxDepth)
{
	m_sampleCount = 0;
	AddressSpace::clearAddressSpaces();
	SampleAggregation::clearAggregations();

	EventFactory::createEvents(*this, maxDepth);
	Image::mapAll();
}

void
Profiler::createProfile(ProfilePrinter & printer)
{
	AggregationList aggregations;
	SampleAggregation::getAggregationList(aggregations);

	printer.printProfile(*this, aggregations);
}

void
Profiler::processEvent(const ProcessExec& processExec)
{
	SampleAggregation::processExec(processExec);
	AddressSpace::processExec(processExec);
}

void
Profiler::processEvent(const Sample& sample)
{
	if (!pid_filter.empty() && pid_filter.count(sample.getProcessID()) == 0)
		return;

	bool kernel = sample.isKernel();
	AddressSpace &space = AddressSpace::getAddressSpace(kernel,
	    sample.getProcessID());

	SampleAggregation::getAggregation(sample).addSample(space, sample);
	m_sampleCount++;
}

void
Profiler::processMapIn(pid_t pid, uintptr_t map_start, const char * image)
{
	/* a pid of -1 indicates that this is for the kernel */
	if (pid == -1) {
		AddressSpace &space = AddressSpace::getKernelAddressSpace();
		space.findAndMap(map_start, modulePath, image);
	} else {
		AddressSpace &space = AddressSpace::getProcessAddressSpace(pid);
		space.mapIn(map_start, image);
	}

	SampleAggregation::processMapIn(pid, image);
}

void
Profiler::parseModulePath(char * pathBuf, std::vector<std::string> & vec)
{
	char * path;
	char * next = pathBuf;

	vec.clear();

	while ((path = strsep(&next, ";")) != NULL) {
		if (*path == '\0')
			continue;

		vec.push_back(path);
	}
}

void
Profiler::getLocalModulePath()
{
	std::unique_ptr<char[]> pathBuf;
	size_t path_len = 0;
	int error;

	sysctlbyname("kern.module_path", NULL, &path_len, NULL, 0);
	pathBuf = std::make_unique<char[]>(path_len+1);

	error = sysctlbyname("kern.module_path", pathBuf.get(), &path_len, NULL, 0);
	if (error == 0)
		parseModulePath(pathBuf.get(), modulePath);
}

void
Profiler::overrideModulePath(const char* modulePathStr)
{
	size_t len = strlen(modulePathStr) + 1;
	std::unique_ptr<char[]> pathBuf = std::make_unique<char[]>(len);
	memcpy(pathBuf.get(), modulePathStr, len);

	parseModulePath(pathBuf.get(), modulePath);
}
