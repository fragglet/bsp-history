bsp: bsp.c makenode.c picknode.c bsp.h structs.h Makefile
	gcc -Wl,-s bsp.c -Wall -Winline -O2 -finline-functions -ffast-math -lm -o bsp

