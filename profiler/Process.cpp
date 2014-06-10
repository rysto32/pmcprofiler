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

#include "Process.h"
#include "Sample.h"
#include "ProcessState.h"
#include "Image.h"

#include <algorithm>
#include <cstring>

#include <fcntl.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/stat.h>

Process::ProcessMap Process::processMap;

void
Process::clearOldSamples()
{
	Process::ProcessMap::iterator it = Process::processMap.begin();
	for(; it != Process::processMap.end(); ++it)
	{
		it->second->m_samples.clear();
		it->second->m_functionLocationMap.clear();
		it->second->m_callchainMap.clear();
		it->second->m_sampleCount = 0;
		it->second->m_numCallchains = 0;
	}
}

void
Process::fillProcessMap()
{
}

void
Process::freeProcessMap()
{
	for(ProcessMap::iterator it = processMap.begin(); it != processMap.end(); ++it)
	{
		delete it->second;
	}
	
	processMap.clear();
}

Process::Process(const Sample& sample) :
m_pid(sample.getProcessID()),
m_sampleCount(0),
m_numCallchains(0),
m_samples(1),
m_functionLocationMap(1),
m_callchainMap(1)
{
}

Process::Process(const ProcessExec& processExec) :
m_pid(processExec.getProcessID()),
m_sampleCount(0),
m_numCallchains(0),
m_name(processExec.getProcessName()),
m_samples(1),
m_functionLocationMap(1),
m_callchainMap(1)
{
}

Process::Process(const char * name, pid_t pid) :
m_pid(pid),
m_sampleCount(0),
m_numCallchains(0),
m_name(name),
m_samples(1),
m_functionLocationMap(1),
m_callchainMap(1)
{
}

std::string
Process::getLoadableImageName(const Location& location, uintptr_t& loadOffset)
{
	loadOffset = 0;
	
	LoadableImageMap::iterator it = m_loadableImageMap.lower_bound(location.getAddress());
	
	if(it == m_loadableImageMap.begin())
		return "";
	
	--it;
	loadOffset = it->first;
	
	return it->second;
}

Process&
Process::getProcess(const Sample& sample)
{
	pid_t pid = sample.getProcessID();
	Process* process = processMap[pid];
	
	if (process == 0)
	{
		process = processMap[pid] = new Process(sample);
	}
	return *process;
}

Process&
Process::getProcess(const ProcessExec& processExec)
{
	pid_t pid = processExec.getProcessID();
	Process* process = processMap[pid];
	
	if (process == 0)
	{
		process = processMap[pid] = new Process(processExec);
	}
	
	if ((*process).m_name.empty() && !processExec.getProcessName().empty())
	{
		process->m_name = processExec.getProcessName();
	}
	
	return *process;
}

Process&
Process::getProcess(const char * name, pid_t pid)
{
	Process* process = processMap[pid];
	
	if(process == 0)
	{
		/* we use the name of the first map-in file as our name, as that
		 * should be the name of our executable
		 */
		process = processMap[pid] = new Process(name, pid);
	}
	
	return *process;
}

void
Process::addSample(const Sample& sample)
{    
	m_samples[sample]++;
	m_sampleCount++;
	m_numCallchains += sample.getChainDepth();
	
}

void
Process::collectLocations(LocationList& locationList)
{ 
	for(SampleMap::iterator it = m_samples.begin(); it != m_samples.end(); ++it)
	{
		std::vector<Location> stack;
		stack.reserve(it->first.getChainDepth());
		for(int i = 0; i < it->first.getChainDepth(); i++) {
			stack.push_back(Location(it->first.isKernel(), m_pid, it->first.getAddress(i), it->second));
		}
		locationList.push_back(stack);
	}
	m_samples.clear();
} 

void
Process::collectActiveProcesses(ActiveProcessList& activeProcessList)
{
	for(ProcessMap::const_iterator it = processMap.begin(); it != processMap.end(); ++it)
	{
		Process* process((*it).second);
		
		if (process->m_sampleCount > 0)
		{
			activeProcessList.push_back(process);
		}
	}
	std::sort(activeProcessList.begin(), activeProcessList.end(), ProcessSampleCompare());
}

void
Process::collectAllLocations(LocationList& locationList)
{
	for(ProcessMap::iterator it = processMap.begin(); it != processMap.end(); ++it)
	{
		(*it).second->collectLocations(locationList);
	}
	std::sort(locationList.begin(), locationList.end());
}

void
Process::getFunctionList(FunctionList& functionList)
{
	for(FunctionLocationMap::iterator it = m_functionLocationMap.begin();
	    it != m_functionLocationMap.end(); ++it)
	    {
		    FunctionLocation functionLocation((*it).second);
		    Image::mapFunctionStart(functionLocation);
		    functionList.push_back(functionLocation);
	    }
	    m_functionLocationMap.clear();
	std::sort(functionList.begin(), functionList.end());
}

unsigned
Process::getCallers(const Callchain & chain, std::vector<FunctionLocation> & functions)
{
	CallchainMap::const_iterator it = m_callchainMap.find(chain);
	unsigned total_samples = 0;
	
	if(it != m_callchainMap.end())
	{
		const FunctionLocationMap & count = it->second;
		functions.reserve(count.size());
		
		FunctionLocationMap::const_iterator func = count.begin();
		for(; func != count.end(); ++func)
		{
			functions.push_back(func->second);
			total_samples += func->second.getCount();
		}
		
		std::sort(functions.begin(), functions.end());
	}
	
	return total_samples;
} 

void 
Process::mapIn(uintptr_t start, const char * imagePath)
{
	m_loadableImageMap.insert(LoadableImageMap::value_type(start, imagePath));
}

