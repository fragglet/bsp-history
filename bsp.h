/*- BSP.H ------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>

#if defined(MSDOS) || defined(__MSDOS__)
#include <dos.h>
#endif
#ifdef __TURBOC__
#include <alloc.h>
#endif
#ifndef GCC
#define __INLINE__
#endif

/*- boolean constants ------------------------------------------------------*/

#define TRUE			1
#define FALSE			0

/*- The function prototypes ------------------------------------------------*/

int main( int, char *[]);												/* from bsp.c */

static void ProgError( char *, ...);											/* from funcs.c */
static __inline__ void *GetMemory( size_t);
static __inline__ void *ResizeMemory( void *, size_t);

static __inline__ unsigned int ComputeAngle( int, int);

#undef max
#define max(a,b) (((a)>(b))?(a):(b))

/*- print a resource name by printing first 8 characters --*/

#define Printname(dir) printf("%-8.8s",(dir)->name)

/*------------------------------- end of file ------------------------------*/
