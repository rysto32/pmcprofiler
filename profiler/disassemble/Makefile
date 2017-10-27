PROG_CXX=disassemble
CXX=clang++-devel
WARNS=0
NO_MAN=

SRCS=	\
	disassemble.cpp \

CFLAGS += -I/usr/local/llvm-devel/include -I..
CXXFLAGS += -std=c++11 -L/usr/local/llvm-devel/lib
LDADD = -ldwarf -lelf -lLLVM
DEBUG_FLAGS=-g -O0
WARNS=3

.include <bsd.prog.mk>

