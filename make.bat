@echo off
REM Compiled with DJGPP v1.11
setlocal
set time=%@TIME[%_time]
gcc -c bsp.c -ffast-math -finline-functions -O2 -DGCC -DMSDOS
gcc -o bsp bsp.o -lc -lm
aout2exe bsp
echo Compile took %@eval[%@TIME[%_time]-%time] seconds.
