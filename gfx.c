/*- GFX.C ------------------------------------------------------------------*/

#include "bsp.h"															/* the includes */
#include <math.h>

/*--------------------------------------------------------------------------*/

/* the global variables */
int GfxMode = 0;			/* graphics mode number, or 0 for text */
int OrigX = 0;				/* the X origin */
int OrigY = 0;				/* the Y origin */
int Scale = 4;				/* the scale value */
int PointerX = 320;		/* X position of pointer */
int PointerY = 240;		/* Y position of pointer */
int Grid = 128;

/*- initialise the graphics display ----------------------------------------*/

void InitGfx()
{
   int gdriver = VGA, gmode = VGAHI, errorcode;
   initgraph( &gdriver, &gmode, NULL);
   errorcode = graphresult();
   if (errorcode != grOk)
      ProgError( "graphics error: %s", grapherrormsg( errorcode));
   setlinestyle( 0, 0, 1);
   setbkcolor( BLACK);
   settextstyle( 0, 0, 1);
   GfxMode = 2; /* 0=text, 1=320x200, 2=640x480, positive=16colors, negative=256colors */
}

/*- terminate the graphics display -----------------------------------------*/

void TermGfx()
{
   if (GfxMode)
   {
      closegraph();
      GfxMode = 0;
   }
}

/*- clear the screen -------------------------------------------------------*/

void ClearScreen()
{
   cleardevice();
}

/*- write text to the screen -----------------------------------------------*/

void DrawScreenText( int Xstart, int Ystart, char *msg, ...)
{
   static int lastX;
   static int lastY;
   char temp[ 120];
   va_list args;

   va_start( args, msg);
   vsprintf( temp, msg, args);
   va_end( args);
   if (Xstart < 0)
      Xstart = lastX;
   if (Ystart < 0)
      Ystart = lastY;
   outtextxy( Xstart, Ystart, temp);
   lastX = Xstart;
   lastY = Ystart + 10;  /* or textheight("W") ? */
}

/*- translate (dx, dy) into an integer angle value (0-65535) ---------------*/

unsigned ComputeAngle( int dx, int dy)
{
   return (unsigned) (atan2( (double) dy, (double) dx) * 10430.37835 + 0.5);
   /* Yes, I know this function could be in another file, but */
   /* this is the only source file that includes <math.h>...  */
}

/*- end of file ------------------------------------------------------------*/
