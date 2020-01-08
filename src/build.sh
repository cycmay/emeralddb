#!/bin/sh
rm configure.in
autoscan
cp configure.in.bak configure.ac
aclocal
autoconf
autoheader
automake --add-missing
./configure CXXFLAGS= CFLAGS=
make
