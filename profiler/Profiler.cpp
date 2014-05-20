#include <Profiler.h>
#include <EventFactory.h>
#include <Process.h>
#include <Image.h>
#include <ProcessState.h>
#include <Sample.h>
#include <ProfilePrinter.h>
#include <cassert>

extern "C" {
#include <paths.h>
#include <libgen.h>
}

void
Profiler::createProfile(ProfilePrinter & printer)
{
    m_sampleCount = 0;
    Process::clearOldSamples();
    if(!m_offline)
        Process::fillProcessMap();
    EventFactory::createEvents(*this, printer.getMaxDepth());
    Process::ActiveProcessList activeProcessList;
    Process::collectActiveProcesses( activeProcessList ); 

    printer.printProfile(*this, activeProcessList);
}

void
Profiler::processEvent( const ProcessExec& processExec )
{
    Process::getProcess(processExec, m_offline);
}

void
Profiler::processEvent( const Sample& sample )
{
    Process::getProcess(sample, m_offline).addSample( sample );
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
        Process & process = Process::getProcess(image, pid, m_offline);
        process.mapIn(map_start, image);
    }
}

