#!/bin/sh

gcc -c *.c $(curl-config --cflags)
gcc -o out *.o $(curl-config --libs)
rm *.o

