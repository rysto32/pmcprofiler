
ifneq ($(LIB),)

$(LIB)_SRCDIR := $(CURDIR)
$(LIB)_OBJDIR := $(OBJDIR)/$(CURDIR)
$(LIB)_DOBJS := $(addprefix $($(LIB)_OBJDIR), $(addsuffix .o,$(basename $(SRCS))))
$(LIB)_LIBRARY := $(LIBDIR)/lib$(LIB).a
DEPDIR := $(DEPENDDIR)/$(CURDIR)
DEPFILES := $(addprefix $(DEPDIR)/,$(addsuffix .d,$(SRCS)))
$(LIB)_CLEAN := $($(LIB)_DOBJS) $($(LIB)_LIBRARY) $(DEPFILES)

$($(LIB)_LIBRARY): $($(LIB)_DOBJS)
	mkdir -p $(dir $@)
	$(AR) -crs $@ $^

$($(LIB)_DOBJS): $($(LIB)_OBJDIR)/flag $(DEPDIR)/flag

$($(LIB)_OBJDIR)/flag $(DEPDIR)/flag:
	mkdir -p $(dir $@)
	touch $@

$($(LIB)_DOBJS): LOCAL_INCLUDE := $(LOCAL_INCLUDE)
$($(LIB)_DOBJS): DEPDIR := $(DEPDIR)

$(OBJDIR)/%.o: %.c
	$(CC) -MD -MF $(DEPDIR)/$(notdir $<.d) -c $(CFLAGS) $(LOCAL_INCLUDE) $< -o $@

$(OBJDIR)/%.o: %.cpp
	$(CXX) -MD -MF $(DEPDIR)/$(notdir $<.d) -c $(CFLAGS) $(CXXFLAGS) $(LOCAL_INCLUDE) $< -o $@

.PHONY: clean_lib_$(LIB)

clean:: clean_lib_$(LIB)

clean_lib_$(LIB): LIB:=$(LIB)

clean_lib_$(LIB):
	$(RM) $($(LIB)_CLEAN)

-include $(DEPFILES)

endif