/*- BSP.C -------------------------------------------------------------------*

 Node builder for DOOM levels (c) 1994 Colin Reed, version 1.2 (dos extended)

 Many thanks to Mark Harrison for finding a bug in 1.1 which caused some
 texture align problems when a flipped SEG was split.

 Credit to:-

 Raphael Quinet (A very small amount of code has been borrowed from DEU).
 Matt Fell for the doom specs.

 Also, the original idea for some of the techniques where also taken from the
 comment at the bottom of OBJECTS.C in DEU, and the doc by Matt Fell about
 the nodes.

 Use this code for your own editors, but please credit me.

*---------------------------------------------------------------------------*/

#include "bsp.h"
#include "structs.h"

/*- Global Vars ------------------------------------------------------------*/

FILE *infile,*outfile;
char *testwad;
char *outwad;

struct wad_header *wad = NULL;
struct directory *direc = NULL;
char rname[]="\0\0\0\0\0\0\0\0";

struct Thing *things;
long num_things = 0;
long things_start = 0;

struct Vertex *vertices;
long num_verts = 0;
long vertices_start = 0;

struct LineDef *linedefs;
long num_lines = 0;
long linedefs_start = 0;

struct SideDef *sidedefs;
long num_sides = 0;
long sidedefs_start = 0;

struct Sector *sectors;
long num_sects = 0;
long sectors_start = 0;

struct SSector *ssectors;
long num_ssectors = 0;
long ssectors_start = 0;

struct Pseg *psegs = NULL;
long num_psegs = 0;
long segs_start = 0;

struct Pnode *pnodes = NULL;
long num_pnodes = 0;
long pnode_indx = 0;
long pnodes_start = 0;

struct Seg *tsegs = NULL;
long num_tsegs = 0;

struct Node *nodelist = NULL;
long num_nodes = 0;

long reject_start;
long reject_size;

struct Block blockhead;
short int *blockptrs;
short int *blocklists=NULL;
long blockptrs_size;
long blockmap_size;
long blockmap_start;

struct splitter sp;

short node_x;
short node_y;
short node_dx;
short node_dy;

short lminx;
short lmaxx;
short lminy;
short lmaxy;

short mapminx;
short mapmaxx;
short mapminy;
short mapmaxy;

long psx,psy,pex,pey,pdx,pdy;
long lsx,lsy,lex,ley;

unsigned char pcnt;

long fagcount = 0;

/*- Prototypes -------------------------------------------------------------*/

void OpenWadFile(char *);
void Printname(struct directory *);
int  FindDir(char *);
void GetThings(void);
void GetVertexes(void);
void GetLinedefs(void);
void GetSidedefs(void);
void GetSectors(void);
void FindLimits(struct Seg *);

struct Seg *CreateSegs();

struct Node *CreateNode(struct Seg *);
void DivideSegs(struct Seg *,struct Seg **,struct Seg **);
int IsItConvex(struct Seg *);

struct Seg *PickNode(struct Seg *);
void ComputeIntersection(short int *,short int *);
int DoLinesIntersect();
int SplitDist(struct Seg *);

void ReverseNodes(struct Node *);
long CreateBlockmap(void);

inline void progress(void);

/*--------------------------------------------------------------------------*/

#include "makenode.c"
#include "picknode.c"
#include "funcs.c"

/*- Main Program -----------------------------------------------------------*/

int main(int argc,char *argv[])
{
	long dir_start = 12;									/* Skip Pwad header*/
	long dir_entries = 0;
	
	int n;
	unsigned char *data;
	
	printf("** Doom BSP node builder ver 1.2x (c) 1994 Colin Reed **\n");

	if(argc<2 || argc>3)
		{
		printf("\nThis Node builder was created from the basic theory stated in DEU5 (OBJECTS.C)\n");
		printf("\nCredits should go to :-\n");
		printf("Matt Fell      (matt.burnett@acebbs.com) for the Doom Specs.\n");
		printf("Raphael Quinet (quinet@montefiore.ulg.ac.be) for DEU and the original idea.\n");
		printf("Mark Harrison  (harrison@lclark.edu) for finding a bug in 1.1x\n");
		printf("\nUsage: BSP name.wad {output.wad}\n");
		printf("     : (If no output.wad is specified, TMP.WAD is written)\n");
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
	
	printf("Map goes from X (%d,%d) Y (%d,%d)\n",lminx,lmaxx,lminy,lmaxy);
	
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

	return 0;
}

/*- initially creates all segs, one for each line def ----------------------*/

struct Seg *CreateSegs()
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

/*--------------------------------------------------------------------------*/
/* Find limits from a list of segs, does this by stepping through the segs*/
/* and comparing the vertices at both ends.*/
/*--------------------------------------------------------------------------*/

void FindLimits(struct Seg *ts)
{
	int n,minx,miny,maxx,maxy;
	int fv,tv;

	minx = 32767;
	maxx = -32767;
	miny = 32767;
	maxy = -32767;

	while(1)
		{
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

int SplitDist(struct Seg *ts)
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

/*- get the directory from a wad file --------------------------------------*/

void OpenWadFile(char *filename)
{
	struct directory *dir;

	long n;

	if((infile = fopen(filename,"rb")) == NULL)
		{
      printf("Error: Cannot find WAD file %s", filename);
		exit(0);
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

/*- read the things from the wad file and place in 'things' ----------------*/

void GetThings(void)
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

/*- read the vertices from the wad file and place in 'vertices' ------------*/

void GetVertexes(void)
{
	struct Vertex *tmpv;
	int n,i,t;
	long used_verts;

	n = FindDir("VERTEXES");
	if(direc[n].length == 0) ProgError("Couldn't find any Vertices");
	fseek(infile,direc[n].start,0);
	num_verts = (direc[n].length) / (sizeof( struct Vertex));
	tmpv = GetMemory(sizeof(struct Vertex)*num_verts);
	vertices = GetMemory(sizeof(struct Vertex)*num_verts);
	fread(tmpv,direc[n].length,1,infile);

	used_verts = 0;
	for(i=0;i<num_verts;i++)
		{
		if(Reference(i))
			{
			vertices[used_verts].x = tmpv[i].x;
			vertices[used_verts].y = tmpv[i].y;
			
			for(t=0; t<num_lines; t++)
				{
				if(linedefs[t].start == i) linedefs[t].start = used_verts;
				if(linedefs[t].end == i) linedefs[t].end = used_verts;
				}
			used_verts++;
			}
		else
			{
/*			printf("Vertex [%d] not used.\n",i);            */
			}
		}
	printf("Loaded %lu vertices, but %lu were unused.\n",num_verts,num_verts-used_verts);
	num_verts = used_verts;
	free(tmpv);
}

/*--------------------------------------------------------------------------*/

int Reference(int vert_num)
{
	int n;

	for(n=0; n<num_lines; n++)
		{
		if(linedefs[n].start == vert_num || linedefs[n].end == vert_num) return 1;
		}
	return 0;
}

/*- read the linedefs from the wad file and place in 'linedefs' ------------*/

void GetLinedefs(void)
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

void GetSidedefs(void)
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

void GetSectors(void)
{
	int n;

	n = FindDir("SECTORS");
	num_sects = (direc[n].length) / (sizeof( struct Sector));
/*	printf("Allocating %lu bytes for %d Sectors\n",direc[n].length,num_sects);*/
	sectors = GetMemory( direc[n].length);
	fseek(infile,direc[n].start,0);
	fread(sectors,direc[n].length,1,infile);
}

/*- find the offset into the directory of a resource -----------------------*/

int FindDir(char *name)
{
	struct directory *dir;
	int n;

	dir = direc;

	for(n=0; n< wad->num_entries; n++)
		{
		strncpy(rname,dir->name,8);
		if(strcmp(rname,name) == 0) return n;
		dir++;
		}
	ProgError( "Cannot find %s", name);
	return 0;
}

/*- print a resource name by copying 8 chars into rname and printing this --*/

void Printname(struct directory *dir)
{
	strncpy(rname,dir->name,8);
	printf("%-8s",rname);
}

/*- Create blockmap --------------------------------------------------------*/

long CreateBlockmap()
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

int IsLineDefInside(int ldnum, int xmin, int ymin, int xmax, int ymax )
{
   int x1 = vertices[ linedefs[ ldnum].start].x;
   int y1 = vertices[ linedefs[ ldnum].start].y;
   int x2 = vertices[ linedefs[ ldnum].end].x;
   int y2 = vertices[ linedefs[ ldnum].end].y;
	
	int outcode1,outcode2,t;

	while(1)
		{
		outcode1=0;
		if(y1>ymax) outcode1|=1;
		if(y1<ymin) outcode1|=2;
		if(x1>xmax) outcode1|=4;
		if(x1<xmin) outcode1|=8;
		outcode2=0;
		if(y2>ymax) outcode2|=1;
		if(y2<ymin) outcode2|=2;
		if(x2>xmax) outcode2|=4;
		if(x2<xmin) outcode2|=8;
		if((outcode1&outcode2)!=0) return FALSE;
		if(((outcode1==0)&&(outcode2==0))) return TRUE;

		if(!(outcode1&15))
			{
			t=outcode1;
			outcode1=outcode2;
			outcode2=t;
			t=x1;x1=x2;x2=t;
			t=y1;y1=y2;y2=t;
			}
		if(outcode1&1)
			{
			x1=x1+(x2-x1)*(ymax-y1)/(y2-y1);
			y1=ymax;
			}
		else
			{
			if(outcode1&2)
				{
				x1=x1+(x2-x1)*(ymin-y1)/(y2-y1);
				y1=ymin;
				}
			else
				{
				if(outcode1&4)
					{
					y1=y1+(y2-y1)*(xmax-x1)/(x2-x1);
					x1=xmax;
					}
				else
					{
					if(outcode1&8)
						{
						y1=y1+(y2-y1)*(xmin-x1)/(x2-x1);
						x1=xmin;
						}
					}
				}
			}
		}
}

/*--------------------------------------------------------------------------*/

void ReverseNodes(struct Node *tn)
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

/*--------------------------------------------------------------------------*/

inline void progress()
{
	char *s="/-\\|/-\\|";

	if((pcnt&15) == 0)
		{

		printf("%c\b",s[((pcnt)/16)&7]);
		fflush(stdout);

		}
	pcnt++;

}

/*- end of file ------------------------------------------------------------*/
