#!/bin/bash

code="$PWD"
opts=-g
cd build > /dev/null
g++ $opts $code/build.bat -o build
cd $code > /dev/null
