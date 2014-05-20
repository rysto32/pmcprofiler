

#include <ProfilePrinter.h>
#include <paths.h>
#include <libgen.h>
#include <cassert>

void 
ProfilePrinter::printLineNumbers(const Profiler & profiler, const LineLocationList& lineLocationList)
{
    if (profiler.showLines())
    {
        fprintf(m_outfile, " lines:");
        for (LineLocationList::const_iterator it = lineLocationList.begin();
            it != lineLocationList.end(); ++it)
        {
            fprintf(m_outfile, " %u", *it);
        }
    }
}

void 
FlatProfilePrinter::printProfile(const Profiler & profiler,
    const Process::ActiveProcessList & activeProcessList)
{
    LocationList locationList;
    Process::collectAllLocations(locationList);
    Image::mapAllLocations(locationList);
    Process::mapAllFunctions(locationList, LeafProcessStrategy());

    fprintf(m_outfile, "Events processed: %u\n\n", profiler.getSampleCount());

    unsigned cumulative = 0;
    for (LocationList::const_iterator it = locationList.begin(); it != locationList.end(); ++it)
    {
        const char* execPath = "";
        const char* base = "";
        const Location& location(it->front());
        char * functionName = Image::demangle(location.getFunctionName());

        if (location.isKernelImage())
        {
            execPath = getbootfile();
            base = basename(execPath);
        }
        else
        {
            Process* process = Process::getProcess(location.getPid());
            execPath = process == 0 ? "" : (process -> getName()).c_str();
            base = basename(execPath);
        }

        cumulative += location.getCount();
        fprintf(m_outfile, "%6.2f%% %6.2f%% %s, %6u, %10s, %6u, 0x%08lx, %s, %s, %s:%u %s\n",
            (location.getCount() * 100.0) / profiler.getSampleCount(),
            (cumulative * 100.0) / profiler.getSampleCount(),
            location.isKernelImage() ? "kern" : "user",
            location.getPid(),
            ((base == 0) || (*base == '\0')) ? execPath : basename(execPath),
            location.getCount(),
            location.getAddress(),
            location.isMapped() ? "mapped  " : "unmapped",
            execPath,
            *location.getFileName() == '\0' ? location.getModuleName() : location.getFileName(),
            location.getLineNumber(),
            functionName ? functionName : "<unknown>");
        free(functionName);
    }

    for (Process::ActiveProcessList::const_iterator processListIterator = activeProcessList.begin();
        processListIterator != activeProcessList.end(); ++processListIterator)
    {
        Process& process(**processListIterator);
        fprintf(m_outfile, "\nProcess: %6u, %s, total: %u (%6.2f%%)\n", process.getPid(), process.getName().c_str(),
            process.getSampleCount(), (process.getSampleCount() * 100.0) / profiler.getSampleCount());
        FunctionList functionList;
        process.getFunctionList(functionList);
        unsigned cumulativeCount = 0;
        fprintf(m_outfile, "       time   time-t   samples   env  file / library, line number, function\n");
        for (FunctionList::iterator functionListIterator = functionList.begin(); functionListIterator != functionList.end(); ++functionListIterator)
        {
            FunctionLocation& functionLocation(*functionListIterator);
            char * functionName = Image::demangle(functionLocation.getFunctionName());
            cumulativeCount += functionLocation.getCount();
            fprintf(m_outfile, "    %6.2f%%, %6.2f%%, %8u, %s, %s:%u, %s",
                    (functionLocation.getCount() * 100.0) / process.getSampleCount(),
                    (cumulativeCount * 100.0) / process.getSampleCount(),
                    functionLocation.getCount(),
                    functionLocation.isKernelImage() ? "kern" : "user",
                    *( functionLocation.getFileName() ) == '\0' ? functionLocation.getModuleName() : functionLocation.getFileName(),
                    functionLocation.getLineNumber(),
                    functionName ? functionName : "<unknown>");
            printLineNumbers(profiler, functionLocation.getLineLocationList());
            fprintf(m_outfile, "\n");
            free(functionName);
        }
    }   
}

