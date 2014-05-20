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

#include <stddef.h>
#include "sharedLib.h"
#include <bfd.h>
#include <elf.h>
#include <sys/link_elf.h>

#include <errno.h>
#include <string.h>

/* the r_debug and Link_map structures differ for 64-bit and 32-bit executables,
 * so we define them here with fixed-width types.  This allows us to properly
 * map executables with a word size different from our own
 */
struct r_debug32 {
	int		r_version;		/* not used */
	uint32_t r_map;			/* list of loaded images */
	uint32_t r_brk;
						/* pointer to break point */
	enum {
	    RT_CONSISTENT,			/* things are stable */
	    RT_ADD,				/* adding a shared library */
	    RT_DELETE				/* removing a shared library */
	}		r_state;
};

typedef struct  {
 uint32_t l_addr;
 uint32_t l_name;
 uint32_t l_ld;
 uint32_t l_next, l_prev;
} Link_map32;


struct r_debug64 {
	int		r_version;		/* not used */
	uint64_t r_map;			/* list of loaded images */
	uint64_t r_brk;
						/* pointer to break point */
	enum {
	    RT_CONSISTENT,			/* things are stable */
	    RT_ADD,				/* adding a shared library */
	    RT_DELETE				/* removing a shared library */
	}		r_state;
};

typedef struct  {
 uint64_t l_addr;
 uint64_t l_name;
 uint64_t l_ld;
 uint64_t l_next, l_prev;
} Link_map64;

#ifndef CTASSERT		/* Allow lint to override */
#define	CTASSERT(x)		_CTASSERT(x, __LINE__)
#define	_CTASSERT(x, y)		__CTASSERT(x, y)
#define	__CTASSERT(x, y)	typedef char __assert ## y[(x) ? 1 : -1]
#endif

#if __ELF_WORD_SIZE == 64

CTASSERT(sizeof(r_debug64) == sizeof(r_debug));
CTASSERT(offsetof(r_debug64, r_version) == offsetof(r_debug, r_version));
CTASSERT(offsetof(r_debug64, r_map) == offsetof(r_debug, r_map));
CTASSERT(offsetof(r_debug64, r_brk) == offsetof(r_debug, r_brk));
CTASSERT(offsetof(r_debug64, r_state) == offsetof(r_debug, r_state));

CTASSERT(sizeof(Link_map64)== sizeof(Link_map));
CTASSERT(offsetof(Link_map64, l_addr) == offsetof(Link_map, l_addr));
CTASSERT(offsetof(Link_map64, l_name) == offsetof(Link_map, l_name));
CTASSERT(offsetof(Link_map64, l_ld) == offsetof(Link_map, l_ld));
CTASSERT(offsetof(Link_map64, l_next) == offsetof(Link_map, l_next));
CTASSERT(offsetof(Link_map64, l_prev) == offsetof(Link_map, l_prev));

#elif __ELF_WORD_SIZE == 32

CTASSERT(sizeof(r_debug32) == sizeof(r_debug));
CTASSERT(offsetof(r_debug32, r_version) == offsetof(r_debug, r_version));
CTASSERT(offsetof(r_debug32, r_map) == offsetof(r_debug, r_map));
CTASSERT(offsetof(r_debug32, r_brk) == offsetof(r_debug, r_brk));
CTASSERT(offsetof(r_debug32, r_state) == offsetof(r_debug, r_state));

CTASSERT(sizeof(Link_map32)== sizeof(Link_map));
CTASSERT(offsetof(Link_map32, l_addr) == offsetof(Link_map, l_addr));
CTASSERT(offsetof(Link_map32, l_name) == offsetof(Link_map, l_name));
CTASSERT(offsetof(Link_map32, l_ld) == offsetof(Link_map, l_ld));
CTASSERT(offsetof(Link_map32, l_next) == offsetof(Link_map, l_next));
CTASSERT(offsetof(Link_map32, l_prev) == offsetof(Link_map, l_prev));

#else

#warning "Unknown __ELF_WORD_SIZE"

#endif
    

sharedLibInfo::sharedLibInfo(unsigned pid)
{
    char base[2048]; // shut up
    sprintf(base, "/proc/%u/", pid);
    m_procBase = std::string(base);
    processFileImage();

    switch(m_cpuArchSize) {
    case 32:
        processRunningImage<Elf32_Dyn, struct r_debug32, Link_map32>();
        break;
    case 64:
        processRunningImage<Elf64_Dyn, struct r_debug64, Link_map64>();
        break;
    default:
        fprintf(stderr, "Unrecognized arch size %d", m_cpuArchSize);
        throw 5;
    }
}

unsigned sharedLibInfo::numLibs() const
{
    return m_libs.size();
}

const sharedLib& sharedLibInfo::getLib(unsigned index) const
{
    return m_libs[index];
}

void sharedLibInfo::processFileImage()
{
    bfd_init(); // I wonder if this can be called many times?

    std::string image = m_procBase + "file";
    bfd* bfdHandle = bfd_openr(image.c_str(), "default");
    if (bfdHandle == 0)
    {
        throw 5;
    }
    // from this point on we need to at least close the bfd on an error
    try
    {
        // make sure we have an object file (executable)
        if (bfd_check_format(bfdHandle, bfd_object) == 0)
        {
            throw 5;
        }
        
        int arch_size = bfd_get_arch_size (bfdHandle);
        if (arch_size == -1) 
        {
            throw 5;
        }

        m_cpuArchSize = arch_size;

        // now find out the load address of the .dynamic section
        struct bfd_section* dyninfo_sect = bfd_get_section_by_name (bfdHandle, ".dynamic");
        if (dyninfo_sect == 0)
        {
            throw 5;
        }
        bfd_vma dyninfo_addr = bfd_section_vma (bfdHandle, dyninfo_sect);
        m_dynamicSize = bfd_section_size (exec_bfd, dyninfo_sect);
        m_dynamicBase = dyninfo_addr;
        //printf(".dynamic vma is: %08lx\n", m_dynamicBase);
        //printf(".dynamic size is %u\n", m_dynamicSize);
    }
    catch (...)
    {
        bfd_close(bfdHandle);
        throw;
    }
    // that's all we need to do for now
    bfd_close(bfdHandle);
}

template <typename ElfDyn, typename RDebug, typename LinkMap>
void sharedLibInfo::processRunningImage()
{
    // the idea here is to open the procfs image, read through the
    // .dynamic section in order to find where the list of shared libraries
    // is, then read out those libraries...

    std::string running = m_procBase + "mem";

    FILE* image = fopen(running.c_str(), "r");
    if (image == 0)
    {
        throw 5;
    }
    ElfDyn* dyn_entries = 0;  // since we can delete 0 if we need to...
    off_t lib_ptr = 0;
    try
    {
        unsigned entries = m_dynamicSize / sizeof(ElfDyn);
        // read in the (hopefully) .dynamic section
        try
        {
            dyn_entries = new ElfDyn[entries];
        }
        catch (...)
        {
            throw 5;
        }
        if (fseeko(image, m_dynamicBase, SEEK_SET) != 0)
        {
            throw 5;
        }
        if (fread(dyn_entries, sizeof(ElfDyn), entries, image) != entries)
        {
            throw 5;
        }
        // we should now have a table of dynamic entries...
        for (unsigned entry = 0; entry < entries; ++entry)
        {
            ElfDyn& dyn = dyn_entries[entry];
            //printf("%u: %08x %08x\n", entry, dyn.d_tag, dyn.d_un.d_ptr);
            if (dyn.d_tag == DT_DEBUG)
            {
                lib_ptr = dyn.d_un.d_ptr;
            }
        }
        if (lib_ptr == 0)
        {
            throw 5;
        }
        // lib_ptr now points to some good stuff...
        //printf("lib_ptr is: %08x\n", lib_ptr);
        // read in the cool r_debug stuff
        if (fseeko(image, lib_ptr, SEEK_SET) != 0)
        {
            throw 5;
        }
        RDebug rDebug;
        if (fread(&rDebug, sizeof(rDebug), 1, image) != 1)
        {
            throw 5;
        }
        //printf("pointer to lib list is %p\n", rDebug.r_map);
        // now that we know where the libraries are...
        off_t lm = (off_t)rDebug.r_map;
        bool skipped = false; // the first entry is the executable
        while (lm)
        {
            LinkMap actual;
            if (fseeko(image, off_t(lm), SEEK_SET) != 0)
            {
                throw 5;
            }
            if (fread(&actual, sizeof(actual), 1, image) != 1)
            {
                throw 5;
            }
            if (skipped)
            {
                sharedLib sl(readString(size_t(actual.l_name), image), size_t(actual.l_addr));
                m_libs.push_back(sl);
            }
            skipped = true;
            lm = (off_t)actual.l_next;
        }
    }
    catch (...)
    {
        delete [] dyn_entries;
        fclose(image);
        throw;
    }
    delete [] dyn_entries;
    fclose(image);
}

std::string sharedLibInfo::readString(uintptr_t addr, FILE* image)
{
    std::string s;
    if (fseek(image, addr, SEEK_SET) != 0)
    {
        throw 5;
    }
    int c = 0;
    do
    {
        c = fgetc(image);
        if (c == -1)
        {
            throw 5;
        }
        s += c;
    } while (c);
    return s;
}
