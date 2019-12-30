#!/bin/bash
machine=`uname -m|grep '64'`
if [ a"$machine" == "a" ]; 
then
	bitlevel=32
else
	bitlevel=64
fi
echo $bitlevel
cd boost
./bootstrap.sh --prefix=./
./tools/build/src/engine/bjam link=static threading=multi variant=release address-model=$bitlevel toolset=gcc runtime-link=static
