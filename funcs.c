/*- FUNCS.C ----------------------------------------------------------------*/

#include "bsp.h"

/*- terminate the program reporting an error -------------------------------*/

void ProgError( char *errstr, ...)
{
   va_list args;

   va_start( args, errstr);
   printf( "\nProgram Error: *** ");
   vprintf( errstr, args);
   printf( " ***\n");
   va_end( args);
   exit( 5);
}

/*- allocate memory with error checking ------------------------------------*/

void *GetMemory( size_t size)
{
   void *ret = malloc( size);
   if (!ret)
      ProgError( "out of memory (cannot allocate %u bytes)", size);
   return ret;
}

/*- reallocate memory with error checking ----------------------------------*/

void *ResizeMemory( void *old, size_t size)
{
   void *ret = realloc( old, size);
   if (!ret)
      ProgError( "out of memory (cannot reallocate %u bytes)", size);
   return ret;
}

/*- allocate memory from the far heap with error checking ------------------*/

void huge *GetFarMemory( unsigned long size)
{
   void huge *ret = farmalloc( size);
   if (!ret)
      ProgError( "out of memory (cannot allocate %lu far bytes)", size);
   return ret;
}

/*- reallocate memory from the far heap with error checking ----------------*/

void huge *ResizeFarMemory( void huge *old, unsigned long size)
{
   void huge *ret = farrealloc( old, size);
   if (!ret)
      ProgError( "out of memory (cannot reallocate %lu far bytes)", size);
   return ret;
}

/*--------------------------------------------------------------------------*/

