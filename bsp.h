/*- BSP.H ------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <graphics.h>
#include <alloc.h>
#include <dos.h>

/*- boolean constants ------------------------------------------------------*/

typedef int Bool;

#define TRUE			1
#define FALSE			0

/*- The interfile global variables -----------------------------------------*/

/* from gfx.c */
extern int Scale;			/* scale to draw map 20 to 1 */
extern int OrigX;			/* the X origin */
extern int OrigY;			/* the Y origin */
extern int PointerX;		/* X position of pointer */
extern int PointerY;		/* Y position of pointer */
extern int Grid;			/* Grid snap amount */

/*- convert pointer coordinates to map coordinates -------------------------*/

#define MAPX(x)			(OrigX + (x - 319) * Scale)
#define MAPY(y)			(OrigY + (239 - y) * Scale)

#define SCRX(x)			((x - OrigX) / Scale + 319)
#define SCRY(y)			((OrigY - y) / Scale + 230)

#define GRIDX(x)			( (x + (Grid/2)) & (0xffff - (Grid-1)) )
#define GRIDY(y)			( (y + (Grid/2)) & (0xffff - (Grid-1)) )

/*- The function prototypes ------------------------------------------------*/

int main( int, char *[]);												/* from bsp.c */

void ProgError( char *, ...);											/* from funcs.c */
void *GetMemory( size_t);
void *ResizeMemory( void *, size_t);
void huge *GetFarMemory( unsigned long size);
void huge *ResizeFarMemory( void huge *old, unsigned long size);

/* from gfx.c */
void InitGfx( void);
void TermGfx( void);
void ClearScreen( void);
unsigned int ComputeAngle( int, int);

/*------------------------------- end of file ------------------------------*/
