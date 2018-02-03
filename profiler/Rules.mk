
PROG:=	bin/pmcprofiler

PROG_LIBS := \
	pmcprofiler \
	aggfactory \
	spacefactory \
	samples \
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
	CallchainProfilePrinter.cpp \
	EventFactory.cpp \
	main.cpp \
	Profiler.cpp \
	ProfilePrinter.cpp \

SUBDIRS := \
	abi \
	addr2line \
	dwarf \
	frame \
	image \
	samples \
	sharedptr \

#DWARFINSTALLDIR=/home/rstone/src/dwarf-20170709/libdwarf

#.if defined(GNU_LIBDWARF)
#LDADD=-Wl,-rpath -Wl,$(DWARFINSTALLDIR) /home/rstone/src/dwarf-20170709/libdwarf/libdwarf.so
#CFLAGS=-I$(DWARFINSTALLDIR) -DGNU_LIBDWARF
#.else
#LDADD=-ldwarf
#.endif

# CWARNFLAGS.gcc += -Wno-expansion-to-defined -Wno-extra -Wno-unused-but-set-variable
