bsp: bsp.c makenode.c picknode.c bsp.h structs.h Makefile
	gcc -Wl,-s bsp.c -Wall -O3 -lm -o bsp

