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

#include "Image.h"
#include "Process.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h> 
#include <libiberty/demangle.h>
#include <paths.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/linker.h>
#include <sys/sysctl.h>
#include <unistd.h>

bool Image::firstTimeInit = true;

void Image::parseModulePath(char * path_buf, std::vector<std::string> & vec)
{
    char * path;
    char * next = path_buf;

    vec.clear();

    while((path = strsep(&next, ";")) != NULL) {
        if(*path == '\0')
            continue;

        vec.push_back(path);
    }
}

std::vector<std::string> Image::getModulePath()
{
    std::vector<std::string> vec;
    char * path_buf;
    size_t path_len = 0;
    int error;

    sysctlbyname("kern.module_path", NULL, &path_len, NULL, 0);
    path_buf = new char[path_len+1];

    error = sysctlbyname("kern.module_path", path_buf, &path_len, NULL, 0);

    if(!error) {
        parseModulePath(path_buf, vec);
    }

    delete [] path_buf;

    return vec;
}

std::string Image::KERNEL_NAME(getbootfile());
std::vector<std::string> Image::MODULE_PATH(Image::getModulePath());
const std::string Image::TEXT_SECTION_NAME( ".text" );

Image* Image::kernelImage = 0;

Image::ImageMap Image::imageMap;
bool Image::loadedKldModules = false;
Image::LoadableImageMap Image::kernelLoadableImageMap;

void 
Image::setBootfile(const char * file)
{
    KERNEL_NAME = file;

    delete kernelImage;
    kernelImage = NULL;
}

void 
Image::setModulePath(char * path)
{
    parseModulePath(path, MODULE_PATH);
}

Image&
Image::getKernel()
{
    if ( Image::kernelImage == 0 )
    {
        kernelImage = new Image( KERNEL_NAME );
    }
    return *kernelImage;
}

void
Image::setUnknownFile(const std::string & path)
{
    Image * newImage = new Image(path);

    std::pair<ImageMap::iterator, bool> inserted = 
        imageMap.insert(ImageMap::value_type("unknown", newImage));

    if(!inserted.second)
    {
        delete inserted.first->second;
        inserted.first->second = newImage;
    }
}

void
Image::freeImages()
{
    delete kernelImage;
    kernelImage = NULL;

    kernelLoadableImageMap.clear();

    Image::ImageMap::iterator it;
    for(it = imageMap.begin(); it != imageMap.end(); ++it)
    {
        delete it->second;
    }

    imageMap.clear();
}

Image::Image( const std::string& imageName ) :
    m_fd( 0 ),
    m_symCount( 0 ),
    m_dynCount( 0 ),
    m_bfdHandle( 0 ),
    m_startAddress( 0 ),
    m_textSize( 0 ),
    m_endAddress( 0 ),
    m_textSection( 0 ),
    m_symTable( 0 ),
    m_dynTable(0),
    m_symFuncCache(1),
    m_dynFuncCache(1)
{
    if ( firstTimeInit )
    {
        bfd_init();
        firstTimeInit = false;
    }

    std::string debugName = imageName + ".debug";
    const std::string * fileName;

    int test_fd = open(debugName.c_str(), O_RDONLY);
    if(test_fd < 0)
    {
        m_fd = open(imageName.c_str(), O_RDONLY);

        if(m_fd < 0)
        {
            if (imageName.size())
            {
                fprintf(stderr, "%s: unable to open file %s\n", 
                        g_quitOnError ? "error" : "warning", imageName.c_str());
                if (g_quitOnError)
                {
                    exit(5);
                }
            }
            return;
        }

        fileName = &imageName;
    }
    else
    {
        m_fd = test_fd;
        fileName = &debugName;
    }

    m_bfdHandle = bfd_fdopenr(fileName->c_str(), 0, m_fd);
    

    if ( m_bfdHandle == 0 )
    {
        return;
    }

    //must check the format!
    if ( bfd_check_format( m_bfdHandle, bfd_object ) == 0 )
    {
        return;
    }

    if ( ( bfd_get_file_flags( m_bfdHandle ) & HAS_SYMS ) == 0 )
    {
        return;
    }

    m_textSection = bfd_get_section_by_name( m_bfdHandle, TEXT_SECTION_NAME.c_str() );
    m_startAddress = bfd_get_section_vma( m_bfdHandle, m_textSection );
    m_textSize = bfd_get_section_size( m_textSection );
    m_endAddress = m_startAddress + m_textSize;
}

Image::~Image()
{
    if ( m_symTable != 0 )
    {
        free( m_symTable );
        m_symTable = 0;
    }

    if ( m_dynTable != 0 )
    {
        free( m_dynTable );
        m_dynTable = 0;
    }

    if ( m_bfdHandle != 0 )
    {
        bfd_close( m_bfdHandle );
        m_bfdHandle = 0;
    }
}

bool
Image::isOk() const
{
    return ( m_fd > 0 ) && ( m_textSize > 0 );
}

void
Image::loadSymbolCache(asymbol ** symtab, long symCount, FuncSymbolCache & symmap)
{
    for (long it = 0; it < symCount; it++)
    {
        asymbol* symbol = symtab[it];

        if (bfd_asymbol_name(symbol) == 0 || bfd_asymbol_name(symbol) == '\0')
        {
            continue;
        }

        symmap[bfd_asymbol_name(symbol)] = symbol;
    }
}

void
Image::loadSymtab()
{
    if(m_symTable == NULL)
    {
        m_symCount = bfd_get_symtab_upper_bound( m_bfdHandle );
        if(m_symCount <= 0)
        {
            return;
        }

        m_symTable = (asymbol **) malloc( m_symCount );
        m_symCount = bfd_canonicalize_symtab( m_bfdHandle, m_symTable );

        if ( m_symCount < 0 )
        {
            printf( "canonicalize symtab failed %ld\n", m_symCount );
        }        
        else
        {
            loadSymbolCache(m_symTable, m_symCount, m_symFuncCache);
        }
    }
}

void
Image::loadDyntab()
{
    if(m_dynTable == NULL)
    {
        m_dynCount = bfd_get_dynamic_symtab_upper_bound(m_bfdHandle);
        if(m_dynCount <= 0)
        {
            return;
        }

        m_dynTable = (asymbol **) malloc(m_dynCount);
        m_dynCount = bfd_canonicalize_dynamic_symtab(m_bfdHandle, m_dynTable);

        if(m_dynCount < 0)
        {
            printf( "canonicalize dynamic symtab failed %ld \n", m_dynCount);
        }
        else
        {
            loadSymbolCache(m_dynTable, m_dynCount, m_dynFuncCache);
        }
    }
}

void
Image::dumpSymtab()
{
    if ( m_symTable == 0 )
    {
        loadSymtab();
    }

    for ( long it = 0; it < m_symCount; it++ ) {

        asymbol* symbol = m_symTable[ it ];

        if (bfd_asymbol_name(symbol) == 0 || *bfd_asymbol_name(symbol) == '\0' )
        {
            continue;
        }

        printf( "symtab " );
        bfd_print_symbol_vandf( m_bfdHandle, stdout, symbol );

        char * symbol_name = demangle(bfd_asymbol_name(symbol));
        if (symbol_name)
        {
            printf(" section: %s, symbol: %s\n", bfd_get_section_name(0, bfd_get_section(symbol)), symbol_name);
        }
        free(symbol_name);
    }
}

void
Image::dumpDyntab()
{
    if ( m_dynTable == 0 )
    {
        loadDyntab();
    }

    for ( long it = 0; it < m_dynCount; it++ ) {

        asymbol* symbol = m_dynTable[ it ];

        if (bfd_asymbol_name(symbol) == 0 || *bfd_asymbol_name(symbol) == '\0' )
        {
            continue;
        }

        printf( "dyntab " );
        bfd_print_symbol_vandf( m_bfdHandle, stdout, symbol );

        char * symbol_name = demangle(bfd_asymbol_name(symbol));
        if (symbol_name)
        {
            printf(" section: %s, symbol: %s\n", bfd_get_section_name(0, bfd_get_section(symbol)), symbol_name);
        }
        free(symbol_name);
    }
}

bool
Image::isContained( Location& location, uintptr_t loadOffset )
{
    const bfd_vma& address( location.getAddress() );

    return ( ( address - loadOffset ) >= m_startAddress ) &&
        ( ( address - loadOffset ) < m_endAddress );
}

void 
Image::mapLocationFromCaches(Location& location, LocationCache::iterator & it, uintptr_t address)
{
    /* calling bfd_find_nearest_line is extremely expensive, so we cache our 
     * results in mappedLocations.  Check whether we've already mapped this
     * address and if so, grab the debug information out of the cache
     */
    it = m_mappedLocations.lower_bound(address);
    if(it != m_mappedLocations.end() && (m_mappedLocations.key_comp()(address, it->first) == 0))
    {
        location.isMapped(it->second.m_isMapped);
        location.m_filename = it->second.m_filename;
        location.m_functionname = it->second.m_functionname;
        location.m_linenumber = it->second.m_linenumber;
    }
}

void
Image::mapLocation( Location& location, uintptr_t loadOffset )
{
    uintptr_t address = location.getAddress() - loadOffset - m_startAddress;

    loadSymtab();

    
    LocationCache::iterator it;
    mapLocationFromCaches(location, it, address);

    if(location.m_isMapped)
        return;

    location.isMapped( false );

    if ( m_symCount > 0 )
    {
        location.isMapped( bfd_find_nearest_line( m_bfdHandle, m_textSection, m_symTable,
            address, &location.m_filename, &location.m_functionname, &location.m_linenumber ) != 0 );
    }

    
    loadDyntab();

    if(!location.m_isMapped)
    {
        loadDyntab();

        if(m_dynCount > 0)
        {
            location.isMapped(bfd_find_nearest_line(m_bfdHandle, m_textSection, m_dynTable,
                address, &location.m_filename, &location.m_functionname,
                &location.m_linenumber) != 0);
        }
    }

    /* put the location in our cache so we can use it for subsequent lookups of this address */
    m_mappedLocations.insert(it, LocationCache::value_type(address, location));
}

void
Image::findFunctionSymbol(Location& location, const FuncSymbolCache & cache)
{
    FuncSymbolCache::const_iterator it = cache.find(location.m_functionname);

    if(it != cache.end())
    {
        asymbol* symbol = it->second;
        Location testLocation = location;
        testLocation.m_address = bfd_asymbol_value(symbol);
        mapLocation(testLocation);
        if(testLocation.m_isMapped)
        {
            if((testLocation.m_filename != 0 && location.m_filename) && (strcmp(testLocation.m_filename, location.m_filename) == 0))
            {
                location = testLocation;
            }
        }
    }
}

void
Image::functionStart( Location& location )
{
    findFunctionSymbol(location, m_symFuncCache);
    if(!location.m_isMapped)
    {
        findFunctionSymbol(location, m_dynFuncCache);
    }
}

void
Image::mapFunctionStart( FunctionLocation& functionLocation )
{
    if ( !functionLocation.m_isMapped )
    {
        return;
    }

    Image* image = functionLocation.m_isKernel ? kernelImage : imageMap[ functionLocation.m_modulename ];

    if ( image == 0 )
    {
        return;
    }

    functionLocation.m_isMapped = false;
    image -> functionStart( functionLocation );

    if ( !functionLocation.m_isMapped )
    {
        if(functionLocation.m_lineLocationList.empty())
            functionLocation.m_linenumber = -1;
        else
            functionLocation.m_linenumber = *functionLocation.m_lineLocationList.begin();
    }
}

void 
Image::offlineProfiling()
{
    /* set this to true to prevent us from loading whatever modules are
     * loaded on this system.
     */
    loadedKldModules = true;
}


bool
Image::findKldModule(const char * kldName, std::string & kldPath)
{
    std::vector<std::string>::const_iterator it = MODULE_PATH.begin();

    for(; it != MODULE_PATH.end(); ++it) {
        std::string path = *it + '/' + kldName;
        int fd = open(path.c_str(), O_RDONLY);

        if(fd >= 0) {
            close(fd);
            kldPath = path;
            return true;
        }
    }

    return false;
}

void 
Image::loadKldImage(uintptr_t loadAddress, const char * moduleName)
{
    std::string kldPath;

    if(findKldModule(moduleName, kldPath))
    {
        kernelLoadableImageMap[bfd_vma(loadAddress)] = kldPath;
    }
    else
    {
        fprintf(stderr, "%s: unable to find kld module %s\n", 
            g_quitOnError ? "error" : "warning", moduleName);
        if (g_quitOnError)
        {
            exit(5);
        }
    }
}

std::string
Image::getLoadableImageName( const Location& location, uintptr_t& loadOffset )
{
    if (!loadedKldModules)
    {
        int fileid = 0;
        for ( fileid = kldnext( fileid ); fileid > 0; fileid = kldnext( fileid ) )
        {
            kld_file_stat stat;
            stat.version = sizeof( kld_file_stat );
            if ( kldstat( fileid, &stat ) < 0 )
            {
                printf( "can't stat module\n" );
            }
            else
            {
                std::string path;
                bool found = findKldModule(stat.name, path);

                if(found)
                    kernelLoadableImageMap[bfd_vma(stat.address)] = path;
                else
                {
                    fprintf(stderr, "%s: unable to find kld module %s\n", 
                        g_quitOnError ? "error" : "warning", stat.name);
                    if (g_quitOnError)
                    {
                        exit(5);
                    }
                }
            }
        }

        loadedKldModules = true;
    }

    loadOffset = 0;

    LoadableImageMap::iterator it = kernelLoadableImageMap.lower_bound(location.getAddress());
   
    if(it == kernelLoadableImageMap.begin())
        return "";

    --it;
    loadOffset = it->first;
    return it->second;
}

void
Image::mapAllLocations( LocationList& locationList )
{
    for ( LocationList::iterator it = locationList.begin(); it != locationList.end(); ++it )
    {
        std::vector<Location> & stack(*it);
        for(std::vector<Location>::iterator jt = stack.begin(); jt != stack.end(); ++jt)
        {
            Location& location(*jt);
        
            if(location.m_isKernel)
            {
                if(getKernel().isContained(location))
                {
                    getKernel().mapLocation(location);
                    if(location.m_isMapped)
                    {
                        location.m_modulename = KERNEL_NAME.c_str();
                    }
                }
                else
                {
                    uintptr_t loadOffset = 0;
                    const std::string& loadableImageName(Image::getLoadableImageName(location, loadOffset));

                    Image* kernelModuleImage = imageMap[loadableImageName];

                    if(kernelModuleImage == 0)
                    {
                        kernelModuleImage = new Image(loadableImageName);
                        imageMap[loadableImageName] = kernelModuleImage;
                    }

                    if(!kernelModuleImage->isOk())
                    {
                        continue;
                    }

                    if(kernelModuleImage->isContained(location, loadOffset))
                    {
                        kernelModuleImage->mapLocation(location, loadOffset);
                        if(location.m_isMapped)
                        {
                            location.m_modulename = loadableImageName.c_str();
                        }
                    }
                }
            }
            else
            {
                Process* process = Process::getProcess(location.m_pid);

                if(process == 0)
                {
                    continue;
                }

                const std::string& processName(process->getName());

                Image* processImage = imageMap[processName];

                if(processImage == 0)
                {
                    processImage = imageMap[processName] = new Image(processName);
                }

                if(!processImage->isOk())
                {
                    continue;
                }

                if(processImage->isContained(location))
                {
                    processImage->mapLocation(location);
                    if(location.m_isMapped)
                    {
                        location.m_modulename = processName.c_str();
                    }
                }
                else
                {
                    uintptr_t loadOffset = 0;
                    const std::string& loadableImageName(
                        process->getLoadableImageName(location, loadOffset));

                    Image* processLoadableImage = imageMap[loadableImageName];

                    if(processLoadableImage == 0)
                    {
                        processLoadableImage = imageMap[loadableImageName] = new Image(loadableImageName);
                    }

                    if(!processLoadableImage->isOk())
                    {
                        continue;
                    }

                    if(processLoadableImage->isContained(location, loadOffset))
                    {
                        processLoadableImage->mapLocation(location, loadOffset);
                        if(location.m_isMapped)
                        {
                            location.m_modulename = loadableImageName.c_str();
                        }
                    }
                }
            }
        }
    }
}

char*
Image::demangle(const char* name)
{
    if ( name == 0 )
    {
        return 0;
    }

    char* result = cplus_demangle(name, DMGL_ANSI | DMGL_PARAMS);

    return result ? result : strdup(name);
}
