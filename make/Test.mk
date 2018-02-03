
ifneq ($(TESTS),)

define test_template

TEST:=$$(shell echo $1 | tr 'a-z' 'A-Z')
TEST_OBJS := $$(call src_to_obj,$$(TEST_$$(TEST)_SRCS))

TEST_$$(TEST)_OBJPATHS=
ifneq ($$(TEST_$$(TEST)_WRAPFUNCS),)

TEST_$$(TEST)_REDEFINE_SYMS:=$$(addprefix --redefine-syms=,$$(TEST_$$(TEST)_WRAPFUNCS))

TEST_$$(TEST)_OBJPATHS:=$$(addprefix $$(TESTOBJDIR)/$$(CURDIR)/,$$(addsuffix .test.o,$$(basename $$(TEST_OBJS))))


$$(TEST_$$(TEST)_OBJPATHS): $$(TESTOBJDIR)/$$(CURDIR)/flag

$$(TEST_$$(TEST)_OBJPATHS) $$(TESTOBJDIR)/%.test.o: $$(OBJDIR)/%.o
	objcopy $$(TEST_$$(TEST)_REDEFINE_SYMS) $> $@

clean:: clean_test_$$(TEST)))

.PHONY: clean_testobjs_$$(TEST)))
clean_testobjs_$$(TEST))):
	$(RM) $(TEST_$$(TEST)_OBJPATHS)

else

TEST_$$(TEST)_OBJPATHS:=$$(addprefix $$(LIB_OBJDIR)/,$$(TEST_OBJS))

endif

TEST_$$(TEST)_GTEST_OBJ := $$($$(LIB)_OBJDIR)/$1.gtest.o
TEST_$$(TEST)_OBJPATHS := $$(TEST_$$(TEST)_OBJPATHS) $$(TEST_$$(TEST)_GTEST_OBJ)


TEST_$$(TEST)_LIBARGS:=$$(addprefix -l,$$(TEST_$$(TEST)_LIBS)) -lgtest -lgtest_main -lpthread

TEST_$$(TEST)_OUTDIR := $$(TESTDIR)/$$(CURDIR)/
TEST_$$(TEST)_PROG := $$(TEST_$$(TEST)_OUTDIR)/$1.testprog

TEST_PROGS := $$(TEST_PROGS) $$(TEST_$$(TEST)_PROG)

$$(TEST_$$(TEST)_PROG): $$(TEST_$$(TEST)_OBJPATHS) $$(TEST_$$(TEST)_OUTDIR)/flag
	$${CXX} -Wl,-L/usr/local/lib $$(LDFLAGS) $$(TEST_$$(TEST)_OBJPATHS) \
	    $$(TEST_$$(TEST)_LIBARGS) -o $$@

.PHONY: test.$1

test.$1: $$(TEST_OUTDIR)/$1.testprog
	@./$$<


clean:: clean_test_$$(TEST)

.PHONY: clean_test_$$(TEST)

clean_test_$$(TEST):
	$(RM) $$(TEST_$$(TEST)_GTEST_OBJ) \
	    $$(call src_to_dep,$$(basename $$(TEST_$$(TEST)_GTEST_OBJ)).cpp) \
	    $$(TEST_$$(TEST)_PROG) $$(TEST_$$(TEST)_OUTDIR)/flag

endef

$(foreach test,$(TESTS),$(eval $(call test_template,$(test))))

endif
