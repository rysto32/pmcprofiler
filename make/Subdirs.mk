
define include_template
 STACK := $$(STACK).x
 DIRSTACK_$$(STACK) := $$(CURDIR)
 CURDIR := $$(CURDIR)/$(1)

 OBJS:=
 LIB:=
 LOCAL_INCLUDE:=

 PROG_TARGET:=
 PROG:=
 PROG_LIBS:=
 PROG_STDLIBS:=

 TESTS :=

 SUBDIRS:=

 include $$(TOPDIR)/make/Defaults.mk
 include $$(CURDIR)/Rules.mk
 include $$(TOPDIR)/make/Library.mk
 include $$(TOPDIR)/make/Program.mk
 include $$(TOPDIR)/make/Test.mk

 include $$(TOPDIR)/make/Subdirs.mk
 CURDIR := $$(DIRSTACK_$$(STACK))
 STACK := $$(basename $$(STACK))
endef

SUBDIRS_$(STACK) := $(SUBDIRS)
$(foreach subdir_$(STACK),$(SUBDIRS_$(STACK)),$(eval $(call include_template,$(subdir_$(STACK)))))
