TEST_OBJDIR := $(OBJDIR)/test
TEST_DEPDIR := $(DEPDIR)/test
TEST_DEPFLAGS = -MT $@ -MMD -MP -MF $(TEST_DEPDIR)/$*.d

$(TEST_OBJDIR) $(TEST_DEPDIR): ; @mkdir -p $@

$(TEST_OBJDIR)/%.o: %.c $(TEST_DEPDIR)/%.d Makefile tests/rules.mk  | $(TEST_OBJDIR) $(TEST_DEPDIR)
	$(CC) -g $(TEST_DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
$(TEST_OBJDIR)/%.o: tests/%.c $(TEST_DEPDIR)/%.d Makefile tests/rules.mk | $(TEST_OBJDIR) $(TEST_DEPDIR)
	$(CC) -g $(TEST_DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

define CREATE_TEST_INTERNAL
  TEST_TARGET_$(1) := test_$(1)
  TEST_SRCS_$(1) := $(2)
  TEST_OBJS_$(1) := $$(addprefix $(TEST_OBJDIR)/,$$(patsubst %.c,%.o,$$(TEST_SRCS_$(1))))
  DEPFILES += $$(patsubst %.c,$(TEST_DEPDIR)/%.d,$$(TEST_SRCS_$(1)))

  $$(TEST_TARGET_$(1)): $$(TEST_OBJS_$(1))
	$(LD) $(LDFLAGS) $$^ -o $$@

  .PHONY: test_run_$(1)
  test_run_$(1): $$(TEST_TARGET_$(1))
	@./$$<

  TEST_TARGETS += test_run_$(1)
endef

define CREATE_TEST
  $(eval $(call CREATE_TEST_INTERNAL,$(1),$(2)))
endef

$(call CREATE_TEST,pyra_vol_mon,test.c pyra_vol_mon.c iio_event.c)

.PHONY: test $(TEST_TARGETS)
test: $(TEST_TARGETS)
