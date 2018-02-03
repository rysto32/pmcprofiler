
PROG:=bin/pmcprofiler
PROG_LIBS:=pmcprofiler
PROG_STDLIBS:= \
	pmc \
	elf \
	dwarf \

LIB:= pmcprofiler

SRCS=	\
	AddressSpace.cpp \
	Callchain.cpp \
	CallchainProfilePrinter.cpp \
	Callframe.cpp \
	DefaultAddressSpaceFactory.cpp \
	DefaultImageFactory.cpp \
	DefaultSampleAggregationFactory.cpp \
	Demangle.cpp \
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
	ImageFactory.cpp \
	main.cpp \
	Profiler.cpp \
	ProfilePrinter.cpp \
	SampleAggregation.cpp \
	SharedString.cpp \

#DWARFINSTALLDIR=/home/rstone/src/dwarf-20170709/libdwarf

#.if defined(GNU_LIBDWARF)
#LDADD=-Wl,-rpath -Wl,$(DWARFINSTALLDIR) /home/rstone/src/dwarf-20170709/libdwarf/libdwarf.so
#CFLAGS=-I$(DWARFINSTALLDIR) -DGNU_LIBDWARF
#.else
#LDADD=-ldwarf
#.endif

# CWARNFLAGS.gcc += -Wno-expansion-to-defined -Wno-extra -Wno-unused-but-set-variable
