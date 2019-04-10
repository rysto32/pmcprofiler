
#PROG:=	bin/addr2line
PROG_LIBS := \
	addrline \
	imagefactory \
	image \
	dwarf \
	type \
	abi \
	frame \
	sharedptr \

PROG_STDLIBS := \
	elf \
	dwarf \
	LLVM \

PROG_LDFLAGS := \
	-L/usr/local/llvm-devel/lib \

LIB:=	addrline

SRCS:=	\
	main.cpp \

