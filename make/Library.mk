
ifneq ($(LIB),)

$(LIB)_SRCDIR := $(CURDIR)
$(LIB)_OBJDIR := $(OBJDIR)/$(CURDIR)
$(LIB)_DOBJS := $(addprefix $($(LIB)_OBJDIR), $(addsuffix .o,$(basename $(SRCS))))
$(LIB)_LIBRARY := $(LIBDIR)/lib$(LIB).a
$(LIB)_CLEAN := $($(LIB)_DOBJS) $($(LIB)_LIBRARY)

$($(LIB)_LIBRARY): $($(LIB)_DOBJS)
	mkdir -p $(dir $@)
	$(AR) -crs $@ $^

$($(LIB)_DOBJS): $($(LIB)_OBJDIR)/flag

$($(LIB)_OBJDIR)/flag:
	mkdir -p $@
	touch $($(LIB)_OBJDIR)/flag

$($(LIB)_DOBJS): LOCAL_INCLUDE := $(LOCAL_INCLUDE)

$(OBJDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $(LOCAL_INCLUDE) $< -o $@

$(OBJDIR)/%.o: %.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(LOCAL_INCLUDE) $< -o $@

.PHONY: clean_lib_$(LIB)

clean:: clean_lib_$(LIB)

clean_lib_$(LIB): LIB:=$(LIB)

clean_lib_$(LIB):
	$(RM) $($(LIB)_CLEAN)

endif