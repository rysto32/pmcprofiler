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

#if !defined(PROCESS_H)
#define PROCESS_H

#include <Sample.h>
#include <Image.h>

#include <string>
#include <vector>
#include <portable/hash_map>
#include <bfd.h>

class Process
{
public:
    typedef std::hash_map<pid_t, Process*> ProcessMap;
    typedef std::hash_map<Sample, unsigned, Sample::hash> SampleMap;
    typedef std::map<bfd_vma, std::string> LoadableImageMap;

private:
    static ProcessMap processMap;

    pid_t m_pid;
    unsigned m_sampleCount;
    unsigned m_numCallchains;
    std::string m_name;
    bool m_loadedLibs;

    SampleMap m_samples;

    LoadableImageMap m_loadableImageMap;
    FunctionLocationMap m_functionLocationMap;
    CallchainMap m_callchainMap;

    Process(const Sample & sample, bool offline);
    Process(const ProcessExec& processExec, bool offline);
    Process(const char * name, pid_t pid, bool offline);

    class ProcessSampleCompare
    {
    public:
        bool operator()( const Process* lhs, const Process* rhs )
        {
            return lhs -> m_sampleCount > rhs -> m_sampleCount;
        }
    };

public:
    typedef std::vector< Process* > ActiveProcessList;

    static void clearOldSamples();
    static void fillProcessMap();
    static void freeProcessMap();

    static Process& getProcess(const Sample& sample, bool offline);
    static Process& getProcess(const ProcessExec& processExec, bool offline);
    static Process& getProcess(const char * name, pid_t pid, bool offline);

    static Process* getProcess( pid_t pid )
    {
        ProcessMap::iterator it = processMap.find(pid);
        if(it == processMap.end())
            return NULL;
        return it->second;
    }

    static void collectAllLocations( LocationList& locationList );

    template <typename ProcessStrategy>
    static void mapAllFunctions(LocationList& locationList, ProcessStrategy strategy);

    static void collectActiveProcesses( ActiveProcessList& activeProcessList );

    std::string getLoadableImageName( const Location& location, uintptr_t& loadOffset );

    pid_t getPid() const
    {
        return m_pid;
    }

    const std::string& getName() const
    {
        return m_name;
    }

    unsigned getSampleCount() const
    {
        return m_sampleCount;
    }

    void addSample( const Sample& sample );
    void collectLocations( LocationList& locationList );
    void getFunctionList( FunctionList& functionList );
    unsigned getCallers(const Callchain & chain, std::vector<FunctionLocation> & functions);

    void mapIn(uintptr_t start, const char * imagePath);
};

template <typename ProcessStrategy>
void
Process::mapAllFunctions(LocationList& locationList, ProcessStrategy strategy)
{
    /* try to avoid hash-table resizes by pre-allocating a large enough hash map */
    for(ProcessMap::iterator it = processMap.begin(); it != processMap.end(); ++it)
    {
        Process * process = it->second;
        process->m_callchainMap.resize(4*process->m_numCallchains/3);
    }

    for (LocationList::iterator it = locationList.begin(); it != locationList.end(); ++it)
    {
        typename ProcessStrategy::iterator jt = strategy.begin(*it);
        typename ProcessStrategy::iterator jt_end = strategy.end(*it);

        if(jt == jt_end)
            continue;

        Location& location(*jt);
        const char* functionName = location.getFunctionName();
        const char* fileName = location.getFileName();
        std::string key(fileName); 
        key += functionName;
        Process & process = *getProcess(location.getPid());
        FunctionLocationMap& functionLocationMap(process.m_functionLocationMap);

        std::pair<FunctionLocationMap::iterator, bool> findPos = 
            functionLocationMap.insert(FunctionLocationMap::value_type(key, location));

        if (!findPos.second)
        {
            findPos.first->second += location;
        }

        Callchain callchain;
        callchain.push_back(functionName);
        ++jt;
        for(; jt != jt_end; ++jt)
        {
            FunctionLocation funcLoc = *jt;

            if(!funcLoc.isMapped()) {
                funcLoc.setFunctionName("[unmapped_function]");
            }
            
            std::pair<CallchainMap::iterator, bool> mapInserted = 
                process.m_callchainMap.insert(CallchainMap::value_type(callchain, FunctionLocationMap(1)));
            std::pair<FunctionLocationMap::iterator, bool> inserted = 
                mapInserted.first->second.insert(FunctionLocationMap::value_type(funcLoc.getFunctionName(), funcLoc));

            if (!inserted.second)
            {
                inserted.first->second += funcLoc;
            }

            callchain.push_back(jt->getFunctionName());
        }

        strategy.processEnd(*it, process.m_callchainMap, callchain);
    }
}


#endif // #if !defined(PROCESS_H)
