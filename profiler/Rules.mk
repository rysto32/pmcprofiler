
PROG:=	bin/pmcprofiler

PROG_LIBS := \
	pmcprofiler \
	aggfactory \
	callchainFactory \
	printers \
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

SRCS :=	\
	EventFactory.cpp \
	main.cpp \
	Profiler.cpp \

SUBDIRS := \
	abi \
	addr2line \
	dwarf \
	frame \
	image \
	printers \
	samples \
	sharedptr \

TESTS := \
	EventFactory \

TEST_EVENTFACTORY_SRCS := \
	EventFactory.cpp \

TEST_EVENTFACTORY_WRAPFUNCS := \
	open=mock_open \
	close=mock_close \

TEST_EVENTFACTORY_LIBS := \
	sharedptr \

TEST_EVENTFACTORY_STDLIBS := \
	gmock \

#DWARFINSTALLDIR=/home/rstone/src/dwarf-20170709/libdwarf

#.if defined(GNU_LIBDWARF)
#LDADD=-Wl,-rpath -Wl,$(DWARFINSTALLDIR) /home/rstone/src/dwarf-20170709/libdwarf/libdwarf.so
#CFLAGS=-I$(DWARFINSTALLDIR) -DGNU_LIBDWARF
#.else
#LDADD=-ldwarf
#.endif

# CWARNFLAGS.gcc += -Wno-expansion-to-defined -Wno-extra -Wno-unused-but-set-variable
