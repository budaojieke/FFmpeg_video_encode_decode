#!/bin/sh

BASH=`pwd`
LIBPATH=${BASH}:${BASH}/lib/
export LD_LIBRARY_PATH=${LIBPATH}:${LD_LIBRARY_PATH}

