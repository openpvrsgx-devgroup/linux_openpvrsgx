#!/bin/bash
libtoolize --force
aclocal
automake --add-missing
autoconf
