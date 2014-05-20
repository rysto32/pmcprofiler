#if !defined(PROCESSSTATE_H)
#define PROCESSSTATE_H

#include <string>

extern "C" {
#include <sys/types.h>
}

class ProcessState
{
    pid_t m_processID;

    const std::string& m_processName;

protected:
    ProcessState( pid_t& processID, const std::string& processName ) :
    m_processID( processID ),
    m_processName( processName )
    {
    }

public:
    pid_t getProcessID() const
    {
        return m_processID;
    }

    const std::string& getProcessName() const
    {
        return m_processName;
    }
};

class ProcessExec : public ProcessState
{
public:
    ProcessExec( pid_t& processID, const std::string& processName ) :
    ProcessState( processID, processName )
    {
    }
};

class ProcessExit : public ProcessState
{
public:
    ProcessExit( pid_t& processID ) :
    ProcessState( processID, "" )
    {
    }
};

#endif // #if !defined(PROCESSSTATE_H)
