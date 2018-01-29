PROG_CXX=pmcprofiler
WARNS=3
NO_MAN=

#CXX=g++8
#COMPILER_TYPE=gcc

SRCS=	\
	AddressSpace.cpp \
	Callchain.cpp \
	CallchainProfilePrinter.cpp \
	Callframe.cpp \
	DefaultAddressSpaceFactory.cpp \
	DefaultImageFactory.cpp \
	DefaultSampleAggregationFactory.cpp \
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
	EventFactory.cpp \
	Image.cpp \
	main.cpp \
	Profiler.cpp \
	ProfilePrinter.cpp \
	SampleAggregation.cpp

DWARFINSTALLDIR=/home/rstone/src/dwarf-20170709/libdwarf

.if defined(GNU_LIBDWARF)
LDADD=-Wl,-rpath -Wl,$(DWARFINSTALLDIR) /home/rstone/src/dwarf-20170709/libdwarf/libdwarf.so
CFLAGS=-I$(DWARFINSTALLDIR) -DGNU_LIBDWARF
.else
LDADD=-ldwarf
.endif

LDADD+=-lpmc -lelf
#-Wl,-rpath -Wl,$(DWARFINSTALLDIR) /home/rstone/src/dwarf-20170709/libdwarf/libdwarf.so
#-Wl,-rpath=/usr/local/lib/gcc8
CFLAGS += -I/usr/local/include
# -I$(DWARFINSTALLDIR) -DGNU_LIBDWARF
# -DLOG_ENABLED
CXXFLAGS += -std=c++17 -fno-omit-frame-pointer

.include <bsd.prog.mk>

CWARNFLAGS.gcc += -Wno-expansion-to-defined -Wno-extra -Wno-unused-but-set-variable
CWARNFLAGS.clang += -Wno-user-defined-literals
