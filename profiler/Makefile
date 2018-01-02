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

LDADD=	-lpmc -ldwarf -lelf
#-Wl,-rpath=/usr/local/lib/gcc8
CFLAGS += -I/usr/local/include
CXXFLAGS += -std=c++17

.include <bsd.prog.mk>

CWARNFLAGS.gcc += -Wno-expansion-to-defined -Wno-extra -Wno-unused-but-set-variable
CWARNFLAGS.clang += -Wno-user-defined-literals
