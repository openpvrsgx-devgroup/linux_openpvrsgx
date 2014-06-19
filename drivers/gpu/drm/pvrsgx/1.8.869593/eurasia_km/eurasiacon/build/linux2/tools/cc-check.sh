#!/bin/sh

LANG=C
export LANG

usage() {
	echo "usage: $0 [--64] --cc CC --out OUT [cflag]"
	exit 1
}

do_cc() {
	echo "int main(void){return 0;}" | $CC $1 -xc -c - -o $ccof 2>/dev/null
}

while [ 1 ]; do
	if [ "$1" = "--64" ]; then
		BIT_CHECK=1
	elif [ "$1" = "--cc" ]; then
		[ "x$2" = "x" ] && usage
		CC="$2" && shift
	elif [ "$1" = "--out" ]; then
		[ "x$2" = "x" ] && usage
		OUT="$2" && shift
	elif [ "${1#--}" != "$1" ]; then
		usage
	else
		break
	fi
	shift
done

[ "x$CC" = "x" ] && usage
[ "x$OUT" = "x" ] && usage
ccof=$OUT/cc-sanity-check

if [ "x$BIT_CHECK" = "x1" ]; then
	do_cc ""
	file $ccof | grep -q 64-bit
	[ "$?" = "0" ] && echo true || echo false
else
	[ "x$1" = "x" ] && usage
	do_cc $1
	[ "$?" = "0" ] && echo $1
fi

rm -f $ccof
exit 0
