#ifndef PROFILER_PRINTER_H
#define PROFILER_PRINTER_H

#include <Profiler.h>
#include <Image.h>
#include <Process.h>
#include <cstdio>
#include <cassert>

class ProfilePrinter
{
protected:
    FILE * m_outfile;
    uint32_t m_maxDepth;

public:
    ProfilePrinter(FILE * file, uint32_t maximumDepth)
      : m_outfile(file),
        m_maxDepth(maximumDepth)
    {
    }

    uint32_t getMaxDepth() const
    {
        return m_maxDepth;
    }
    
    virtual void printProfile(const Profiler & profiler, 
                              const Process::ActiveProcessList & activeProcessList) = 0;

    virtual ~ProfilePrinter()
    {
        if(m_outfile != stdout)
            fclose(m_outfile);
    }

    void printLineNumbers(const Profiler & profiler, const LineLocationList & functionLocation);
};

class FlatProfilePrinter : public ProfilePrinter
{
public:
    FlatProfilePrinter(FILE * file)
        : ProfilePrinter(file, 1)
    {
    }
    
    virtual void printProfile(const Profiler & profiler,
                              const Process::ActiveProcessList & activeProcessList);
};

template <class ProcessStrategy, class PrintStrategy>
class CallchainProfilePrinter : public ProfilePrinter
{
    int m_threshold;
    bool m_printBoring;

    void printCallChain(const Profiler & profiler, Process& process, Callchain & chain, int depth, PrintStrategy &strategy);
    bool isCallChainBoring(Process& process, Callchain & chain);

public:
    CallchainProfilePrinter(FILE * file, uint32_t maximumDepth, int threshold, bool printBoring)
      : ProfilePrinter(file, maximumDepth), 
        m_threshold(threshold), 
        m_printBoring(printBoring)
    {
    }
    
    virtual void printProfile(const Profiler & profiler,
                              const Process::ActiveProcessList & activeProcessList);
};


struct PrintCallchainStrategy
{
    void printFileHeader(FILE *outfile, const Profiler &profiler)
    {
        fprintf(outfile, "Events processed: %u\n", profiler.getSampleCount());
    }
    
    void printProcessHeader(FILE *outfile, const Profiler &profiler, Process &process)
    {
        fprintf(outfile, "\nProcess: %6u, %s, total: %u (%6.2f%%)\n", process.getPid(), process.getName().c_str(),
            process.getSampleCount(), (process.getSampleCount() * 100.0) / profiler.getSampleCount());
    }
    
    void printFrame(FILE *outfile, int depth, double processPercent, double parentPercent, 
        ProfilePrinter &printer, const Profiler &profiler, FunctionLocation& functionLocation, 
        Process &process, const char *functionName, Callchain & chain __unused)
    {
        for(int i = 0; i < depth; i++) {
            fprintf(outfile, "  ");
        }

        fprintf(outfile, "[%d] %.2f%% %.2f%%(%d/%d) %s", depth, parentPercent, processPercent,
                    functionLocation.getCount(), process.getSampleCount(), functionName);
        printer.printLineNumbers(profiler, functionLocation.getLineLocationList());
        fprintf(outfile, "\n");
    }
};

struct PrintFlameGraphStrategy
{
    void printFileHeader(FILE *outfile __unused, const Profiler &profiler __unused)
    {
    }
    
    void printProcessHeader(FILE *outfile __unused, const Profiler &profiler __unused, Process &process __unused)
    {
    }
    
    void printFrame(FILE *outfile, int depth __unused, double processPercent __unused, double parentPercent __unused, 
        ProfilePrinter &printer __unused, const Profiler &profiler __unused, FunctionLocation& functionLocation, 
        Process &process __unused, const char *functionName, Callchain & chain)
    {
        if (strcmp(functionName, "[self]") == 0) {
            const char *sep = "";
            for (Callchain::iterator it = chain.begin(); it != chain.end(); ++it) {
                char *demangled = Image::demangle(*it);
                fprintf(outfile, "%s%s", sep, demangled);
                free(demangled);
                sep = ";";
            }

            fprintf(outfile, " %d\n", functionLocation.getCount());
        }
    }
};


struct LeafProcessStrategy
{
    typedef std::vector<Location>::iterator iterator;

    iterator begin(std::vector<Location> & vec)
    {
        return vec.begin();
    }

    iterator end(std::vector<Location> & vec)
    {
        return vec.end();
    }

    void processEnd(std::vector<Location> & vec __unused, CallchainMap & callchainMap __unused, const Callchain & callchain __unused)
    {
    }
};

struct RootProcessStrategy
{
    typedef std::vector<Location>::reverse_iterator iterator;

    iterator begin(std::vector<Location> & vec)
    {
        iterator it = vec.rbegin();

        while(!it->isMapped() && it != end(vec)) {
            ++it;
        }

        return it;
    }

    iterator end(std::vector<Location> & vec)
    {
        return vec.rend();
    }

    void processEnd(std::vector<Location> & vec, CallchainMap & callchainMap, const Callchain & callchain)
    {
        std::pair<CallchainMap::iterator, bool> mapInserted = 
            callchainMap.insert(CallchainMap::value_type(callchain, FunctionLocationMap(1)));
        std::pair<FunctionLocationMap::iterator, bool> inserted = 
            mapInserted.first->second.insert(FunctionLocationMap::value_type("[self]", FunctionLocation(vec.front())));
        if(!inserted.second)
            inserted.first->second += vec.front();
        else
            inserted.first->second.setFunctionName("[self]");
    }
};

template <class ProcessStrategy, class PrintStrategy>
bool
CallchainProfilePrinter<ProcessStrategy, PrintStrategy>::isCallChainBoring(Process& process, Callchain & chain)
{
    std::vector<FunctionLocation> functions;
    process.getCallers(chain, functions);

    if(functions.size() > 1)
        return false;
    else if(functions.size() == 0)
        return true;
    else
    {
        chain.push_back(functions.front().getFunctionName());
        bool boring = isCallChainBoring(process, chain);
        assert(chain.back() == functions.front().getFunctionName());
        chain.pop_back();
        return boring;
    }
}

template <class ProcessStrategy, class PrintStrategy>
void
CallchainProfilePrinter<ProcessStrategy, PrintStrategy>::printCallChain(const Profiler & profiler, Process& process, Callchain & chain, int depth, PrintStrategy &strategy)
{
    std::vector<FunctionLocation> functions;
    unsigned total_samples = process.getCallers(chain, functions);

    bool isBoring = false;
    if(!m_printBoring && functions.size() == 1) {
        isBoring = isCallChainBoring(process, chain);
    }

    std::vector<FunctionLocation>::iterator it = functions.begin();
    for(; it != functions.end(); ++it)
    {
        double parent_percent = (it->getCount() * 100.0) / total_samples;
        double total_percent = (it->getCount() * 100.0) / process.getSampleCount();

        if(total_percent < m_threshold)
            continue;

        char * functionName = Image::demangle(it->getFunctionName());
        strategy.printFrame(m_outfile, depth, parent_percent, total_percent, *this, profiler, *it, 
            process, functionName, chain);
        free(functionName);

        if(!isBoring)
        {
            chain.push_back(it->getFunctionName());
            printCallChain(profiler, process, chain, depth + 1, strategy);
            assert(chain.back() == it->getFunctionName());
            chain.pop_back();
        }
    }
}

template <class ProcessStrategy, class PrintStrategy>
void 
CallchainProfilePrinter<ProcessStrategy, PrintStrategy>::printProfile(const Profiler & profiler,
    const Process::ActiveProcessList & activeProcessList)
{
    PrintStrategy strategy;
    strategy.printFileHeader(m_outfile, profiler);

    for ( Process::ActiveProcessList::const_iterator processListIterator = activeProcessList.begin();
        processListIterator != activeProcessList.end(); ++processListIterator )
    {
        Process& process( **processListIterator );
        
        LocationList locationList;
        process.collectLocations(locationList);
        std::sort(locationList.begin(), locationList.end());
        Image::mapAllLocations(locationList);
        Process::mapAllFunctions<ProcessStrategy>(locationList, ProcessStrategy());

        strategy.printProcessHeader(m_outfile, profiler, process);
        FunctionList functionList;
        process.getFunctionList( functionList );
        for ( FunctionList::iterator functionListIterator = functionList.begin(); functionListIterator != functionList.end(); ++functionListIterator )
        {
            FunctionLocation& functionLocation( *functionListIterator );

            double percent = (functionLocation.getCount() * 100.0) / process.getSampleCount();
            if(percent >= m_threshold)
            {
                Callchain chain;

                char * functionName = Image::demangle(functionLocation.getFunctionName());
                strategy.printFrame(m_outfile, 0, percent, percent, *this, profiler, functionLocation,
                    process, functionName, chain);

                chain.push_back(functionLocation.getFunctionName());
                printCallChain(profiler, process, chain, 1, strategy);
                free(functionName);
            }
        }
    }
}

typedef CallchainProfilePrinter<LeafProcessStrategy, PrintCallchainStrategy> LeafProfilePrinter;
typedef CallchainProfilePrinter<RootProcessStrategy, PrintCallchainStrategy> RootProfilePrinter;
typedef CallchainProfilePrinter<RootProcessStrategy, PrintFlameGraphStrategy> FlameGraphProfilerPrinter;

#endif

