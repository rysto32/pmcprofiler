PROG_CXX=pmcprofiler
WARNS=3
NO_MAN=

SRCS=	\
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

.include <bsd.prog.mk>
