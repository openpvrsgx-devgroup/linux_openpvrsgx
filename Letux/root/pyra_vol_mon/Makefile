CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)gcc

OBJDIR := .objs
DEPDIR := .deps

CPPFLAGS := -D_GNU_SOURCE
CFLAGS :=
LDFLAGS := -lc
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

TARGET := pyra_vol_mon
SRCS := pyra_vol_mon.c iio_utils.c config.c iio_event.c main.c
OBJS := $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

$(OBJDIR)/%.o: %.c $(DEPDIR)/%.d Makefile | $(OBJDIR) $(DEPDIR)
	$(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(OBJDIR) $(DEPDIR): ; @mkdir -p $@

DEPFILES += $(DEPDIR)/test.d

$(OBJDIR)/test.o: tests/test.c $(DEPDIR)/test.d Makefile | $(OBJDIR) $(DEPDIR)
	$(CC) -MT $@ -MMD -MP -MF $(DEPDIR)/test.d $(CFLAGS) $(CPPFLAGS) -g -I. -c -o $@ $<

test.bin: $(addprefix $(OBJDIR)/,test.o pyra_vol_mon.o iio_event.o)
	$(LD) $(LDFLAGS) $^ -o $@

test: test.bin
	./$<

.PHONY: clean test
clean:
	rm -rf $(TARGET) $(OBJDIR) $(DEPDIR)

$(DEPFILES):

include $(wildcard $(DEPFILES))
