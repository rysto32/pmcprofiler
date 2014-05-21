PROG_CXX=pmcprofiler
WARNS=0
CFLAGS=-I/usr/local/include -DPACKAGE=${name} -DPACKAGE_VERSION=${version}
NO_MAN=

SRCS=	\
	EventFactory.cpp \
	Image.cpp \
	main.cpp \
	Profiler.cpp \
	Process.cpp \
	ProfilePrinter.cpp \
	sharedLib.cpp \

LDADD=	-lbfd -liberty -lintl -lz -lpmc -lkvm -L/usr/local/lib

.include <bsd.prog.mk>
