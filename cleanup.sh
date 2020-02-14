#!/bin/sh

test -f Makefile && make clean

# Use this to get rid of clutter you don't want in version control.
# "autoreconf -i" and "./configure" are required after executing this
# script.
rm -f stamp-h1 *.h configure config.log config.status ltmain.sh
rm -f *.m4
rm -rf autom4te.cache/
rm -f m4/*.m4
rm -f build/m4/*.m4
rm -r libtool
rm -f test-driver
rm -f build/ltmain.sh
rm -f build/test-driver

for dir in . lib mandelbrot examples julia1 buddhabrot1 bbrot2 mbrot2
do
    rm -f ${dir}/*.in
    rm -f ${dir}/Makefile
    rm -rf ${dir}/.deps
    rm -f ${dir}/*~
done

