gcc bsp.c -Wall -Winline -O2 -ffast-math -DGCC -DMSDOS -lc -lm -o bsp
strip bsp
copy /y /b go32.exe+bsp bsp.exe
del bsp