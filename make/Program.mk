
ifneq ($(PROG),)

PROG_TARGET := $(notdir $(PROG))_prog
$(PROG_TARGET)_PROG := $(PROG)
$(PROG_TARGET)_PROG_LIBS := $(addprefix $(LIBDIR)/lib,  $(addsuffix .a, $(PROG_LIBS)))
$(PROG_TARGET)_PROG_STDLIBS := $(addprefix -l,$(PROG_STDLIBS))
$(PROG_TARGET)_DEST := $(INSTALLDIR)/$($(PROG_TARGET)_PROG)

$(PROG_TARGET): $($(PROG_TARGET)_DEST)

$($(PROG_TARGET)_DEST): LDFLAGS := $(LDFLAGS)
$($(PROG_TARGET)_DEST): PROG_TARGET := $(PROG_TARGET)

$($(PROG_TARGET)_DEST): $($(PROG_TARGET)_PROG_LIBS)
	mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $^ $($(PROG_TARGET)_PROG_STDLIBS) -o $@


.PHONY: clean_$(PROG_TARGET)

clean:: clean_prog_$(PROG_TARGET)

clean_prog_$(PROG_TARGET):
	$(RM) $($(subst clean_prog_,,$@)_DEST)

.PHONY: programs
programs: $(PROG_TARGET)

endif