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

