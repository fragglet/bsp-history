/*- BSP.C -------------------------------------------------------------------*

 Node builder for DOOM levels (c) 1998 Colin Reed, version 3.0 (dos extended)

 Performance increased 200% over 1.2x

 Many thanks to Mark Harrison for finding a bug in 1.1 which caused some
 texture align problems when a flipped SEG was split.

 Credit to:-

 Raphael Quinet (A very small amount of code has been borrowed from DEU).

 Matt Fell for the doom specs.

 Lee Killough for performance tuning, support for multilevel wads, special
 effects, and wads with lumps besides levels.

 Also, the original idea for some of the techniques where also taken from the
 comment at the bottom of OBJECTS.C in DEU, and the doc by Matt Fell about
 the nodes.

 Use this code for your own editors, but please credit me.

*---------------------------------------------------------------------------*/

#include "bsp.h"
#include "structs.h"

/*- Global Vars ------------------------------------------------------------*/

static FILE *outfile;
static char *testwad;
static char *outwad;

static struct directory *direc = NULL;

static struct Thing *things;
static long num_things = 0;

static struct Vertex *vertices;
static long num_verts = 0;

static struct LineDef *linedefs;
static long num_lines = 0;

static struct SideDef *sidedefs;
static long num_sides = 0;

static struct Sector *sectors;
static long num_sects = 0;

static struct SSector *ssectors;
static long num_ssectors = 0;

static struct Pseg *psegs = NULL;
static long num_psegs = 0;

static struct Pnode *pnodes = NULL;
static long num_pnodes = 0;
static long pnode_indx = 0;
static long num_nodes = 0;

static struct Block blockhead;
static short int *blockptrs;
static short int *blocklists=NULL;
static long blockptrs_size;

unsigned char *SectorHits;

static short lminx;
static short lmaxx;
static short lminy;
static short lmaxy;

static short mapminx;
static short mapmaxx;
static short mapminy;
static short mapmaxy;

static long psx,psy,pex,pey,pdx,pdy;
static long lsx,lsy,lex,ley;
static short node_x;
static short node_y;
static short node_dx;
static short node_dy;

static unsigned char pcnt;

static struct Seg *PickNode_traditional(struct Seg *);
static struct Seg *PickNode_visplane(struct Seg *);
static struct Seg *(*PickNode)(struct Seg *)=PickNode_traditional;
static int visplane;
static int noreject;

static struct lumplist {
 struct lumplist *next;
 struct directory *dir;
 void *data;
 char islevel;
} *lumplist,*current_level;

static char current_level_name[9];

static struct wad_header wad;

/*- Prototypes -------------------------------------------------------------*/

static void OpenWadFile(char *);
static struct lumplist *FindDir(const char *);
static void GetThings(void);
static void GetVertexes(void);
static void GetLinedefs(void);
static void GetSidedefs(void);
static void GetSectors(void);
static void FindLimits(struct Seg *);

static struct Seg *CreateSegs();

static struct Node *CreateNode(struct Seg *);
static int IsItConvex(struct Seg *);

static void ReverseNodes(struct Node *);
static long CreateBlockmap(void);

static void progress(void);
static int IsLineDefInside(int, int, int, int, int);
static int CreateSSector(struct Seg *);

/*--------------------------------------------------------------------------*/

static void progress()
{
	if(!((++pcnt)&31))
		fprintf(stderr,"%c\b","/-\\|"[((pcnt)>>5)&3]);
}

/*--------------------------------------------------------------------------*/
/* Find limits from a list of segs, does this by stepping through the segs*/
/* and comparing the vertices at both ends.*/
/*--------------------------------------------------------------------------*/

static void FindLimits(struct Seg *ts)
{
	int minx=INT_MAX,miny=INT_MAX,maxx=INT_MIN,maxy=INT_MIN;
	int fv,tv;

	for(;;)	{
		fv = ts->start;
		tv = ts->end;
/*		printf("%d : %d,%d\n",n,vertices[n].x,vertices[n].y);*/
		if(vertices[fv].x < minx) minx = vertices[fv].x;
		if(vertices[fv].x > maxx) maxx = vertices[fv].x;
		if(vertices[fv].y < miny) miny = vertices[fv].y;
		if(vertices[fv].y > maxy) maxy = vertices[fv].y;
		if(vertices[tv].x < minx) minx = vertices[tv].x;
		if(vertices[tv].x > maxx) maxx = vertices[tv].x;
		if(vertices[tv].y < miny) miny = vertices[tv].y;
		if(vertices[tv].y > maxy) maxy = vertices[tv].y;
		if(ts->next == NULL) break;
		ts = ts->next;
		}
	lminx = minx;
	lmaxx = maxx;
	lminy = miny;
	lmaxy = maxy;
}

/*--------------------------------------------------------------------------*/

static __inline__ int SplitDist(struct Seg *ts)
{
	double t,dx,dy;

	if(ts->flip==0)
		{
		dx = (double)(vertices[linedefs[ts->linedef].start].x)-(vertices[ts->start].x);
		dy = (double)(vertices[linedefs[ts->linedef].start].y)-(vertices[ts->start].y);

		if(dx == 0 && dy == 0) printf("Trouble in SplitDist %f,%f\n",dx,dy);
		t = sqrt((dx*dx) + (dy*dy));
		return (int)t;
		}
	else
		{
		dx = (double)(vertices[linedefs[ts->linedef].end].x)-(vertices[ts->start].x);
		dy = (double)(vertices[linedefs[ts->linedef].end].y)-(vertices[ts->start].y);

		if(dx == 0 && dy == 0) printf("Trouble in SplitDist %f,%f\n",dx,dy);
		t = sqrt((dx*dx) + (dy*dy));
		return (int)t;
		}
}

/*--------------------------------------------------------------------------*/
/* Find limits from a list of segs, does this by stepping through the segs*/
/* and comparing the vertices at both ends.*/

/*--------------------------------------------------------------------------*/
#include "funcs.c"
#include "picknode.c"
#include "makenode.c"


static struct Seg *add_seg(struct Seg *cs, int n, int fv, int tv,
                           struct Seg **fs, struct SideDef *sd)
{
 struct Seg *ts = GetMemory( sizeof( struct Seg));       /* get mem for Seg*/
 cs = cs ? (cs->next = ts) : (*fs = ts);
 cs->next = NULL;
 cs->start = fv;
 cs->end = tv;
 cs->pdx = (long) (cs->pex = vertices[tv].x)
                - (cs->psx = vertices[fv].x);
 cs->pdy = (long) (cs->pey = vertices[tv].y)
                - (cs->psy = vertices[fv].y);
 cs->ptmp = cs->pdx*cs->psy-cs->psx*cs->pdy;
 cs->len = (long) sqrt( (double) cs->pdx * cs->pdx +
                        (double) cs->pdy * cs->pdy);

 if ((cs->sector=sd->sector)==-1)
   printf("\nWarning: Bad sidedef in linedef %d (Z_CheckHeap error)\n",n);

 cs->angle = ComputeAngle(cs->pdx,cs->pdy);

 if (linedefs[n].tag==999)
   cs->angle=(cs->angle + (unsigned)(sd->xoff*(65536.0/360.0))) & 65535u;

 cs->linedef = n;
 cs->dist = 0;
 return cs;
}

/*- initially creates all segs, one for each line def ----------------------*/

static struct Seg *CreateSegs()
{
 struct Seg *cs = NULL;		/* current Seg */
 struct Seg *fs = NULL;		/* first Seg in list */
 struct LineDef *l = linedefs;
 int n;

 puts("Creating Segs ..........");

 for (n=0;n<num_lines;n++,l++)   /* step through linedefs and get side*/
  {											/* numbers*/
   if (l->sidedef1 != -1)
     (cs=add_seg(cs,n,l->start,l->end,&fs,sidedefs+l->sidedef1))->flip=0;
   else
     printf("\nWarning: Linedef %d has no right sidedef\n",n);

   if (l->sidedef2 != -1)
     (cs=add_seg(cs,n,l->end,l->start,&fs,sidedefs+l->sidedef2))->flip=1;
   else
     if (l->flags & 4)
       printf("\nWarning: Linedef %d is 2s but has no left sidedef\n",n);
  }
 return fs;
}

/*- get the directory from a wad file --------------------------------------*/
/* rewritten by Lee Killough to support multiple levels and extra lumps */

static void OpenWadFile(char *filename)
{
 long i;
 register struct directory *dir;
 struct lumplist *levelp=NULL;
 FILE *infile;

 if (!(infile = fopen(filename,"rb")))
   ProgError("Error: Cannot open WAD file %s", filename);

 if (fread(&wad,1,sizeof(wad),infile)!=sizeof(wad) ||
     (wad.type[0]!='I' && wad.type[0]!='P') || wad.type[1]!='W'
     || wad.type[2]!='A' || wad.type[3]!='D')
   ProgError("%s does not appear to be a wad file -- bad magic", filename);

 printf("Opened %cWAD file : %s. %lu dir entries at 0x%lX.\n",
	wad.type[0],filename,wad.num_entries,wad.dir_start);

 direc = GetMemory(sizeof(struct directory) * wad.num_entries);

 fseek(infile,wad.dir_start,0);
 fread(direc,sizeof(struct directory),wad.num_entries,infile);

 current_level=NULL;
 dir = direc;
 for (i=wad.num_entries;i--;dir++)
  {
   register struct lumplist *l=GetMemory(sizeof(*l));
   l->dir=dir;
   l->data=NULL;
   l->next=NULL;
   l->islevel=0;

   if ((dir->name[0]=='E' && dir->name[2]=='M' &&
        dir->name[1]>='1' && dir->name[1]<='9' &&
        dir->name[3]>='1' && dir->name[3]<='9' &&
       (!dir->name[4] || (dir->name[4]>='0' &&
                          dir->name[4]<='9' &&
                         !dir->name[5])  )) || (
        dir->name[0]=='M' && dir->name[1]=='A' &&
        dir->name[2]=='P' && !dir->name[5] &&
        dir->name[3]>='0' && dir->name[3]<='9' &&
        dir->name[4]>='0' && dir->name[4]<='9'))
    {
     dir->length=0;
     l->islevel=1;                                 /* Begin new level */
    }
   else
    {
     if (levelp && (!strncmp(dir->name,"SEGS",8) ||
                    !strncmp(dir->name,"SSECTORS",8) ||
                    !strncmp(dir->name,"NODES",8) ||
                    !strncmp(dir->name,"BLOCKMAP",8) ||
                    (!noreject && !strncmp(dir->name,"REJECT",8)) ||
                    FindDir(dir->name)))
       continue;  /* Ignore these since we're rebuilding them anyway */

     if (levelp && FindDir(dir->name))
      {
       printf("Warning: Duplicate entry \"%-8.8s\" ignored in %-.8s\n",
         dir->name, current_level->dir->name);
       continue;
      }

     if (dir->length)
      {
       l->data=GetMemory(dir->length);
       if (fseek(infile,dir->start,0) ||
           fread(l->data,1,dir->length,infile) != dir->length)
         printf("Warning: Trouble reading wad directory entry \"%-8.8s\" in %-.8s\n",
                dir->name, current_level->dir->name);
      }

     if (levelp && (!strncmp(dir->name,"THINGS",8) ||
         !strncmp(dir->name,"LINEDEFS",8) ||
         !strncmp(dir->name,"SIDEDEFS",8) ||
         !strncmp(dir->name,"VERTEXES",8) ||
         !strncmp(dir->name,"SECTORS",8) ||
         (noreject && !strncmp(dir->name,"REJECT",8))))
      {                       /* Store in current level */
       levelp->next=l;
       levelp=l;
       continue;
      }
    }

    /* Mark end of level and advance one main lump entry */

   if (levelp && !(current_level->data=current_level->next))
     current_level->islevel=0;  /* Ignore oddly formed levels */

   levelp = l->islevel ? l : NULL;

   if (current_level)
     current_level->next=l;
   else
     lumplist=l;
   current_level=l;
  }

 if (levelp)
  {
   if (!(current_level->data=current_level->next))
     current_level->islevel=0;  /* Ignore oddly formed levels */
   current_level->next=NULL;
  }
 fclose(infile);
}

/*- find the pointer to a resource -----------------------*/

static struct lumplist *FindDir(const char *name)
{
 struct lumplist *l = current_level;
 while (l && strncmp(l->dir->name,name,8))
   l=l->next;
 return l;
}

/*- read the things from the wad file and place in 'things' ----------------*/
static void GetThings(void)
{
	struct lumplist *l;

	l = FindDir("THINGS");

	if (!l || !(num_things = l->dir->length / sizeof( struct Thing)))
  	   ProgError("Must have at least 1 Thing");

       things = l->data;
}

/*- read the vertices from the wad file and place in 'vertices' ------------
    Rewritten by Lee Killough, to speed up performance                       */

static struct lumplist *vertlmp;

static void GetVertexes(void)
{
 long n,used_verts,i;
 int *translate;
 struct lumplist *l = FindDir("VERTEXES");

 if(!l || !(num_verts = l->dir->length / sizeof(struct Vertex)))
   ProgError("Couldn't find any Vertices");

 vertlmp = l;

 vertices = l->data;

 translate = GetMemory(num_verts*sizeof(*translate));

 for (n=0;n<num_verts;n++)     /* Unmark all vertices */
   translate[n]= -1;

 for (i=n=0;i<num_lines;i++)   /* Mark all used vertices */
  {                            /* Remove 0-length lines */
   int s=linedefs[i].start;
   int e=linedefs[i].end;
   if (s<0 || s>=num_verts || e<0 || e>=num_verts)
     ProgError("Linedef %ld has vertex out of range\n",i);
   if (vertices[s].x!=vertices[e].x || vertices[s].y!=vertices[e].y)
    {
     linedefs[n++]=linedefs[i];
     translate[s]=translate[e]=0;
    }
  }
 i-=num_lines=n;
 used_verts=0;
 for (n=0;n<num_verts;n++)     /* Sift up all unused vertices */
   if (!translate[n])
     vertices[translate[n]=used_verts++]=vertices[n];

 for (n=0;n<num_lines;n++)     /* Renumber vertices */
  {
   int s=translate[linedefs[n].start];
   int e=translate[linedefs[n].end];
   if (s < 0 || s >= used_verts || e < 0 || e >= used_verts)
     ProgError("Trouble in GetVertexes: Renumbering\n");
   linedefs[n].start=s;
   linedefs[n].end=e;
  }

 free(translate);

 printf("Loaded %ld vertices",num_verts);
 if (num_verts>used_verts)
   printf(", but %ld were unused\n(this is normal if the nodes were built before).\n",
     num_verts-used_verts);
 else
   puts(".");
 printf(i ? "%ld zero-length lines were removed.\n" : "\n",i);
 num_verts = used_verts;
 if (!num_verts)
   ProgError("Couldn't find any used Vertices");
}

/*- read the linedefs from the wad file and place in 'linedefs' ------------*/

static void GetLinedefs(void)
{
  struct lumplist *l = FindDir("LINEDEFS");

  if (!l || !(num_lines = l->dir->length / sizeof(struct LineDef)))
    ProgError("Couldn't find any Linedefs");

  linedefs = l->data;
}

/*- read the sidedefs from the wad file and place in 'sidedefs' ------------*/

static void GetSidedefs(void)
{
  struct lumplist *l = FindDir("SIDEDEFS");

  if (!l || !(num_sides = l->dir->length / sizeof(struct SideDef)))
    ProgError("Couldn't find any Sidedefs");

  sidedefs = l->data;
}

/*- read the sectors from the wad file and place count in 'num_sectors' ----*/

static void GetSectors(void)
{
  struct lumplist *l = FindDir("SECTORS");

  if (!l || !(num_sects = l->dir->length / sizeof(struct Sector)))
    ProgError("Couldn't find any Sectors");

  sectors = l->data;
}

/*--------------------------------------------------------------------------*/

static int IsLineDefInside(int ldnum, int xmin, int ymin, int xmax, int ymax )
{
 int x1 = vertices[ linedefs[ ldnum].start].x;
 int y1 = vertices[ linedefs[ ldnum].start].y;
 int x2 = vertices[ linedefs[ ldnum].end].x;
 int y2 = vertices[ linedefs[ ldnum].end].y;
 int count=2;

 for (;;)
   if (y1>ymax)
    {
     if (y2>ymax)
       return(FALSE);
     x1=x1+(x2-x1)*(double)(ymax-y1)/(y2-y1);
     y1=ymax;
     count=2;
    }
   else
     if (y1<ymin)
      {
       if (y2<ymin)
         return(FALSE);
       x1=x1+(x2-x1)*(double)(ymin-y1)/(y2-y1);
       y1=ymin;
       count=2;
      }
     else
       if (x1>xmax)
        {
         if (x2>xmax)
           return(FALSE);
         y1=y1+(y2-y1)*(double)(xmax-x1)/(x2-x1);
         x1=xmax;
         count=2;
        }
       else
         if (x1<xmin)
          {
           if (x2<xmin)
             return(FALSE);
           y1=y1+(y2-y1)*(double)(xmin-x1)/(x2-x1);
           x1=xmin;
           count=2;
          }
         else
          {
           int t;
           if (!--count)
             return(TRUE);
           t=x1;x1=x2;x2=t;
           t=y1;y1=y2;y2=t;
          }
}

/*- Create blockmap --------------------------------------------------------*/

static long CreateBlockmap()
{
	long blockoffs = 0;
	int x,y,n;
	int blocknum = 0;

	blockhead.minx = mapminx&-8;
	blockhead.miny = mapminy&-8;
	blockhead.xblocks = ((mapmaxx - (mapminx&-8)) / 128) + 1;
	blockhead.yblocks = ((mapmaxy - (mapminy&-8)) / 128) + 1;

	blockptrs_size = (blockhead.xblocks*blockhead.yblocks)*2;
	blockptrs = GetMemory(blockptrs_size);

	for(y=0;y<blockhead.yblocks;y++)
		{
		for(x=0;x<blockhead.xblocks;x++)
			{
			progress();

			blockptrs[blocknum]=(blockoffs+4+(blockptrs_size/2));

			blocklists = ResizeMemory(blocklists,((blockoffs+1)*2));
			blocklists[blockoffs]=0;
			blockoffs++;
			for(n=0;n<num_lines;n++)
				{
				if(IsLineDefInside(n,(blockhead.minx+(x*128)),(blockhead.miny+(y*128)),(blockhead.minx+(x*128))+127,(blockhead.miny+(y*128))+127))
					{
/*					printf("found line %d in block %d\n",n,blocknum);*/
					blocklists = ResizeMemory(blocklists,((blockoffs+1)*2));
					blocklists[blockoffs]=n;
					blockoffs++;
					}
				}
			blocklists = ResizeMemory(blocklists,((blockoffs+1)*2));
			blocklists[blockoffs]=-1;
			blockoffs++;

			blocknum++;
			}
		}
	return blockoffs*2;
}

/*--------------------------------------------------------------------------*/

static void ReverseNodes(struct Node *tn)
{
 struct Pnode *pn;

 tn->chright = tn->nextr ? ReverseNodes(tn->nextr), tn->nextr->node_num :
                           tn->chright | 0x8000;

 tn->chleft  = tn->nextl ? ReverseNodes(tn->nextl), tn->nextl->node_num :
                           tn->chleft  | 0x8000;

 pn = pnodes + pnode_indx++;

 pn->x = tn->x;
 pn->y = tn->y;
 pn->dx = tn->dx;
 pn->dy = tn->dy;
 pn->maxy1 = tn->maxy1;
 pn->miny1 = tn->miny1;
 pn->minx1 = tn->minx1;
 pn->maxx1 = tn->maxx1;
 pn->maxy2 = tn->maxy2;
 pn->miny2 = tn->miny2;
 pn->minx2 = tn->minx2;
 pn->maxx2 = tn->maxx2;
 pn->chright = tn->chright;
 pn->chleft = tn->chleft;
 tn->node_num = num_pnodes++;
}

/* Add a lump to current level
   by Lee Killough */

static void add_lump(const char *name, void *data, size_t length)
{
 struct lumplist *l;
 for (l=current_level;l;l=l->next)
   if (!strncmp(name,l->dir->name,8))
     break;
 if (!l)
  {
   l=current_level;
   while (l->next)
     l=l->next;
   l->next = GetMemory(sizeof(*l));
   l = l->next;
   l->next=NULL;
   l->dir = GetMemory(sizeof(struct directory));
   strncpy(l->dir->name,name,8);
  }
 l->dir->length=length;
 l->islevel=0;
 l->data=data;
}

static struct directory write_lump(struct lumplist *lump)
{
 if (ftell(outfile)!=lump->dir->start || (lump->dir->length &&
   fwrite(lump->data, 1, lump->dir->length, outfile) != lump->dir->length))
   printf("Warning: Consistency check failure writing %-.8s\n", lump->dir->name);
 return *lump->dir;
}

static void sortlump(struct lumplist **link)
{
 static const char *const lumps[10]={"THINGS", "LINEDEFS", "SIDEDEFS",
  "VERTEXES", "SEGS", "SSECTORS", "NODES", "SECTORS", "REJECT", "BLOCKMAP"};
 int i=sizeof(lumps)/sizeof(*lumps)-1;
 struct lumplist **l;
 do
   for (l=link;*l;l=&(*l)->next)
     if (!strncmp(lumps[i],(*l)->dir->name,8))
      {
       struct lumplist *t=(*l)->next;
       (*l)->next=*link;
       *link=*l;
       *l=t;
       break;
      }
 while (--i>=0);
}


void usage(void)
{
 printf("\nThis Node builder was created from the basic theory stated in DEU5 (OBJECTS.C)\n"
        "\nCredits should go to :-\n"
        "Matt Fell      (msfell@aol.com) for the Doom Specs.\n"
        "Raphael Quinet (Raphael.Quinet@eed.ericsson.se) for DEU and the original idea.\n"
        "Mark Harrison  (harrison@lclark.edu) for finding a bug in 1.1x\n"
        "Jim Flynn      (jflynn@pacbell.net) for many good ideas and encouragement.\n"
        "Jan Van der Veken for finding invisible barrier bug.\n"
#ifdef MSDOS
        "\nUsage: BSP [options] input.wad [[-o] <output.wad>]\n"
#else
        "\nUsage: BSP [options] input.wad [-o <output.wad>]\n"
#endif
        "       (If no output.wad is specified, TMP.WAD is written)\n\n"
        "Options:\n\n"
        "  -factor <nnn>  Changes the cost assigned to SEG splits\n"
        "  -vp            Attempts to prevent visplane overflows\n"
        "  -noreject      Does not clobber reject map\n"
       );
 exit(1);
}

static void parse_options(int argc, char *argv[])
{
 static char *fnames[2];
 static const struct {
   const char *option;
   void *var;
   enum {NONE, STRING, INT} arg;
 } tab[]= { {"-vp", &visplane, NONE},
            {"-noreject", &noreject, NONE},
            {"-factor", &factor, INT},
            {"-o", fnames+1, STRING},
          };
 int nf=0;

 while (--argc)
   if (**++argv=='-')
    {
     int i=sizeof(tab)/sizeof(*tab);
     for (;;)
       if (!strcmp(*argv,tab[--i].option))
        {
         if (tab[i].arg==INT)
           if (--argc)
            {
             char *end;
             *(int *) tab[i].var=strtol(*++argv,&end,0);
             if (*end || factor<0)
               usage();
            }
           else
             usage();
         else
	   if (tab[i].arg==STRING)
	     if (--argc)
	       *(char **) tab[i].var = *++argv;
	     else
	       usage();
	   else
	     ++*(int *) tab[i].var;
         break;
        }
       else
         if (!i)
          {
           usage();
           break;
          }
    }
   else
#ifdef MSDOS
     if (nf<2)
#else
     if (nf<1)
#endif
       fnames[nf++]=*argv;
     else
       usage();

 testwad = fnames[0];                          /* Get input name*/

 if (!testwad || factor<0)
   usage();

 outwad = fnames[1] ? fnames[1] : "tmp.wad";   /* Get output name*/
}

/* Height of nodes */
static unsigned height(const struct Node *tn)
{
 if (tn)
  {
   unsigned l = height(tn->nextl), r = height(tn->nextr);
   return l>r ? l+1 : r+1;
  }
 return 1;
}


/*- Main Program -----------------------------------------------------------*/

int main(int argc,char *argv[])
{
 struct lumplist *lump,*l;
 struct directory *newdirec;

 setbuf(stdout,NULL);

 puts("* Doom BSP node builder ver 3.0 (c) 1998 Colin Reed, Lee Killough *");

 parse_options(argc,argv);

 OpenWadFile(testwad);		/* Opens and reads directory*/

 printf("\nCreating nodes using tunable factor of %d\n",factor);

 if (visplane)
  {
   puts("\nTaking special measures to reduce the chances of visplane overflow");
   PickNode=PickNode_visplane;
  }

 for (lump=lumplist; lump; lump=lump->next)
  if (lump->islevel)
    {
     struct Seg *tsegs;
     static struct Node *nodelist;

     current_level=lump->data;
     strncpy(current_level_name,lump->dir->name,8);
     printf("\nBuilding nodes on %-.8s\n\n",current_level_name);

     blockptrs=NULL;
     blocklists=NULL;
     blockptrs_size=0;
     num_ssectors=0;
     num_psegs=0;
     num_nodes=0;

     GetThings();
     GetLinedefs();											/* Get linedefs and vertices*/
     GetVertexes();											/* and delete redundant.*/
     GetSidedefs();
     GetSectors();

     tsegs = CreateSegs();		  						/* Initially create segs*/

     FindLimits(tsegs);									/* Find limits of vertices*/

     mapminx = lminx;										/* store as map limits*/
     mapmaxx = lmaxx;
     mapminy = lminy;
     mapmaxy = lmaxy;

     printf("Map goes from (%d,%d) to (%d,%d)\n",lminx,lminy,lmaxx,lmaxy);

     SectorHits = GetMemory(num_sects);

     nodelist = CreateNode(tsegs);	  /* recursively create nodes*/

     fputs(" \n",stderr);

     printf("%lu NODES created, with %lu SSECTORS.\n",num_nodes,num_ssectors);

     printf("Found %lu used vertices\n",num_verts);

     printf("Heights of left and right subtrees = (%u,%u)\n",
         height(nodelist->nextl), height(nodelist->nextr));

     vertlmp->dir->length=num_verts * sizeof(struct Vertex);
     vertlmp->data=vertices;

     add_lump("SEGS", psegs, sizeof(struct Pseg)*num_psegs);

     add_lump("SSECTORS", ssectors, sizeof(struct SSector)*num_ssectors);

     if (!FindDir("REJECT"))
      {
       long reject_size = (num_sects*num_sects+7)/8;
       void *data = GetMemory(reject_size);
       memset(data,0,reject_size);
       add_lump("REJECT", data, reject_size);
      }

     {
      long blockmap_size = CreateBlockmap();
      char *data = GetMemory(blockmap_size+blockptrs_size+8);
      memcpy(data,&blockhead,8);
      memcpy(data+8,blockptrs,blockptrs_size);
      memcpy(data+8+blockptrs_size,blocklists,blockmap_size);
      free(blockptrs);
      free(blocklists);
      add_lump("BLOCKMAP",data,blockmap_size+blockptrs_size+8);
      puts("Completed blockmap building");
     }

    pnodes = GetMemory(sizeof(struct Pnode)*num_nodes);
    num_pnodes = 0;
    pnode_indx = 0;
    ReverseNodes(nodelist);
    add_lump("NODES", pnodes, sizeof(struct Pnode)*num_pnodes);

    free(SectorHits);

   }  /* if (lump->islevel) */

 {
  long offs=12;
  wad.num_entries=0;
  for (lump=lumplist; lump; lump=lump->next)
   {
    lump->dir->start=offs;
    offs+=lump->dir->length;
    wad.num_entries++;
    if (lump->islevel)
     {
      sortlump((struct lumplist **) &lump->data);
      for (l=lump->data; l; l=l->next)
       {
        l->dir->start=offs;
        offs+=l->dir->length;
        wad.num_entries++;
       }
     }
   }

  newdirec = GetMemory(wad.num_entries*sizeof(struct directory));

  if (!(outfile=fopen(outwad,"wb")))
   {
    fputs("Error: Could not open output PWAD file ",stderr);
    perror(outwad);
    exit(1);
   }

  wad.dir_start=offs;
  if (fwrite(&wad,1,12,outfile)!=12)
    puts("Warning: Consistency check failure writing wad header");

  for (offs=0,lump=lumplist; lump; lump=lump->next)
   {
    newdirec[offs++]=write_lump(lump);
    if (lump->islevel)
      for (l=lump->data; l; l=l->next)
        newdirec[offs++]=write_lump(l);
   }

  if (ftell(outfile)!=wad.dir_start ||
    fwrite(newdirec,sizeof(struct directory),wad.num_entries,outfile)!=wad.num_entries)
    puts("Warning: Consistency check failure writing lump directory");
  fclose(outfile);

  if (offs!=wad.num_entries)
    puts("Warning: Lump directory count consistency check failure");
  printf("\nSaved WAD as %s\n",outwad);
 }
 return 0;
}

/*- end of file ------------------------------------------------------------*/
