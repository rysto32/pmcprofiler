PROG_CXX=profiler
WARNS=0
NO_MAN=

SRCS=	\
	Callframe.cpp \
	DwarfCompileUnit.cpp \
	DwarfCompileUnitDie.cpp \
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

.if defined(GNU_LIBDWARF)
LDADD=-Wl,-rpath -Wl,$(DWARFINSTALLDIR) /home/rstone/src/dwarf-20170709/libdwarf/libdwarf.so
CXXFLAGS=-I$(DWARFINSTALLDIR) -DGNU_LIBDWARF
.else
LDADD=-ldwarf
.endif

CXXFLAGS+=-std=c++17 -DLOG_ENABLED
LDADD+=-lelf
DEBUG_FLAGS=-g -O0
WARNS=3
CWARNFLAGS.clang += -Wno-user-defined-literals

.include <bsd.prog.mk>

