
SUBDIRS=\
	profiler \

.DEFAULT_GOAL:=programs

TOPDIR:=.
CURDIR:=
STACK=x
OUTDIR:=$(TOPDIR)/obj
OBJDIR:=$(OUTDIR)/objects
LIBDIR=$(OUTDIR)/lib
INSTALLDIR=$(OUTDIR)/staging/

include make/Subdirs.mk
