gcc bsp.c -Wall -Winline -O2 -finline-functions -ffast-math -DGCC -DMSDOS -lm -o bsp
strip bsp
copy /y /b go32.exe+bsp bsp.exe
del bsp

:gcc bsp.c -Wall -g -fno-inline-functions -ffast-math -DGCC -DMSDOS -lm -o bsp
:copy /y /b go32.exe+bsp bsp.exe
:go32 -d \lang\gcc\bin\gdb bsp mans.wad

