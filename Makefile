
SUBDIRS=\
	profiler \

.DEFAULT_GOAL:=all

all: programs test

TOPDIR:=.
CURDIR:=.
STACK=x

OUTDIR:=$(TOPDIR)/obj
OBJDIR:=$(OUTDIR)/objects
LIBDIR:=$(OUTDIR)/lib
INSTALLDIR:=$(OUTDIR)/staging/
DEPENDDIR:=$(OUTDIR)/depend/
TESTOBJDIR:=$(OUTDIR)/test_obj
TESTDIR:=$(OUTDIR)/test

$(OUTDIR)/%/flag:
	mkdir -p $(dir $@)
	touch $@

include make/Subdirs.mk


.PHONY: test

test: $(TEST_PROGS) | programs
	@for test in $(TEST_PROGS); do \
		limits -c 0 ./$${test} || break; \
	done

.PHONY: nothing
nothing:
	@true
