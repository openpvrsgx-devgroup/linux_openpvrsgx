all: abe_dump

abe_dump: abe_dump.c
	@echo "Cross-compiling abe_dump"
	arm-none-linux-gnueabi-gcc abe_dump.c -o abe_dump --static

clean:
	rm -vf abe_dump
