REM Compiled with DJGPP v1.11

gcc -g -DGCC -DMSDOS -c bsp.c
gcc -o bsp bsp.o -lsys -lm
aout2exe bsp
