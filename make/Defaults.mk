
CC=cc
CXX=c++
LD=c++

#CWARNFLAGS.gcc += -Wno-expansion-to-defined -Wno-extra -Wno-unused-but-set-variable

C_OPTIM=-O3 -fno-omit-frame-pointer

CXX_STD=-std=c++17
CXX_WARNFLAGS=-Wall -Werror -Wno-user-defined-literals

CFLAGS:=-I/usr/local/include -I$(TOPDIR)/include $(C_OPTIM) -g
CXXFLAGS:=$(CXX_STD) $(CXX_WARNFLAGS)
