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

#include "Profiler.h"
#include "EventFactory.h"
#include "Process.h"
#include "Image.h"
#include "ProcessState.h"
#include "Sample.h"
#include "ProfilePrinter.h"
#include <cassert>

#include <paths.h>
#include <libgen.h>

void
Profiler::createProfile(ProfilePrinter & printer)
{
    m_sampleCount = 0;
    Process::clearOldSamples();
    EventFactory::createEvents(*this, printer.getMaxDepth());
    Process::ActiveProcessList activeProcessList;
    Process::collectActiveProcesses(activeProcessList); 

    printer.printProfile(*this, activeProcessList);
}

void
Profiler::processEvent(const ProcessExec& processExec)
{
    Process::getProcess(processExec);
}

void
Profiler::processEvent(const Sample& sample)
{
    Process::getProcess(sample).addSample(sample);
    m_sampleCount++;
}

void 
Profiler::processMapIn(pid_t pid, uintptr_t map_start, const char * image)
{
    /* a pid of -1 indicates that this is for the kernel, but we don't have a process
     * for the kernel.  Load the images directly to get around this:
     */
    if(pid == -1) 
    {
        Image::loadKldImage(map_start, image);
    }
    else
    {
        Process & process = Process::getProcess(image, pid);
        process.mapIn(map_start, image);
    }
}

