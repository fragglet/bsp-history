/*- BSP.C -------------------------------------------------------------------*

 Node builder for DOOM levels (c) 1997 Colin Reed, version 2.0 (dos extended)

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
/* static long things_start = 0; */

static struct Vertex *vertices;
static long num_verts = 0;
/* static long vertices_start = 0; */

static struct LineDef *linedefs;
static long num_lines = 0;
/* static long linedefs_start = 0; */

static struct SideDef *sidedefs;
static long num_sides = 0;
/* static long sidedefs_start = 0; */

static struct Sector *sectors;
static long num_sects = 0;
/* static long sectors_start = 0; */

static struct SSector *ssectors;
static long num_ssectors = 0;
/* static long ssectors_start = 0; */

static struct Pseg *psegs = NULL;
static long num_psegs = 0;
/* static long segs_start = 0; */

static struct Pnode *pnodes = NULL;
static long num_pnodes = 0;
static long pnode_indx = 0;
/* static long pnodes_start = 0; */

/* static struct Seg *tsegs = NULL; */
/* static long num_tsegs = 0; */

static long num_nodes = 0;

/*
static long reject_start;
static long reject_size;
*/

static struct Block blockhead;
static short int *blockptrs;
static short int *blocklists=NULL;
static long blockptrs_size;
/* static long blockmap_size; */
/* static long blockmap_start; */

/* static struct splitter sp; */

static short node_x;
static short node_y;
static short node_dx;
static short node_dy;

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

static unsigned char pcnt;


static struct lumplist {
 struct lumplist *next;
 struct directory *dir;
 void *data;
 char islevel;
} *lumplist,*current_level;

static struct wad_header wad;


/*- Prototypes -------------------------------------------------------------*/

static __inline__ void OpenWadFile(char *);
static __inline__ struct lumplist *FindDir(char *);
static __inline__ void GetThings(void);
static __inline__ void GetVertexes(void);
static __inline__ void GetLinedefs(void);
static __inline__ void GetSidedefs(void);
static __inline__ void GetSectors(void);
static __inline__ void FindLimits(struct Seg *);

static __inline__ struct Seg *CreateSegs();

static struct Node *CreateNode(struct Seg *);
static __inline__ void DivideSegs(struct Seg *,struct Seg **,struct Seg **);
static __inline__ int IsItConvex(struct Seg *);

static __inline__ struct Seg *PickNode(struct Seg *);
static __inline__ void ComputeIntersection(short int *,short int *);
static __inline__ int DoLinesIntersect();
static __inline__ int SplitDist(struct Seg *);

static void ReverseNodes(struct Node *);
static __inline__ long CreateBlockmap(void);

static __inline__ void progress(void);
static __inline__ int IsLineDefInside(int, int, int, int, int);
static __inline__ int CreateSSector(struct Seg *);


/*--------------------------------------------------------------------------*/

static __inline__ void progress()
{
	if(!((++pcnt)&15))
		fprintf(stderr,"%c\b","/-\\|"[((pcnt)>>4)&3]);
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

static __inline__ void FindLimits(struct Seg *ts)
{
	int minx,miny,maxx,maxy;
	int fv,tv;

	minx = 32767;
	maxx = -32767;
	miny = 32767;
	maxy = -32767;

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
#include "funcs.c"
#include "picknode.c"
#include "makenode.c"

/*- initially creates all segs, one for each line def ----------------------*/

static __inline__ struct Seg *CreateSegs()
{
	struct Seg *cs = NULL;					/* current and temporary Segs*/
	struct Seg *ts = NULL;
	struct Seg *fs = NULL;					/* first Seg in list*/

	short	n,fv,tv;
	int	dx,dy;

	puts("Creating Segs ..........");

	for(n=0; n < num_lines; n++)			/* step through linedefs and get side*/
		{											/* numbers*/
		fv = linedefs[n].start;
		tv = linedefs[n].end;

		if(linedefs[n].sidedef1 != -1)
			{														/* create normal seg*/
			ts = GetMemory( sizeof( struct Seg));		/* get mem for Seg*/
			if(cs)
				{
				cs->next = ts;
				cs = ts;
				cs->next = NULL;
				}
			else
				{
				fs = cs = ts;
				cs->next = NULL;
				}
			cs->start = fv;
			cs->end = tv;
/*			printf("%d,%d\n",fv,tv);*/
			dx = (vertices[tv].x-vertices[fv].x);
			dy = (vertices[tv].y-vertices[fv].y);
			cs->angle = ComputeAngle(dx,dy);
			cs->linedef = n;
			cs->dist = 0;
			cs->flip = 0;
/*			num_tsegs++; */
			}
		if(linedefs[n].sidedef2 != -1)
			{														/* create flipped seg*/
			ts = GetMemory( sizeof( struct Seg));		/* get mem for Seg*/
			if(cs)
				{
				cs->next = ts;
				cs = ts;
				cs->next = NULL;
				}
			else
				{
				fs = cs = ts;
				cs->next = NULL;
				}
			cs->start = tv;
			cs->end = fv;
			dx = (vertices[fv].x-vertices[tv].x);
			dy = (vertices[fv].y-vertices[tv].y);
			cs->angle = ComputeAngle(dx,dy);
			cs->linedef = n;
			cs->dist = 0;
			cs->flip = 1;
/*			num_tsegs++; */
			}
		}

	return fs;
}

/*- get the directory from a wad file --------------------------------------*/
/* rewritten by Lee Killough to support multiple levels and extra lumps */


static __inline__ void OpenWadFile(char *filename)
{
 long i;
 register struct directory *dir;
 struct lumplist *levelp=NULL;
 register struct lumplist *lumpp=NULL;
 FILE *infile;

	if(!(infile = fopen(filename,"rb")))
 	{
         fprintf(stderr,"Error: Cannot find WAD file %s\n", filename);
	 exit(1);
	}

	fread(&wad,sizeof(wad),1,infile);

	printf("Opened %cWAD file : %s. %lu dir entries at 0x%lX.\n",
		wad.type[0],filename,wad.num_entries,wad.dir_start);

	direc = GetMemory(sizeof(struct directory) * wad.num_entries);

	fseek(infile,wad.dir_start,0);
	fread(direc,sizeof(struct directory),wad.num_entries,infile);

        dir = direc;
        for (i=wad.num_entries;i--;dir++)
          {
           register struct lumplist *l=GetMemory(sizeof(*l));
           l->dir=dir;
           l->data=NULL;
	   l->next=NULL;
           l->islevel=0;

           if (dir->length)
            {
             l->data=GetMemory(dir->length);
             fseek(infile,dir->start,0);
             fread(l->data,dir->length,1,infile);
            }

           if (!dir->length && ( (dir->name[0]=='E' && dir->name[2]=='M' &&
                dir->name[1]>='1' && dir->name[1]<='4' &&
                dir->name[3]>='1' && dir->name[3]<='9' &&
               !dir->name[4] ) || (
                dir->name[0]=='M' && dir->name[1]=='A' &&
                dir->name[2]=='P' &&
		((dir->name[3]>='0' && dir->name[4]>'0') ||
                 (dir->name[3]>'0' && dir->name[4]=='0')) &&
                ((dir->name[3]<'3' && dir->name[4]<='9') ||
		 (dir->name[3]=='3' && dir->name[4]<='2')) &&
	        !dir->name[5] )))
                   l->islevel=1;          /* Begin new level */
           else
             if (levelp && (
	         !strncmp(dir->name,"THINGS",8) ||
                 !strncmp(dir->name,"LINEDEFS",8) ||
                 !strncmp(dir->name,"SIDEDEFS",8) ||
                 !strncmp(dir->name,"VERTEXES",8) ||
                 !strncmp(dir->name,"SECTORS",8) ) )
               {                       /* Store in current level */
                levelp->next=l;
                levelp=l;
                continue;
               }
             else
               if (levelp && (
	           !strncmp(dir->name,"SEGS",8) ||
                   !strncmp(dir->name,"SSECTORS",8) ||
                   !strncmp(dir->name,"NODES",8) ||
                   !strncmp(dir->name,"BLOCKMAP",8) ||
		   !strncmp(dir->name,"REJECT",8)  ) )
                {    /* Ignore these since we're rebuilding them anyway */
                 if (dir->length)
                   free(l->data);
                 free(l);
                 continue;
                }

    /* Mark end of level and advance one main lump entry */

           if (levelp && !(lumpp->data=lumpp->next))
             lumpp->islevel=0;  /* Ignore oddly formed levels */

           levelp=l->islevel ? l : NULL;
           if (lumpp)
             lumpp->next=l;
           else
             lumplist=l;
           lumpp=l;
        }
   if (levelp)
    {
     if (!(lumpp->data=lumpp->next))
       lumpp->islevel=0;  /* Ignore oddly formed levels */
     lumpp->next=NULL;
    }
   fclose(infile);
}

/*- find the pointer to a resource -----------------------*/

static struct lumplist *FindDir(char *name)
{
        struct lumplist *l = current_level;
        while (l && strncmp(l->dir->name,name,8))
 	  l=l->next;
	return l;
}

/*- read the things from the wad file and place in 'things' ----------------*/
static __inline__ void GetThings(void)
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

static __inline__ void GetVertexes(void)
{
	long n,used_verts,i;
        int *translate;
        struct lumplist *l;
	l = FindDir("VERTEXES");

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
            ProgError("Linedef has vertex out of range\n");
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
/*
  	  else
	    printf("Vertex [%d] not used.\n",n);
*/
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
	  printf(", but %ld were unused.",num_verts-used_verts);
	printf(i ? "%ld zero-length lines were removed.\n" : "\n",i);
	num_verts = used_verts;
	if(!num_verts)
	  ProgError("Couldn't find any used Vertices");

}

/*- read the linedefs from the wad file and place in 'linedefs' ------------*/

static __inline__ void GetLinedefs(void)
{
        struct lumplist *l;

	l = FindDir("LINEDEFS");

	if(!l || !(num_lines = l->dir->length / sizeof(struct LineDef)))
	  ProgError("Couldn't find any Linedefs");

	linedefs = l->data;
}

/*- read the sidedefs from the wad file and place in 'sidedefs' ------------*/

static __inline__ void GetSidedefs(void)
{

	struct lumplist *l;

	l = FindDir("SIDEDEFS");

	if (!l || !(num_sides = l->dir->length / sizeof(struct SideDef)))
	   ProgError("Couldn't find any Sidedefs");

	sidedefs = l->data;
}

/*- read the sectors from the wad file and place count in 'num_sectors' ----*/

static __inline__ void GetSectors(void)
{
	struct lumplist *l;

	l = FindDir("SECTORS");

	if (!l || !(num_sects = l->dir->length / sizeof(struct Sector)))
	   ProgError("Couldn't find any Sectors");

	sectors = l->data;
}

/*--------------------------------------------------------------------------*/

static __inline__ int IsLineDefInside(int ldnum, int xmin, int ymin, int xmax, int ymax )
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
     x1=x1+(x2-x1)*(ymax-y1)/(y2-y1);
     y1=ymax;
     count=2;
    }
   else
     if (y1<ymin)
      {
       if (y2<ymin)
         return(FALSE);
       x1=x1+(x2-x1)*(ymin-y1)/(y2-y1);
       y1=ymin;
       count=2;
      }
     else
       if (x1>xmax)
        {
         if (x2>xmax)
           return(FALSE);
         y1=y1+(y2-y1)*(xmax-x1)/(x2-x1);
         x1=xmax;
         count=2;
        }
       else
         if (x1<xmin)
          {
           if (x2<xmin)
             return(FALSE);
           y1=y1+(y2-y1)*(xmin-x1)/(x2-x1);
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

static __inline__ long CreateBlockmap()
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
	if((tn->chright & 0x8000) == 0)
		{
		ReverseNodes(tn->nextr);
		tn->chright = tn->nextr->node_num;
		}
	if((tn->chleft & 0x8000) == 0)
		{
		ReverseNodes(tn->nextl);
		tn->chleft = tn->nextl->node_num;
		}
	pnodes[pnode_indx].x = tn->x;
	pnodes[pnode_indx].y = tn->y;
	pnodes[pnode_indx].dx = tn->dx;
	pnodes[pnode_indx].dy = tn->dy;
	pnodes[pnode_indx].maxy1 = tn->maxy1;
	pnodes[pnode_indx].miny1 = tn->miny1;
	pnodes[pnode_indx].minx1 = tn->minx1;
	pnodes[pnode_indx].maxx1 = tn->maxx1;
	pnodes[pnode_indx].maxy2 = tn->maxy2;
	pnodes[pnode_indx].miny2 = tn->miny2;
	pnodes[pnode_indx].minx2 = tn->minx2;
	pnodes[pnode_indx].maxx2 = tn->maxx2;
	pnodes[pnode_indx].chright = tn->chright;
	pnodes[pnode_indx].chleft = tn->chleft;
	pnode_indx++;
	tn->node_num = num_pnodes++;
}

/* Add a lump to current level
   by Lee Killough */

static void add_lump(const char *name, void *data, size_t length)
{
 struct lumplist *l;
 struct directory *dir;

 l = current_level;
 while (l->next)
   l=l->next;

 l->next = GetMemory(sizeof(*l));
 l = l->next;
 l->dir = dir = GetMemory(sizeof(struct directory));
 strncpy(dir->name,name,8);
 dir->length=length;
 l->islevel=0;
 l->data=data;
 l->next=NULL;
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


/*- Main Program -----------------------------------------------------------*/

int main(int argc,char *argv[])
{
#if 0
	long dir_start = 12;		/* Skip Pwad header*/
	long dir_entries = 0;
	unsigned char *data;
#endif

        struct lumplist *lump,*l;
        struct directory *newdirec;

        setbuf(stdout,NULL);

	printf("* Doom BSP node builder ver 2.0 (c) 1997 Colin Reed (creed@graymatter.on.ca) *\n");

        if (argc>=2 && !strcmp(argv[1],"-factor") && (argc-=2))
         {
          char *end;
          long f;
          f=strtol(*(argv+=2),&end,0);
          factor=f>0 && !*end ? f : 0;
         }

	if(argc<2 || argc>3 || factor<=0)
		{
		printf("\nThis Node builder was created from the basic theory stated in DEU5 (OBJECTS.C)\n"
		       "\nCredits should go to :-\n"
		       "Matt Fell      (msfell@aol.com) for the Doom Specs.\n"
		       "Raphael Quinet (Raphael.Quinet@eed.ericsson.se) for DEU and the original idea.\n"
		       "Mark Harrison  (harrison@lclark.edu) for finding a bug in 1.1x\n"
                       "Lee Killough   (killough@rsn.hp.com) for tuning speed and multilevel support\n"
		       "\nUsage: BSP [-factor <nnn>] input.wad [<output.wad>]\n"
		       "     : (If no output.wad is specified, TMP.WAD is written)\n"
		       );
		exit(1);
		}

	testwad = argv[1];									/* Get input name*/
	if(argc == 3) outwad = argv[2];					/* Get output name*/
	else outwad = "tmp.wad";

	OpenWadFile(testwad);						/* Opens and reads directory*/

     if((outfile = fopen(outwad,"wb")) == NULL)
 	 {
          fputs("Error: Could not open output PWAD file ",stderr);
	  perror(outwad);
	  exit(1);
	 }

        printf("Creating nodes using tunable factor of %d\n",factor);

        for (lump=lumplist; lump; lump=lump->next)
         if (lump->islevel)
           {
            struct Seg *tsegs;
            static struct Node *nodelist;

            current_level=lump->data;
            printf("\nBuilding nodes on %-.8s\n\n",lump->dir->name);

	blockptrs=NULL;
        blocklists=NULL;
        blockptrs_size=0;
        num_ssectors=0;
        num_psegs=0;
	num_nodes = 0;

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

	nodelist = CreateNode(tsegs);						/* recursively create nodes*/
	printf("%lu NODES created, with %lu SSECTORS.\n",num_nodes,num_ssectors);

	pnodes = GetMemory(sizeof(struct Pnode)*num_nodes);
	num_pnodes = 0;
	pnode_indx = 0;
	ReverseNodes(nodelist);

	printf("Found %lu used vertices\n",num_verts);

        vertlmp->dir->length=num_verts * sizeof(struct Vertex);
        vertlmp->data=vertices;

        add_lump("SEGS", psegs, sizeof(struct Pseg)*num_psegs);

        add_lump("SSECTORS", ssectors, sizeof(struct SSector)*num_ssectors);

        add_lump("NODES", pnodes, sizeof(struct Pnode)*num_pnodes);

        {
 	 long reject_size = (num_sects*num_sects+7)/8;
         void *data = GetMemory(reject_size);
         memset(data,0,reject_size);
         add_lump("REJECT", data, reject_size);
        }

        {
 	 long blockmap_size = CreateBlockmap();
         char *data=GetMemory(blockmap_size+blockptrs_size+8);
         memcpy(data,&blockhead,8);
         memcpy(data+8,blockptrs,blockptrs_size);
         memcpy(data+8+blockptrs_size,blocklists,blockmap_size);
         free(blockptrs);
         free(blocklists);
         add_lump("BLOCKMAP",data,blockmap_size+blockptrs_size+8);
        }
       puts("Completed blockmap building");

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
     wad.dir_start=offs;
     newdirec = GetMemory(wad.num_entries*sizeof(struct directory));
     fwrite(&wad,12,1,outfile);
     for (offs=0,lump=lumplist; lump; lump=lump->next)
      {
       if (ftell(outfile)!=lump->dir->start)
         ProgError("File position consistency check failure writing %-.8s", lump->dir->name);
       if (lump->dir->length)
          fwrite(lump->data, lump->dir->length, 1, outfile);
       else
          lump->dir->start=0;
       newdirec[offs++]=*lump->dir;
       if (lump->islevel)
         for (l=lump->data; l; l=l->next)
          {
           if (ftell(outfile)!=l->dir->start)
             ProgError("File position consistency check failure writing %-.8s", l->dir->name);
           if (l->dir->length)
             fwrite(l->data, l->dir->length, 1, outfile);
           else
             l->dir->start=0;
           newdirec[offs++]=*l->dir;
          }
      }
     if (ftell(outfile)!=wad.dir_start)
       ProgError("File position consistency check failure writing %-.8s", "lump dir");
     fwrite(newdirec,sizeof(struct directory)*wad.num_entries,1,outfile);
     fclose(outfile);
     if (offs!=wad.num_entries)
       ProgError("Lump directory count consistency check failure");
     printf("\nSaved PWAD as %s\n",outwad);
     exit(0);
   }
}

/*- end of file ------------------------------------------------------------*/
