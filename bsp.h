/*- BSP.H ------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <graphics.h>
#include <dos.h>
#include <math.h>

/*- boolean constants ------------------------------------------------------*/

#define TRUE			1
#define FALSE			0

/*- The function prototypes ------------------------------------------------*/

int main( int, char *[]);												/* from bsp.c */

void ProgError( char *, ...);											/* from funcs.c */
void *GetMemory( size_t);
void *ResizeMemory( void *, size_t);

unsigned int ComputeAngle( int, int);

#define max(a,b) (((a)>(b))?(a):(b))

/*------------------------------- end of file ------------------------------*/
