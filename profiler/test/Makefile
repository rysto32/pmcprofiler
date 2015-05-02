PROG_CXX=profiler
WARNS=0
NO_MAN=

SRCS=	\
	DwarfCompileUnit.cpp \
	DwarfLocation.cpp \
	DwarfLookup.cpp \
	DwarfRange.cpp \
	main.cpp \

LDADD = -ldwarf -lelf
DEBUG_FLAGS=-g -O0
WARNS=3

.include <bsd.prog.mk>

