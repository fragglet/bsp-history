/* $Id: endian.c,v 1.1 2000/08/27 20:38:19 cph Exp $
 *
 * Endianness correction for Doom level structures
 * Written by Oliver Kraus <olikraus@yahoo.com>
 *
 */

#include "structs.h"
#include "bsp.h"

/*-- BIGEND functions ------------------------------------------------------*/

#ifndef WORDS_BIGENDIAN
void ConvertAll(void) {};

void swapshort(unsigned short *i) {};
void swaplong(unsigned long *l) {};
void swapint(unsigned int *l) {};

#else

void swapshort(unsigned short *i)
{
  unsigned short int t;

  ((char *) &t)[ 0] = ((char *) i)[ 1];
  ((char *) &t)[ 1] = ((char *) i)[ 0];
  *i = t;
}

void swaplong(unsigned long *l)
{
  unsigned long t;

  ((char *) &t)[ 0] = ((char *) l)[ 3];
  ((char *) &t)[ 1] = ((char *) l)[ 2];
  ((char *) &t)[ 2] = ((char *) l)[ 1];
  ((char *) &t)[ 3] = ((char *) l)[ 0];
  *l = t;
}

void swapint(unsigned int *l)
{
  unsigned int t;

  ((char *) &t)[ 0] = ((char *) l)[ 3];
  ((char *) &t)[ 1] = ((char *) l)[ 2];
  ((char *) &t)[ 2] = ((char *) l)[ 1];
  ((char *) &t)[ 3] = ((char *) l)[ 0];
  *l = t;
}

static void ConvertThing(void)
{
  int i, cnt;
  struct Thing *s;
  struct lumplist *l = FindDir("THINGS");
  if ( l == NULL )
    return;
  cnt = l->dir->length / sizeof(struct Thing);
  for ( i = 0; i < cnt; i++ )
  {
    s = ((struct Thing *)l->data)+i;
    swapshort((unsigned short *)&(s->xpos));
    swapshort((unsigned short *)&(s->ypos));
    swapshort((unsigned short *)&(s->angle));
    swapshort((unsigned short *)&(s->type));
    swapshort((unsigned short *)&(s->when));
  }
}

static void ConvertVertex(void)
{
  int i, cnt;
  struct Vertex *s;
  struct lumplist *l = FindDir("VERTEXES");
  if ( l == NULL )
    return;
  cnt = l->dir->length / sizeof(struct Vertex);
  for ( i = 0; i < cnt; i++ )
  {
    s = ((struct Vertex *)l->data)+i;
    swapshort((unsigned short *)&(s->x));
    swapshort((unsigned short *)&(s->y));
  }
}

static void ConvertLineDef(void)
{
  int i, cnt;
  struct LineDef *s;
  struct lumplist *l = FindDir("LINEDEFS");
  if ( l == NULL )
    return;
  cnt = l->dir->length / sizeof(struct LineDef);
  for ( i = 0; i < cnt; i++ )
  {
    s = ((struct LineDef *)l->data)+i;
    swapshort((unsigned short *)&(s->start));
    swapshort((unsigned short *)&(s->end));
    swapshort((unsigned short *)&(s->flags));
    swapshort((unsigned short *)&(s->type));
    swapshort((unsigned short *)&(s->tag));
    swapshort((unsigned short *)&(s->sidedef1));
    swapshort((unsigned short *)&(s->sidedef2));
  }
}

static void ConvertSidedefs(void)
{
  int i, cnt;
  struct SideDef *s;
  struct lumplist *l = FindDir("SIDEDEFS");
  if ( l == NULL )
    return;
  cnt = l->dir->length / sizeof(struct SideDef);
  for ( i = 0; i < cnt; i++ )
  {
    s = ((struct SideDef *)l->data)+i;
    swapshort((unsigned short *)&(s->xoff));
    swapshort((unsigned short *)&(s->yoff));
    swapshort((unsigned short *)&(s->sector));
  }
}

static void ConvertSector(void)
{
  int i, cnt;
  struct Sector *s;
  struct lumplist *l = FindDir("SECTORS");
  if ( l == NULL )
    return;
  cnt = l->dir->length / sizeof(struct Sector);
  for ( i = 0; i < cnt; i++ )
  {
    s = ((struct Sector *)l->data)+i;
    swapshort((unsigned short *)&(s->floorh));
    swapshort((unsigned short *)&(s->ceilh));
    swapshort((unsigned short *)&(s->light));
    swapshort((unsigned short *)&(s->special));
    swapshort((unsigned short *)&(s->tag));
  }
}

static void ConvertPseg(void)
{
  int i, cnt;
  struct Pseg *s;
  struct lumplist *l = FindDir("SEGS");
  if ( l == NULL )
    return;
  cnt = l->dir->length / sizeof(struct Pseg);
  for ( i = 0; i < cnt; i++ )
  {
    s = ((struct Pseg *)l->data)+i;
    swapshort((unsigned short *)&(s->start));
    swapshort((unsigned short *)&(s->end));
    swapshort((unsigned short *)&(s->angle));
    swapshort((unsigned short *)&(s->linedef));
    swapshort((unsigned short *)&(s->flip));
    swapshort((unsigned short *)&(s->dist));
  }
}

static void ConvertSSector(void)
{
  int i, cnt;
  struct SSector *s;
  struct lumplist *l = FindDir("SSECTORS");
  if ( l == NULL )
    return;
  cnt = l->dir->length / sizeof(struct SSector);
  for ( i = 0; i < cnt; i++ )
  {
    s = ((struct SSector *)l->data)+i;
    swapshort((unsigned short *)&(s->num));
    swapshort((unsigned short *)&(s->first));
  }
}


static void ConvertPnode(void)
{
  int i, cnt;
  struct Pnode *s;
  struct lumplist *l = FindDir("NODES");
  if ( l == NULL )
    return;
  cnt = l->dir->length / sizeof(struct Pnode);
  for ( i = 0; i < cnt; i++ )
  {
    s = ((struct Pnode *)l->data)+i;
    swapshort((unsigned short *)&(s->x));
    swapshort((unsigned short *)&(s->y));
    swapshort((unsigned short *)&(s->dx));
    swapshort((unsigned short *)&(s->dy));
    swapshort((unsigned short *)&(s->maxy1));
    swapshort((unsigned short *)&(s->miny1));
    swapshort((unsigned short *)&(s->maxx1));
    swapshort((unsigned short *)&(s->minx1));
    swapshort((unsigned short *)&(s->maxy2));
    swapshort((unsigned short *)&(s->miny2));
    swapshort((unsigned short *)&(s->maxx2));
    swapshort((unsigned short *)&(s->minx2));
    swapshort((unsigned short *)&(s->chright));
    swapshort((unsigned short *)&(s->chleft));
  }
}


void ConvertAll(void)
{
  Verbose("Doing endianness correction... ");
  ConvertThing();
  ConvertVertex();
  ConvertLineDef();
  ConvertSidedefs();
  ConvertSector();
  ConvertPseg();
  ConvertSSector();
  ConvertPnode();
  /* should we consider the REJECT map....???? 
   * cph - no, REJECT is effectively a byte array */
  /* BLOCKMAP is converted inside CreateBlockmap */
  Verbose("done\n");
}

#endif

