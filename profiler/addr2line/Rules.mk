
PROG:=	bin/addr2line
PROG_LIBS := \
	addrline \
	imagefactory \
	image \
	dwarf \
	abi \
	frame \
	sharedptr \

PROG_STDLIBS := \
	elf \
	dwarf \

LIB:=	addrline

SRCS:=	\
	main.cpp \

