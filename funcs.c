/*- FUNCS.C ----------------------------------------------------------------*/
/* $Id: funcs.c,v 1.6 2001/07/21 15:10:08 cph Exp $ */
/*- terminate the program reporting an error -------------------------------*/

#include "structs.h"
#include "bsp.h"

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

void ProgError(const char *errstr, ...)
{
   va_list args;

   va_start( args, errstr);
   fprintf(stderr, "\nProgram Error: *** ");
   vfprintf( stderr, errstr, args);
   fprintf(stderr, " ***\n");
   va_end( args);
#ifdef HAVE_UNLINK
   if (unlinkwad) unlink(unlinkwad);
#endif
   exit( 5);
}

/* Print stuff if verbose output */

int verbosity;

void Verbose(const char *errstr, ...)
{
   va_list args;

   if (!verbosity) return;

   va_start( args, errstr);
   vprintf(errstr, args);
   va_end( args);
}

#ifndef HAVE_LIBDMALLOC
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


#endif
/*--------------------------------------------------------------------------*/

