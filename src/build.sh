#!/bin/sh
rm configure.ac
autoscan
cp configure.in.bak configure.ac
aclocal
autoconf
autoheader
automake --add-missing
./configure CXXFLAGS= CFLAGS=
make
