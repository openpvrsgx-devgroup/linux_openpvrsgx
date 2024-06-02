gen: ../abe/abe_hal/abe_firmware.c header.c
	gcc header.c -o gen

fw:	gen
	./header

all:	gen fw

