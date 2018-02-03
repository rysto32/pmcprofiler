
ifneq ($(TESTS),)

define test_template

TEST:=$$(shell echo $1 | tr 'a-z' 'A-Z')
TEST_OBJS := $$(notdir $$(call src_to_obj,$$(TEST_$$(TEST)_SRCS)))

TEST_$$(TEST)_OBJPATHS=
ifneq ($$(TEST_$$(TEST)_WRAPFUNCS),)

TEST_$$(TEST)_REDEFINE_SYMS:=$$(addprefix --redefine-sym ,$$(TEST_$$(TEST)_WRAPFUNCS))

TEST_$$(TEST)_OBJDIR := $$(TESTOBJDIR)/$$(CURDIR)

TEST_$$(TEST)_OBJPATHS:=$$(addprefix $$(TEST_$$(TEST)_OBJDIR)/,$$(addsuffix .test.o,$$(basename $$(TEST_OBJS))))

$$(TEST_$$(TEST)_OBJDIR)/%.test.o: $$($$(LIB)_OBJDIR)/%.o
	mkdir -p $$(dir $$@)
	objcopy $$(TEST_$$(TEST)_REDEFINE_SYMS) $$< $$@

clean:: clean_testobjs_$$(TEST)

.PHONY: clean_testobjs_$$(TEST)
clean_testobjs_$$(TEST):
	$(RM) $$(TEST_$$(TEST)_OBJPATHS)

else

TEST_$$(TEST)_OBJPATHS:=$$(addprefix $$(LIB_OBJDIR)/,$$(TEST_OBJS))

endif

TEST_GTEST_PREFIX := $$($$(LIB)_OBJDIR)/$1.gtest
TEST_$$(TEST)_GTEST_OBJ := $$(TEST_GTEST_PREFIX).o
TEST_$$(TEST)_GTEST_DEPFILE := $$(call src_to_dep,$$(TEST_GTEST_PREFIX).cpp)

$$(TEST_$$(TEST)_GTEST_OBJ): LIB := $(LIB)

-include $$(TEST_$$(TEST)_GTEST_DEPFILE)

TEST_$$(TEST)_OBJPATHS := $$(TEST_$$(TEST)_OBJPATHS) $$(TEST_$$(TEST)_GTEST_OBJ)

TEST_$$(TEST)_LIBARGS := $$(addprefix $$(LIBDIR)/lib,  $$(addsuffix .a, $$(TEST_$$(TEST)_LIBS)))

TEST_$$(TEST)_STDLIBARGS:= \
	$$(addprefix -l,$$(TEST_$$(TEST)_STDLIBS)) \
	-lgtest -lgtest_main -lpthread

TEST_$$(TEST)_OUTDIR := $$(TESTDIR)/$$(CURDIR)
TEST_$$(TEST)_PROG := $$(TEST_$$(TEST)_OUTDIR)/$1.testprog

TEST_PROGS := $$(TEST_PROGS) $$(TEST_$$(TEST)_PROG)

$$(TEST_$$(TEST)_PROG): TEST := $$(TEST)
$$(TEST_$$(TEST)_PROG): $$(TEST_$$(TEST)_OBJPATHS) $$(TEST_$$(TEST)_LIBARGS)
	mkdir -p $$(dir $$@)
	$${CXX} -Wl,-L/usr/local/lib $$(LDFLAGS) $$(TEST_$$(TEST)_OBJPATHS) \
	    $$(TEST_$$(TEST)_LIBARGS) $$(TEST_$$(TEST)_STDLIBARGS) -o $$@

.PHONY: test.$1

test.$1: $$(TEST_$$(TEST)_OUTDIR)/$1.testprog
	@./$$<

clean:: clean_test_$$(TEST)

.PHONY: clean_test_$$(TEST)
clean_test_$$(TEST): TEST := $$(TEST)

clean_test_$$(TEST):
	$(RM) $$(TEST_$$(TEST)_GTEST_OBJ) \
	    $$(TEST_$$(TEST)_GTEST_DEPFILE) \
	    $$(TEST_$$(TEST)_PROG)

endef

$(foreach test,$(TESTS),$(eval $(call test_template,$(test))))

endif
