PROG_CXX=pmcprofiler
WARNS=3
NO_MAN=

SRCS=	\
	DwarfCompileUnit.cpp \
	DwarfLocation.cpp \
	DwarfLookup.cpp \
	DwarfRange.cpp \
	EventFactory.cpp \
	Image.cpp \
	main.cpp \
	Profiler.cpp \
	Process.cpp \
	ProfilePrinter.cpp \

LDADD=	-lpmc -ldwarf -lelf
CFLAGS += -I/usr/local/include

.include <bsd.prog.mk>
