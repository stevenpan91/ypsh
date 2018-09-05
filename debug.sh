#!/bin/sh
gcc -Wall ypsh.c -o ypshstart -g && valgrind --leak-check=full -v ./ypshstart