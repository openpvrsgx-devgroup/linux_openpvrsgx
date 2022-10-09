CROSS_COMPILE ?= arm-linux-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)gcc

CPPFLAGS := -D_GNU_SOURCE
CFLAGS :=
LDFLAGS :=

TARGET := pyra_vol_mon
SRCS := pyra_vol.c iio_utils.c
OBJS := $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -rf $(TARGET) $(OBJS)
