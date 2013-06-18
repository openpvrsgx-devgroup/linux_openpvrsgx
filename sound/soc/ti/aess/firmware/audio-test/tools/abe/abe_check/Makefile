all:	gen check target

gen: abe_check.c
	gcc abe_check.c -o abe_check -lm

check:	abe_check
	./abe_check

target:	abe_check.c
	${CROSS_COMPILE}gcc -DARM abe_check.c -o abe_check_arm -lm

clean:
	rm -fv abe_check
