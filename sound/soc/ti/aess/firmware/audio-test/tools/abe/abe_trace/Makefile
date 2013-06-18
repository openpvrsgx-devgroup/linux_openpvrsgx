gen: abe_trace.c abe_trace_read.c
	gcc abe_trace.c -o abe_trace -lm
	${CROSS_COMPILE}gcc abe_trace_read.c -o abe_trace_read -lpthread

check:	abe_trace
	./abe_trace

all:	abe_trace trace

