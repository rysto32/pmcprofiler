
ifneq ($(LIB),)

define src_to_obj
$(addprefix $($(LIB)_OBJDIR)/, $(addsuffix .o,$(basename $1)))
endef

define src_to_dep
$(addprefix $($(LIB)_DEPDIR)/,$(addsuffix .d,$(notdir $1)))
endef

$(LIB)_SRCDIR := $(CURDIR)
$(LIB)_OBJDIR := $(OBJDIR)/$(CURDIR)
$(LIB)_DOBJS := $(call src_to_obj,$(SRCS))
$(LIB)_LIBRARY := $(LIBDIR)/lib$(LIB).a
$(LIB)_DEPDIR := $(DEPENDDIR)/$(CURDIR)
DEPFILES := $(call src_to_dep,$(SRCS))
$(LIB)_CLEAN := $($(LIB)_DOBJS) $($(LIB)_LIBRARY) $(DEPFILES)

$($(LIB)_LIBRARY): $($(LIB)_DOBJS)
	mkdir -p $(dir $@)
	$(AR) -crs $@ $^

$($(LIB)_DOBJS): LOCAL_INCLUDE := $(LOCAL_INCLUDE)
$($(LIB)_DOBJS): LIB := $(LIB)

$(OBJDIR)/%.o: %.c
	mkdir -p $(dir $@) $($(LIB)_DEPDIR)
	$(CC) -MD -MF $(call src_to_dep,$<) -c $(CFLAGS) $(LOCAL_INCLUDE) $< -o $@

$(OBJDIR)/%.o: %.cpp
	mkdir -p $(dir $@) $($(LIB)_DEPDIR)
	$(CXX) -MD -MF $(call src_to_dep,$<) -c $(CFLAGS) $(CXXFLAGS) $(LOCAL_INCLUDE) $< -o $@

.PHONY: clean_lib_$(LIB)

clean:: clean_lib_$(LIB)

clean_lib_$(LIB): LIB:=$(LIB)

clean_lib_$(LIB):
	$(RM) $($(LIB)_CLEAN)

-include $(DEPFILES)

endif