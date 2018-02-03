
SUBDIRS=\
	profiler \

.DEFAULT_GOAL:=programs

TOPDIR:=.
CURDIR:=
STACK=x

OUTDIR:=$(TOPDIR)/obj
OBJDIR:=$(OUTDIR)/objects
LIBDIR:=$(OUTDIR)/lib
INSTALLDIR:=$(OUTDIR)/staging/
DEPENDDIR:=$(OUTDIR)/depend/

include make/Subdirs.mk
