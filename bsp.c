/*- BSP.C -------------------------------------------------------------------*

 Node builder for DOOM levels (c) 1996 Colin Reed, version 1.5 (dos extended)

 Performance increased 200% over 1.2x

 Many thanks to Mark Harrison for finding a bug in 1.1 which caused some
 texture align problems when a flipped SEG was split.

 Credit to:-

 Raphael Quinet (A very small amount of code has been borrowed from DEU).
 Matt Fell for the doom specs.
 Lee Killough for performance tuning.

 Also, the original idea for some of the techniques where also taken from the
 comment at the bottom of OBJECTS.C in DEU, and the doc by Matt Fell about
 the nodes.

 Use this code for your own editors, but please credit me.

*---------------------------------------------------------------------------*/

#include "bsp.h"
#include "structs.h"

/*- Global Vars ------------------------------------------------------------*/

static FILE *infile,*outfile;
static char *testwad;
static char *outwad;

static struct wad_header *wad = NULL;
static struct directory *direc = NULL;

static struct Thing *things;
static long num_things = 0;
static long things_start = 0;

static struct Vertex *vertices;
static long num_verts = 0;
static long vertices_start = 0;

static struct LineDef *linedefs;
static long num_lines = 0;
static long linedefs_start = 0;

static struct SideDef *sidedefs;
static long num_sides = 0;
static long sidedefs_start = 0;

static struct Sector *sectors;
static long num_sects = 0;
static long sectors_start = 0;

static struct SSector *ssectors;
static long num_ssectors = 0;
static long ssectors_start = 0;

static struct Pseg *psegs = NULL;
static long num_psegs = 0;
static long segs_start = 0;

static struct Pnode *pnodes = NULL;
static long num_pnodes = 0;
static long pnode_indx = 0;
static long pnodes_start = 0;

static struct Seg *tsegs = NULL;
static long num_tsegs = 0;

static struct Node *nodelist = NULL;
static long num_nodes = 0;

static long reject_start;
static long reject_size;

static struct Block blockhead;
static short int *blockptrs;
static short int *blocklists=NULL;
static long blockptrs_size;
static long blockmap_size;
static long blockmap_start;

static struct splitter sp;

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

/*- Prototypes -------------------------------------------------------------*/

static __inline__ void OpenWadFile(char *);
static __inline__ int  FindDir(char *);
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

	printf("Creating Segs ..........\n");

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
			num_tsegs++;
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
			num_tsegs++;
			}
		}

	return fs;
}

/*- get the directory from a wad file --------------------------------------*/

static __inline__ void OpenWadFile(char *filename)
{
	struct directory *dir;

	long n;

	if((infile = fopen(filename,"rb")) == NULL)
		{
                printf("Error: Cannot find WAD file %s", filename);
		exit(1);
		}

	wad = GetMemory( sizeof( struct wad_header));

	fread(wad,sizeof( struct wad_header),1,infile);

	printf("Opened %c%c%c%c file : %s. %lu dir entries at %lu.\n",
		wad->type[0],wad->type[1],wad->type[2],wad->type[3],filename,
		wad->num_entries,wad->dir_start);

	direc = dir = GetMemory( sizeof( struct directory) * wad->num_entries);

	fseek(infile,wad->dir_start,0);

	for(n = 0; n < wad->num_entries; n++)
		{
		fread(dir,sizeof( struct directory),1,infile);
/*		Printname(dir);
		printf(" of size %lu at %lu.\n",dir->length,dir->start);	*/
		dir++;
		}
}

/*- find the offset into the directory of a resource -----------------------*/

static __inline__ int FindDir(char *name)
{
	struct directory *dir;
	int n;

	dir = direc;

	for(n=0; n< wad->num_entries; n++)
  	  if(strncmp(dir++->name,name,8) == 0) return n;
	ProgError( "Cannot find %s", name);
	return 0;
}

/*- read the things from the wad file and place in 'things' ----------------*/

static __inline__ void GetThings(void)
{
	int n;

	n = FindDir("THINGS");
	if(direc[n].length == 0) ProgError("Must have at least 1 thing");
	num_things = (direc[n].length) / (sizeof( struct Thing));
/*	printf("Allocating %lu bytes for %d things\n",direc[n].length,num_things);*/
	things = GetMemory( direc[n].length);
	fseek(infile,direc[n].start,0);
	fread(things,direc[n].length,1,infile);
}

/*- read the vertices from the wad file and place in 'vertices' ------------
    Rewritten by Lee Killough, to speed up performance                       */

static __inline__ void GetVertexes(void)
{
	long n,used_verts,i;
        int *translate;

	n = FindDir("VERTEXES");

	if(direc[n].length == 0)
	  ProgError("Couldn't find any Vertices");

	fseek(infile,direc[n].start,0);

	num_verts = (direc[n].length) / (sizeof( struct Vertex));

	vertices = GetMemory(sizeof(struct Vertex)*num_verts);
        translate = GetMemory(num_verts*sizeof(*translate));

	fread(vertices,direc[n].length,1,infile);

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

	printf("Loaded %ld vertices, but %ld were unused.\n%ld zero-length lines were removed.\n",
	       num_verts,num_verts-used_verts,i);
	num_verts = used_verts;
}

/*- read the linedefs from the wad file and place in 'linedefs' ------------*/

static __inline__ void GetLinedefs(void)
{
	int n;

	n = FindDir("LINEDEFS");
	if(direc[n].length == 0) ProgError("Couldn't find any Linedefs");
	num_lines = (direc[n].length) / (sizeof( struct LineDef));
/*	printf("Allocating %lu bytes for %d Linedefs\n",direc[n].length,num_lines);*/
	linedefs = GetMemory( direc[n].length);
	fseek(infile,direc[n].start,0);
	fread(linedefs,direc[n].length,1,infile);
}

/*- read the sidedefs from the wad file and place in 'sidedefs' ------------*/

static __inline__ void GetSidedefs(void)
{
	int n;

	n = FindDir("SIDEDEFS");
	if(direc[n].length == 0) ProgError("Couldn't find any Sidedefs");
	num_sides = (direc[n].length) / (sizeof( struct SideDef));
/*	printf("Allocating %lu bytes for %d Sidedefs\n",direc[n].length,num_sides);*/
	sidedefs = GetMemory( direc[n].length);
	fseek(infile,direc[n].start,0);
	fread(sidedefs,direc[n].length,1,infile);
}

/*- read the sectors from the wad file and place in 'sectors' ------------*/

static __inline__ void GetSectors(void)
{
	int n;

	n = FindDir("SECTORS");
	num_sects = (direc[n].length) / (sizeof( struct Sector));
/*	printf("Allocating %lu bytes for %d Sectors\n",direc[n].length,num_sects);*/
	sectors = GetMemory( direc[n].length);
	fseek(infile,direc[n].start,0);
	fread(sectors,direc[n].length,1,infile);
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

/*- Main Program -----------------------------------------------------------*/

int main(int argc,char *argv[])
{
	long dir_start = 12;									/* Skip Pwad header*/
	long dir_entries = 0;

	unsigned char *data;

	printf("* Doom BSP node builder ver 1.5x (c) 1996 Colin Reed (creed@graymatter.on.ca) *\n");
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
                       "Lee Killough   (killough@convex.com) for performance tuning\n"
		       "\nUsage: BSP [-factor nnn] name.wad {output.wad}\n"
		       "     : (If no output.wad is specified, TMP.WAD is written)\n"
		       );
		exit(0);
		}

	testwad = argv[1];									/* Get input name*/
	if(argc == 3) outwad = argv[2];					/* Get output name*/
	else outwad = "tmp.wad";

	OpenWadFile(testwad);								/* Opens and reads directory*/
	GetThings();											/* of wad file*/
	GetLinedefs();											/* Get linedefs and vertices*/
	GetVertexes();											/* and delete redundant.*/
	GetSidedefs();
 	GetSectors();

	num_tsegs = 0;
	tsegs = CreateSegs();		  						/* Initially create segs*/

	FindLimits(tsegs);									/* Find limits of vertices*/

	mapminx = lminx;										/* store as map limits*/
	mapmaxx = lmaxx;
	mapminy = lminy;
	mapmaxy = lmaxy;

	printf("Map goes from (%d,%d) to (%d,%d)\n",lminx,lminy,lmaxx,lmaxy);

        printf("Creating nodes using tunable factor of %d\n",factor);

	num_nodes = 0;
	nodelist = CreateNode(tsegs);						/* recursively create nodes*/
	printf("%lu NODES created, with %lu SSECTORS.\n",num_nodes,num_ssectors);

	pnodes = GetMemory(sizeof(struct Pnode)*num_nodes);
	num_pnodes = 0;
	pnode_indx = 0;
	ReverseNodes(nodelist);

	dir_entries++;											/* Skip level number*/

	things_start = dir_start;
	dir_start = dir_start + (sizeof(struct Thing)*num_things);
	direc[dir_entries].start = things_start;
	direc[dir_entries].length = (sizeof(struct Thing)*num_things);
	dir_entries++;

	linedefs_start = dir_start;
	dir_start = dir_start + (sizeof(struct LineDef)*num_lines);
	direc[dir_entries].start = linedefs_start;
	direc[dir_entries].length = (sizeof(struct LineDef)*num_lines);
	dir_entries++;

	sidedefs_start = dir_start;
	dir_start = dir_start + (sizeof(struct SideDef)*num_sides);
	direc[dir_entries].start = sidedefs_start;
	direc[dir_entries].length = (sizeof(struct SideDef)*num_sides);
	dir_entries++;

	printf("Found %lu used vertices\n",num_verts);

	vertices_start = dir_start;
	dir_start = dir_start + (sizeof(struct Vertex)*num_verts);
	direc[dir_entries].start = vertices_start;
	direc[dir_entries].length = (sizeof(struct Vertex)*num_verts);
	dir_entries++;

	segs_start = dir_start;
	dir_start = dir_start + (sizeof(struct Pseg)*num_psegs);
	direc[dir_entries].start = segs_start;
	direc[dir_entries].length = (sizeof(struct Pseg)*num_psegs);
	dir_entries++;

	ssectors_start = dir_start;
	dir_start = dir_start + (sizeof(struct SSector)*num_ssectors);
	direc[dir_entries].start = ssectors_start;
	direc[dir_entries].length = (sizeof(struct SSector)*num_ssectors);
	dir_entries++;

	pnodes_start = dir_start;
	dir_start = dir_start + (sizeof(struct Pnode)*num_pnodes);
	direc[dir_entries].start = pnodes_start;
	direc[dir_entries].length = (sizeof(struct Pnode)*num_pnodes);
	dir_entries++;

	sectors_start = dir_start;
	dir_start = dir_start + (sizeof(struct Sector)*num_sects);
	direc[dir_entries].start = sectors_start;
	direc[dir_entries].length = (sizeof(struct Sector)*num_sects);
	dir_entries++;

	reject_size = (num_sects*num_sects+7)/8;
	data = calloc(reject_size,1);
	reject_start = dir_start;
	dir_start+=reject_size;
	direc[dir_entries].start = reject_start;			/* Skip reject map*/
	direc[dir_entries].length = reject_size;
	dir_entries++;

	blockmap_size = CreateBlockmap();

	blockmap_start = dir_start;
	dir_start = dir_start + (blockmap_size+blockptrs_size+8);
	direc[dir_entries].start = blockmap_start;
	direc[dir_entries].length = (blockmap_size+blockptrs_size+8);
	dir_entries++;

	printf("Completed blockmap building and saved PWAD as %s\n",outwad);

	if((outfile = fopen(outwad,"wb")) == NULL)
		{
      printf("Error: Could not open output PWAD file %s", outwad);
		exit(0);
		}
	fwrite(wad,4,1,outfile);
	fwrite(&dir_entries,sizeof(long),1,outfile);
	fwrite(&dir_start,sizeof(long),1,outfile);
	fwrite(things,(sizeof(struct Thing)*num_things),1,outfile);
	fwrite(linedefs,(sizeof(struct LineDef)*num_lines),1,outfile);
	fwrite(sidedefs,(sizeof(struct SideDef)*num_sides),1,outfile);
	fwrite(vertices,(sizeof(struct Vertex)*num_verts),1,outfile);
	fwrite(psegs,(sizeof(struct Pseg)*num_psegs),1,outfile);
	fwrite(ssectors,(sizeof(struct SSector)*num_ssectors),1,outfile);
	fwrite(pnodes,(sizeof(struct Pnode)*num_pnodes),1,outfile);
	fwrite(sectors,(sizeof(struct Sector)*num_sects),1,outfile);
	fwrite(data,reject_size,1,outfile);
	fwrite(&blockhead,(sizeof(struct Block)),1,outfile);
	fwrite(blockptrs,blockptrs_size,1,outfile);
	fwrite(blocklists,blockmap_size,1,outfile);
	fwrite(direc,(sizeof(struct directory)*wad->num_entries),1,outfile);
	fclose(outfile);

	exit(0);
}

/*- end of file ------------------------------------------------------------*/
