
PROG:=	bin/pmcprofiler

PROG_LIBS := \
	pmcprofiler \
	imagefactory \
	image \
	dwarf \
	abi \
	frame \
	sharedptr \

PROG_STDLIBS:= \
	pmc \
	elf \
	dwarf \

LIB:= pmcprofiler

SRCS=	\
	AddressSpace.cpp \
	CallchainProfilePrinter.cpp \
	DefaultAddressSpaceFactory.cpp \
	DefaultSampleAggregationFactory.cpp \
	EventFactory.cpp \
	main.cpp \
	Profiler.cpp \
	ProfilePrinter.cpp \
	SampleAggregation.cpp \

SUBDIRS := \
	abi \
	addr2line \
	dwarf \
	frame \
	image \
	sharedptr \

#DWARFINSTALLDIR=/home/rstone/src/dwarf-20170709/libdwarf

#.if defined(GNU_LIBDWARF)
#LDADD=-Wl,-rpath -Wl,$(DWARFINSTALLDIR) /home/rstone/src/dwarf-20170709/libdwarf/libdwarf.so
#CFLAGS=-I$(DWARFINSTALLDIR) -DGNU_LIBDWARF
#.else
#LDADD=-ldwarf
#.endif

# CWARNFLAGS.gcc += -Wno-expansion-to-defined -Wno-extra -Wno-unused-but-set-variable
