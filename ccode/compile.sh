#!/usr/bin/env bash

echo "compiling. . ."
#/c/MinGW/bin/gcc -c spiralsurf.c math_*.c  -funsigned-char

echo "linking libspiralsurf.so. . ."
gcc spiralsurf.c -c
gcc -shared -o libspiralsurf.so *.o

echo "compiling test binary. . ."
gcc -DDO_TEST spiralsurf.c -c
gcc *.o -o tst.exe

