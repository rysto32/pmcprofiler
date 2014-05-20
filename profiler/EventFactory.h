#if !defined(EVENTFACTORY_H)
#define EVENTFACTORY_H

#include <stdint.h>

class Profiler;

class EventFactory
{
public:
    static void createEvents(Profiler& profiler, uint32_t maxDepth);
};

#endif // #if !defined(EVENTFACTORY_H)
