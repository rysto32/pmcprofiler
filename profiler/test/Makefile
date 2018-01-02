PROG_CXX=profiler
WARNS=0
NO_MAN=

SRCS=	\
	Callframe.cpp \
	DwarfDie.cpp \
	DwarfDieRanges.cpp \
	DwarfDieStack.cpp \
	DwarfResolver.cpp \
	DwarfSearch.cpp \
	DwarfStackState.cpp \
	DwarfSubprogramInfo.cpp \
	DwarfUtil.cpp \
	Image.cpp \
	main.cpp \

DWARFINSTALLDIR=/home/rstone/src/dwarf-20170709/libdwarf

CXXFLAGS=-std=c++14 -DLOG_ENABLED
#-I$(DWARFINSTALLDIR) -DGNU_LIBDWARF
# -Wl,-L$(DWARFINSTALLDIR) -Wl,-rpath -Wl,$(DWARFINSTALLDIR)  $(DWARFINSTALLDIR)/libdwarf.so.1
LDADD =  -ldwarf -lelf
DEBUG_FLAGS=-g -O0
WARNS=3

.include <bsd.prog.mk>

