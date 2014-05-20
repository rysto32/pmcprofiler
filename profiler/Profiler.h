#if !defined(PROFILER_H)
#define PROFILER_H

#include <string>
#include <vector>
#include <stdint.h>

class Process;
class ProcessExit;
class ProcessExec;
class Sample;
class ProfilePrinter;

class Profiler
{
private:

    unsigned m_sampleCount;

    const std::string& m_dataFile;
    bool m_showlines;
    bool m_offline;

public:

    Profiler(const std::string& dataFile, bool showlines, bool offline) :
        m_sampleCount(0),
        m_dataFile( dataFile ),
        m_showlines(showlines),
        m_offline(offline)
    {
    }

    const std::string& getDataFile() const
    {
        return m_dataFile;
    }

    unsigned getSampleCount() const
    {
        return m_sampleCount;
    }

    bool showLines() const
    {
        return m_showlines;
    }

    void createProfile(ProfilePrinter & printer);

    void processEvent( const ProcessExec& processExec );
    void processEvent( const ProcessExit& processExit );
    void processEvent( const Sample& sample );
    void processMapIn(pid_t pid, uintptr_t map_start, const char * image);
};

#endif // #if !defined(PROFILER_H)

