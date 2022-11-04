CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)gcc

DEPDIR := .deps

CPPFLAGS := -D_GNU_SOURCE
CFLAGS :=
LDFLAGS := -lc
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

TARGET := pyra_vol_mon
SRCS := pyra_vol_mon.c iio_utils.c config.c iio_event.c main.c
OBJS := $(SRCS:.c=.o)
DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

%.o: %.c
%.o: %.c $(DEPDIR)/%.d Makefile | $(DEPDIR)
	$(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(DEPDIR): ; @mkdir -p $@

$(DEPFILES):

.PHONY: clean
clean:
	rm -rf $(TARGET) $(OBJS) $(DEPDIR)

include $(wildcard $(DEPFILES))
