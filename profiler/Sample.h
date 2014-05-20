#if !defined(SAMPLE_H)
#define SAMPLE_H

extern "C" {
#include <sys/types.h>
#include <sys/param.h>
#include <pmclog.h>
}

#include <portable/hash_fun>
#include <portable/hash_fun_extension>

#include <algorithm>
#include <vector>

class Sample
{
    bool m_isKernel;
    pid_t m_processID;
    std::vector<uintptr_t> m_address;

public:
    Sample(const pmclog_ev_callchain & event, uint32_t maxDepth)
      : m_isKernel(!PMC_CALLCHAIN_CPUFLAGS_TO_USERMODE(event.pl_cpuflags)),
        m_processID(event.pl_pid)
    {
        uint32_t i;
        uint32_t depth = std::min(maxDepth, event.pl_npc);
        for(i = 0; i < depth; i++) {
            m_address.push_back(event.pl_pc[i]-1);
        }
    }
    
    Sample(const pmclog_ev_pcsample & event)
      : m_isKernel(event.pl_usermode == 0),
        m_processID(event.pl_pid)
    {
        m_address.push_back(event.pl_pc-1);
    }

    bool isKernel() const
    {
        return m_isKernel;
    }

    unsigned getProcessID() const
    {
        return m_processID;
    }

    uintptr_t getAddress(size_t index) const
    {
        return m_address[index];
    }

    int getChainDepth() const
    {
        return m_address.size();
    }

    bool operator==(const Sample & other) const
    {
        if(m_isKernel != other.m_isKernel)
            return false;

        if(m_processID != other.m_processID)
            return false;

        if(getChainDepth() != other.getChainDepth())
            return false;

        for(int i = 0; i < getChainDepth(); i++) {
            if(m_address[i] != other.m_address[i]) {
                return false;
            }
        }

        return true;
    } 
    
    struct hash
    {
        size_t operator()(const Sample & sample) const
        {
            int i;
            size_t val = sample.isKernel();
            for(i = 0; i < sample.getChainDepth(); i++) {
                val += svhash::hash<uintptr_t>()(sample.getAddress(i));
            }

            return val;
        }
    };

};

#endif // #if !defined(SAMPLE_H)
