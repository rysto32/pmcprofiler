//
//  sharedLib.h
// 
// get information concerning loaded shared libraries given a pid
// 
#ifndef GETSHARED_H
#define GETSHARED_H

#include <stdio.h>

#include <string>
#include <vector>

class sharedLib
{
private:
    std::string m_name;
    uintptr_t    m_base;
public:
    sharedLib(const std::string& name, uintptr_t base) :
        m_name(name), m_base(base)
    {
    }
    const std::string& getName() const {return m_name;}
    uintptr_t getBase() const {return m_base;}
};

class sharedLibInfo
{
private:
    std::vector<sharedLib>  m_libs;
    std::string             m_procBase;
    unsigned long           m_dynamicBase;
    unsigned                m_dynamicSize;
    int                     m_cpuArchSize;
public:
    sharedLibInfo(unsigned pid);
    unsigned numLibs() const;
    const sharedLib& getLib(unsigned index) const;
private:
    void processFileImage();

    template <typename ElfDyn, typename r_debug_struct, typename LinkMap>
    void processRunningImage();
    std::string readString(uintptr_t addr, FILE* image);
};

#endif

