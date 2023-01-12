AR = ar
CC = ${CROSS_COMPILE}gcc
CFLAGS =
DYLIB = so

# DYLIB = dylib
# CFLAGS += -DPC_SIMULATION

OBJECTS = abe_api.o \
	abe_dat.o \
	abe_dbg.o \
	abe_ext.o \
	abe_ini.o \
	abe_irq.o \
	abe_lib.o \
	abe_mem.o \
	abe_seq.o

all:	gen

gen:	libabe_hal.a libabe_hal.$(DYLIB)

libabe_hal.a: $(OBJECTS)
	$(AR) rcs $@ $^

libabe_hal.$(DYLIB): $(OBJECTS)
	$(CC) $(CFLAGS) -shared $^ -o $@ -lm

%.o : %.c
	$(CC) $(CFLAGS) $< -c

abe_firmware: \
	abe_firmware.o
	: firmware v9560 for #include into some main program
	: u32 fw[]={
	:   #include abe_firmware.c
	: }; abe_reload_fw_param(fw);

main: \
	abe_main.o

clean:
	rm -fv libabe_hal.so *.o
