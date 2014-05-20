TOPDIR ?= ../../../..

LOCAL_INCLUDE=-I. -I$(OPENSOURCE_INCLUDE)/gcc/$(GCC_VERSION)
LOCAL_CFLAGS=-Wall
LIB=profiler
OBJS=main.o Profiler.o EventFactory.o Image.o Process.o sharedLib.o ProfilePrinter.o

.PHONY: all profiler release
all: profiler

include $(TOPDIR)/make/macros
include $(TOPDIR)/make/library

OUTPUT_DIR=../$(BSPEC)/sandvine/bin
EXEC=$(OUTPUT_DIR)/profiler

# Not threaded
THREAD_LIBS=
QUICKEXIT=

 LOCAL_LIBS=-lbfd -liberty -lintl -lpmc -lkvm -lz

ifeq ($(BUILD_OPTS), BULLSEYE)
	LOCAL_LIBS+=-lpthread
endif

profiler: ${EXEC}

${EXEC}: lib 
	@echo `date  +%H:%M:%S` "Making $@"
	@mkdir -p `dirname $@`
	@${LD} $(BBJS) -o $(EXEC) ${LOCAL_LIBS} 

# appease make and give it a release target that does nothing
release:

