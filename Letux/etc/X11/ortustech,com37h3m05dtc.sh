#!/bin/bash

cd /root
NAME=blanviewd
[ -x $NAME ] || make $NAME
daemon --name=$NAME $PWD/$NAME
