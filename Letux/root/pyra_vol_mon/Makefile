CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)gcc

CPPFLAGS := -D_GNU_SOURCE
CFLAGS :=
LDFLAGS := -lc

TARGET := pyra_vol_mon
SRCS := pyra_vol_mon.c iio_utils.c config.c iio_event.c main.c
OBJS := $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

%.o: %.c Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -rf $(TARGET) $(OBJS)
