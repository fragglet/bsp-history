/*- BSP.C -------------------------------------------------------------------*

 Node builder for DOOM levels, still only version 0.9, so not totally finished
 but, seems to be doing pretty well, for about a weeks work!
 Credit to Raphael Quinet (Some of the code has been modified from DEU).
 Also, the original idea for some of the techniques where also taken from the
 comment at the bottom of OBJECTS.C in DEU, and the doc by Matt Fell about
 the nodes.

 Also to Dylan Cuthbert, cos I am really shit at C, and maths.
 And Giles Goddard for helping with the which side checker.

 Any inquiries to Colin Reed via DYL@CIX.COMPULINK.CO.UK

 Use this code for your own editors, but please credit me.

*---------------------------------------------------------------------------*/

#include "bsp.h"
#include "structs.h"
#include <math.h>

/*- Global Vars ------------------------------------------------------------*/

FILE *infile,*outfile;
char *testwad;

struct wad_header *wad = NULL;
struct directory *direc = NULL;
char rname[]="\0\0\0\0\0\0\0\0";

struct Thing *things;
int num_things = 0;
long things_start = 0;

struct Vertex *vertices;
int num_verts = 0;
long vertices_start = 0;

struct LineDef *linedefs;
int num_lines = 0;
long linedefs_start = 0;

struct SideDef *sidedefs;
int num_sides = 0;
long sidedefs_start = 0;

struct Sector *sectors;
int num_sects = 0;
long sectors_start = 0;

struct SSector *ssectors;
int num_ssectors = 0;
long ssectors_start = 0;

struct Pseg *psegs = NULL;
int num_psegs = 0;
long segs_start = 0;

struct Pnode *pnodes = NULL;
int num_pnodes = 0;
int pnode_indx = 0;
long pnodes_start = 0;

struct Seg *tsegs = NULL;
int num_tsegs = 0;

struct Node *nodelist = NULL;
int num_nodes = 0;

long reject_start;
long reject_size;

struct Block blockhead;
int *blockptrs;
int *blocklists=NULL;
long blockptrs_size;
int blockmap_size;
long blockmap_start;

int node_x;
int node_y;
int node_dx;
int node_dy;

int lminx;
int lmaxx;
int lminy;
int lmaxy;

int mapminx;
int mapmaxx;
int mapminy;
int mapmaxy;

unsigned char pcnt;

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
int CheckSplits(struct Seg *,struct Seg *);
int NearerMiddle(struct splitter *,struct Seg *);
void ComputeIntersection(struct Seg *,struct Seg *,int *,int *);
int DoLinesIntersect(struct Seg *,struct Seg *);
int WhichSideSeg(struct Seg *,struct Seg *);
int SplitDist(struct Seg *);
int LineSplit(struct Seg *,struct Seg *);
int AngleThreshold(int);
struct Seg *AddtoRight(struct Seg *);
struct Seg *AddtoLeft(struct Seg *);
void ReverseNodes(struct Node *);
long CreateBlockmap(void);
int Clipper(int,int,int,int,int,int,int,int);
void progress(void);

/*- Main Program -----------------------------------------------------------*/

int main(int argc,char *argv[])
{
	long dir_start = 12;			// Skip Pwad header
	long dir_entries = 0;
	
	int n;
	unsigned char *data;
	
	printf("Doom bsp node builder ver 0.9 (c) Colin Reed\n");

	if(argc!=2) ProgError("Wad files needs to be specified");

	testwad = argv[1];

	OpenWadFile(testwad);		// Opens and reads directory of wad file
	GetThings();
	GetVertexes();
	GetLinedefs();
	GetSidedefs();
	GetSectors();

	num_tsegs = 0;
	tsegs = CreateSegs();		  				// Initially create segs

	FindLimits(tsegs);							// Find limits from list of vertexes
	
	mapminx = lminx;
	mapmaxx = lmaxx;
	mapminy = lminy;
	mapmaxy = lmaxy;
	
	printf("Map goes from X (%d,%d) Y (%d,%d) ratio is %d:%d\n",
		lminx,lmaxx,lminy,lmaxy,(lmaxx-lminx),(lmaxy-lminy));
	
	num_nodes = 0;
	
	nodelist = CreateNode(tsegs);						// recursively create nodes

	printf("%d NODES created, with %d SSECTORS.\n",num_nodes,num_ssectors);

	pnodes = GetMemory(sizeof(struct Pnode)*num_nodes);
	num_pnodes = 0;
	pnode_indx = 0;
	ReverseNodes(nodelist);

	dir_entries++;											// Skip level number

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
	direc[dir_entries].start = reject_start;			// Skip reject map
	direc[dir_entries].length = reject_size;
	dir_entries++;

	blockmap_size = CreateBlockmap();

	blockmap_start = dir_start;
	dir_start = dir_start + (blockmap_size+blockptrs_size+8);
	direc[dir_entries].start = blockmap_start;
	direc[dir_entries].length = (blockmap_size+blockptrs_size+8);
	dir_entries++;

	outfile = fopen("tmp.wad","wb");
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
// Recursively create nodes and return the pointers.
/*--------------------------------------------------------------------------*/

struct Node *CreateNode(struct Seg *ts)
{
	struct Node *tn;
	struct Seg *rights = NULL;
	struct Seg *lefts = NULL;

	tn = GetMemory( sizeof( struct Node));			// Create a node
 
	DivideSegs(ts,&rights,&lefts);

	num_nodes++;

	tn->x = node_x;
	tn->y = node_y;
	tn->dx = node_dx;
	tn->dy = node_dy;

	FindLimits(lefts);							// Find limits from list of vertexes
	
	tn->maxy2 = lmaxy;
	tn->miny2 = lminy;
	tn->minx2 = lminx;
	tn->maxx2 = lmaxx;

	if(NeedFurtherDivision(lefts))												// Check lefthand side
		{
		tn->nextl = CreateNode(lefts);				// still segs remaining
		tn->chleft = 0;
		}
	else
		{
		tn->nextl = NULL;
		tn->chleft = CreateSSector(lefts) | 0x8000;
		}

	FindLimits(rights);							// Find limits from list of vertexes
	
	tn->maxy1 = lmaxy;
	tn->miny1 = lminy;
	tn->minx1 = lminx;
	tn->maxx1 = lmaxx;

	if(NeedFurtherDivision(rights))												// Check righthand side
		{
		tn->nextr = CreateNode(rights);				// still segs remaining
		tn->chright = 0;
		}
	else
		{
		tn->nextr = NULL;
		tn->chright =  CreateSSector(rights) | 0x8000;
		}

	return tn;
}

/*--------------------------------------------------------------------------*/

int CreateSSector(struct Seg *tmps)
{
	int n;
	
	if(num_ssectors == 0)
		{
		ssectors = GetMemory(sizeof(struct SSector));
		}
	else
		{
		ssectors = ResizeMemory(ssectors,sizeof(struct SSector)*(num_ssectors+1));
		}
	
	ssectors[num_ssectors].first = num_psegs;

	n = num_psegs;
	
//	printf("\n");

	for(;tmps;tmps=tmps->next)
		{
		if(num_psegs == 0)
			{
			psegs = GetMemory(sizeof(struct Pseg));
			}
		else
			{
			psegs = ResizeMemory(psegs,sizeof(struct Pseg)*(num_psegs+1));
			}

		psegs[num_psegs].start = tmps->start;
		psegs[num_psegs].end = tmps->end;
		psegs[num_psegs].angle = tmps->angle;
		psegs[num_psegs].linedef = tmps->linedef;
		psegs[num_psegs].flip = tmps->flip;
		psegs[num_psegs].dist = tmps->dist;
//		printf("%d,%d,%u,%d,%d,%u\n",
//			psegs[num_psegs].start,
//			psegs[num_psegs].end,
//			psegs[num_psegs].angle,
//			psegs[num_psegs].linedef,
//			psegs[num_psegs].flip,
//			psegs[num_psegs].dist);
		num_psegs++;
		}
	
	ssectors[num_ssectors].num = num_psegs-n;
	
	num_ssectors++;
	
	return num_ssectors-1;
}

/*--------------------------------------------------------------------------*/

int NeedFurtherDivision( struct Seg *ts)
{
   struct Seg *line,*check;
   int   sector,val;

	if (ts->flip) sector = sidedefs[ linedefs[ ts->linedef].sidedef2].sector;
   else sector = sidedefs[ linedefs[ ts->linedef].sidedef1].sector;
   
	for (line = ts->next; line; line = line->next)
   	{
      if (line->flip)
      	{
			if (sidedefs[ linedefs[ line->linedef].sidedef2].sector != sector)
				return TRUE;
      	}
      else
      	{
			if (sidedefs[ linedefs[ line->linedef].sidedef1].sector != sector)
	    		return TRUE;
			}
   	}
   
	/* all of the segs must be on the same side all the other segs */

	for(line=ts;line;line=line->next)
		{
		for(check=ts;check;check=check->next)
			{
			if(line!=check)
				{
				val = DoLinesIntersect(line,check);
				if(val&2 || val&32)
					{
					return TRUE;
					}
				}
			}
		}

	/* no need to split the list: these Segs can be put in a SSector */
   return FALSE;
}

/*---------------------------------------------------------------------------*
 Split a list of segs (ts) into two.
 This is done by scanning all of the segs and finding the one that does
 the least splitting. Also tries the one that is nearest the middle of the
 Node.
 If the ones on the left side make a SSector, then create another SSector
 and make (lefts = NULL) else put the segs into lefts list.
 If the ones on the right side make a SSector, then create another SSector
 and make (rights = NULL) else put the segs into rights list.
*---------------------------------------------------------------------------*/

void DivideSegs(struct Seg *ts,struct Seg **rs,struct Seg **ls)
{
	struct Seg *rights,*lefts;
	struct Seg *tmps,*best,*news,*prev;
	struct Seg *add_to_rs,*add_to_ls;
  	struct Seg *new_best,*new_rs,*new_ls;
	
	struct Seg *strights,*stlefts;
	struct splitter sp;

	int num_secs_r,num_secs_l,last_sec_r,last_sec_l;

	int num,least_splits,least;
	int fv,tv,dist,splits,min_dist = 0,num_new = 0;
	int bangle,cangle,cangle2,cfv,ctv,dx,dy;
	int x,y,val;

/* Go through the list of segs and try each one with all the other segs to
	find the one that does the least splitting.
	I'm gonna put in a bit to check whether the line it is splitting is really
	angled and if so, don't choose this one. */

	FindLimits(ts);							// Find limits of this set of Segs
	
	sp.halfsx = (lmaxx - lminx) / 2;		// Find half width of Node
	sp.halfsy = (lmaxy - lminy) / 2;
	sp.halfx = lminx + sp.halfsx;			// Find middle of Node
	sp.halfy = lminy + sp.halfsy;
	
	for(tmps=ts;tmps;tmps = tmps->next)
		{
		progress();								// Something for the user to look at.
		
		num = CheckSplits(tmps,ts);		// Check how many splits this line makes

		if(num > 50) num = 50;
		splits = (50 - num);									// 0 = 100 .. 50+ = 0
		dist = NearerMiddle(&sp,tmps) * splits;		// 0 - 10000

		if(dist > min_dist)					// Find out if this is better choice
			{										// than last one
			min_dist = dist;
			best = tmps;
//			least_splits = num;
//			least = tmps->linedef;
			}
		}
	
//	printf("\nLinedef %d had least amount of splits (%d)\n",least,least_splits);

	node_x = vertices[best->start].x;
	node_y = vertices[best->start].y;
	node_dx = vertices[best->end].x-vertices[best->start].x;
	node_dy = vertices[best->end].y-vertices[best->start].y;

/* When we get to here, *best is a pointer to the partition seg.
	Using this partition line, we must split any lines that are intersected
	into a left and right half, flagging them to be put their respective sides
	
	Ok, now we have the best line to use as a partitioning line, we must
   split all of the segs into two lists (rightside & leftside) keeping
	track of the sectors used on each side, so that we can do a simple
	check to see if a SSector needs creating */

	rights = NULL;												// Start off with empty
	lefts = NULL;												// lists.
	strights = NULL;												// Start off with empty
	stlefts = NULL;												// lists.

	for(tmps=ts;tmps;tmps=tmps->next)
		{
		progress();								// Something for the user to look at.
		add_to_rs = NULL;
		add_to_ls = NULL;
		if(tmps != best)
			{
			val = DoLinesIntersect(best,tmps);
			if((val&2 && val&64) || (val&4 && val&32))	// If intersecting !!
				{
				ComputeIntersection(best,tmps,&x,&y);
//				printf("Splitting Linedef %d at %d,%d\n",tmps->linedef,x,y);
				
				vertices = ResizeMemory(vertices, sizeof(struct Vertex) * (num_verts+1));
				vertices[num_verts].x = x;
				vertices[num_verts].y = y;
	
				news = GetMemory(sizeof( struct Seg));
	
				news->next = tmps->next;
				tmps->next = news;
				news->start = num_verts;
				news->end = tmps->end;
				tmps->end = num_verts;
				news->linedef = tmps->linedef;
				news->angle = tmps->angle;
				news->flip = tmps->flip;
				news->dist = SplitDist(news);
//				printf("splitting dist = %d\n",news->dist);
//				printf("splitting vertices = %d,%d,%d,%d\n",tmps->start,tmps->end,news->start,news->end);
				if(val&32)
					{
					add_to_ls = tmps;
//					printf("Old seg is on left side ");
					}
				if(val&64)
					{
					add_to_rs = tmps;
//					printf("Old seg is on right side ");
					}
				if(val&2)
					{
					add_to_ls = news;
//					printf("New seg is on left side \n");
					}
				if(val&4)
					{
					add_to_rs = news;
//					printf("New seg is on right side \n");
					}
				tmps = news;
				num_verts++;
				num_new++;
				}
			else
				{											// Not split, which side ?
				if(val&2 || val&32) add_to_ls = tmps;
				if(val&4 || val&64) add_to_rs = tmps;
				if(val&1 && val&16)					// On same line
					{
					if(tmps->flip == best->flip) add_to_rs = tmps;
					if(tmps->flip != best->flip) add_to_ls = tmps;
					}
				}
			}
		else add_to_rs = tmps;						// This is the partition line
		
//		printf("Val = %X\n",val);

		if(add_to_rs)							// CHECK IF SHOULD ADD RIGHT ONE 
			{
			new_rs = GetMemory(sizeof(struct Seg));
			if(add_to_rs == best) new_best = new_rs;
			new_rs->start = add_to_rs->start;
			new_rs->end = add_to_rs->end;
			new_rs->linedef = add_to_rs->linedef;
			new_rs->angle = add_to_rs->angle;
			new_rs->flip = add_to_rs->flip;
			new_rs->dist = add_to_rs->dist;
			new_rs->next = NULL;
			if(!rights)
				{
				strights = rights = new_rs;
				}
			else
				{
				rights->next = new_rs;
				rights = new_rs;
				}
			}
				
		if(add_to_ls)							// CHECK IF SHOULD ADD LEFT ONE 
			{
			new_ls = GetMemory(sizeof(struct Seg));
			if(add_to_ls == best) new_best = new_ls;
			new_ls->start = add_to_ls->start;
			new_ls->end = add_to_ls->end;
			new_ls->linedef = add_to_ls->linedef;
			new_ls->angle = add_to_ls->angle;
			new_ls->flip = add_to_ls->flip;
			new_ls->dist = add_to_ls->dist;
			new_ls->next = NULL;
			if(!lefts)
				{
				stlefts = lefts = new_ls;
				}
			else
				{
				lefts->next = new_ls;
				lefts = new_ls;
				}
			}
		}

	if(strights == NULL)
		{
//		printf("No right side, moving partition into right side\n");
		strights = rights = new_best;
		prev = NULL;
		for(tmps=stlefts;tmps;tmps=tmps->next)
			{
			if(tmps == new_best)
				{
				if(prev != NULL) prev->next=tmps->next;
				else stlefts=tmps->next;
				}
			prev=tmps;
			}
		prev->next = NULL;
		}
	
	if(stlefts == NULL)
		{
//		printf("No left side, moving partition into left side\n");
		stlefts = lefts = new_best;
		prev = NULL;
		for(tmps=strights;tmps;tmps=tmps->next)
			{
			if(tmps == new_best)
				{
				if(prev != NULL) prev->next=tmps->next;
				else strights=tmps->next;
				}
			prev=tmps;
			}
		stlefts->next = NULL;
		prev->next = NULL;								// Make sure end of list = NULL
		}

	if(rights->next != NULL) rights->next = NULL;
	if(lefts->next != NULL) lefts->next = NULL;

	for(tmps=ts;tmps;tmps=best)
		{
		best=tmps->next;
		free(tmps);
		}

//	printf("Made %d new Vertices and Segs\n",num_new);

	*rs = strights ; *ls = stlefts;
}

/*--------------------------------------------------------------------------*/

int SplitDist(struct Seg *ts)
{
	double t,dx,dy;
	
	dx = (double)(vertices[linedefs[ts->linedef].start].x)-(vertices[ts->start].x);
	dy = (double)(vertices[linedefs[ts->linedef].start].y)-(vertices[ts->start].y);

	if(dx == 0 && dy == 0) printf("Trouble in SplitDist %f,%f\n",dx,dy);
	
	t = sqrt((dx*dx) + (dy*dy));
	return (int)t;
}

/*--------------------------------------------------------------------------*/
/* This routine returns 1 = lines are intersecting at some point
								2 = line is on the right side
								3 = line is on the left side
								4 = lines are on the same line							 */
/*--------------------------------------------------------------------------*/

int DoLinesIntersect(struct Seg *part,struct Seg *line)
{
	int val = 0;
	int x,y;

	double pdx,pdy,dx,dy,dx2,dy2,dx3,dy3,l,l2,l3,a,b;
	
	dx = (double)vertices[part->start].x - (double)vertices[part->end].x;
	dy = (double)vertices[part->start].y - (double)vertices[part->end].y;
	
	dx2 = (double)vertices[part->start].x - (double)vertices[line->start].x;
	dy2 = (double)vertices[part->start].y - (double)vertices[line->start].y;
	dx3 = (double)vertices[part->start].x - (double)vertices[line->end].x;
	dy3 = (double)vertices[part->start].y - (double)vertices[line->end].y;
	
	if(dx == 0 && dy == 0) ProgError("Trouble in DolinesIntersect dx,dy");
	l = sqrt((dx*dx) + (dy*dy));
	pdx = dy / l;
	pdy = dx / l;
	
	a = 0;
	if(!(dx2 == 0 && dy2 == 0))
		{
		if(dx2 == 0 && dy2 == 0) ProgError("Trouble in DolinesIntersect dx2,dy2");
		l2 = sqrt((dx2*dx2) + (dy2*dy2));
		dx2 = dx2 / l2;
		dy2 = dy2 / l2;
		a = pdx*dx2 - pdy*dy2;
		}
	
	b = 0;
	if(!(dx3 == 0 && dy3 == 0))
		{
		if(dx3 == 0 && dy3 == 0) ProgError("Trouble in DolinesIntersect dx3,dy3");
		l3 = sqrt((dx3*dx3) + (dy3*dy3));
		dx3 = dx3 / l3;
		dy3 = dy3 / l3;
		b = pdx*dx3 - pdy*dy3;
		}

	if((a<0 && b>0) || (a>0 && b<0))
		{
		ComputeIntersection(part,line,&x,&y);
		if(x == vertices[line->start].x && y == vertices[line->start].y)
			{										// Intersection is at line start
			a = 0;
//			printf("Wanted to split start !!!!!!!!!!!!!!!!\n");
			}
		if(x == vertices[line->end].x && y == vertices[line->end].y)
			{										// Intersection is at line end
			b = 0;
//			printf("Wanted to split end !!!!!!!!!!!!!!!!\n");
			}
		}

	if(a == 0) val = val | 16;						// start is on middle
	if(a < 0) val = val | 32;						// start is on left side
	if(a > 0) val = val | 64;						// start is on right side

	if(b == 0) val = val | 1;						// end is on middle
	if(b < 0) val = val | 2;						// end is on left side
	if(b > 0) val = val | 4;						// end is on right side

	return val;
}

/*--------------------------------------------------------------------------*/

void ComputeIntersection(struct Seg *part,struct Seg *line,int *outx,int *outy)
{
	double a,b,a2,b2,x,y,x2,y2,l,l2,w,d;
	double dx,dy,dx2,dy2,fractx,fracty;

	x = (double)vertices[part->start].x;
	y = (double)vertices[part->start].y;
	x2 = (double)vertices[line->start].x;
	y2 = (double)vertices[line->start].y;
	
	dx = (double)vertices[part->end].x - (double)vertices[part->start].x;
	dy = (double)vertices[part->end].y - (double)vertices[part->start].y;
	dx2 = (double)vertices[line->end].x - (double)vertices[line->start].x;
	dy2 = (double)vertices[line->end].y - (double)vertices[line->start].y;

	if(dx == 0 && dy == 0) ProgError("Trouble in ComputeIntersection dx,dy");
	l = sqrt((dx*dx) + (dy*dy));
	if(dx2 == 0 && dy2 == 0) ProgError("Trouble in ComputeIntersection dx2,dy2");
	l2 = sqrt((dx2*dx2) + (dy2*dy2));
	
	d = dy * dx2 - dx * dy2;
	if(d != 0.0)
		{
		a = dx / l;
		b = dy / l;
		a2 = dx2 / l2;
		b2 = dy2 / l2;
	
		w = ((a*(y2-y))+(b*(x-x2))) / ((b*a2) - (a*b2));
	
//		printf("Intersection at (%f,%f)\n",x2+(a2*w),y2+(b2*w));
	
		x2 = x2+(a2*w);
		y2 = y2+(b2*w);
		modf((float)(x2)+ ((x2<0)?-0.5:0.5) ,&x);
		modf((float)(y2)+ ((y2<0)?-0.5:0.5) ,&y);
		}
	
		*outx = x;
		*outy = y;
}

/*--------------------------------------------------------------------------*/

int CheckSplits(struct Seg *thiss,struct Seg *ts)
{
	int val;
	int num = 0;

	for(;ts;ts = ts->next)
		{
		if(ts != thiss)					// Not check same line with itself
			{
			val = DoLinesIntersect(thiss,ts);
			if((val&2 && val&64) || (val&4 && val&32))
				{
				num++;
				}
			}
		}
	return num;
}

/*--------------------------------------------------------------------------*/

int NearerMiddle(struct splitter *sp,struct Seg *tmps)
{
	int per;
	double centrex,centrey,dtc,dtm;

	centrex = ((vertices[tmps->start].x) + ((vertices[tmps->end].x-vertices[tmps->start].x)/2));
	centrey = ((vertices[tmps->start].y) + ((vertices[tmps->end].y-vertices[tmps->start].y)/2));

	centrex = centrex - (double)sp->halfx;
	centrey = centrey - (double)sp->halfy;

	if(centrex == 0 && centrey == 0) dtc = 0;
	else dtc = sqrt((centrex*centrex) + (centrey*centrey));
	
	if(sp->halfsx == 0 && sp->halfsy == 0) ProgError("Trouble in NearerMiddle sp->halfsx,sp->halfsy");
	dtm = sqrt(((double)sp->halfsx*(double)sp->halfsx) + ((double)sp->halfsy*(double)sp->halfsy));

	per = (100 - ((dtc/dtm)*100));

	return per;
}		

/*- initially creates all segs, one for each line def ----------------------*/

struct Seg *CreateSegs()
{
	struct Seg *cs = NULL;					// current and temporary Segs
	struct Seg *ts = NULL;
	struct Seg *fs = NULL;					// first Seg in list
	
	int	num,t,n,this_seg,fv,tv;
	int	f,dx,dy;

	for(n=0; n < num_lines; n++)			// step through linedefs and get side
		{											// numbers
		fv = linedefs[n].start;
		tv = linedefs[n].end;

		if(linedefs[n].sidedef1 != -1)
			{																// create normal seg
			ts = GetMemory( sizeof( struct Seg));				// get mem for Seg
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
			dx = (vertices[tv].x-vertices[fv].x);
			dy = (vertices[tv].y-vertices[fv].y);
			cs->angle = ComputeAngle(dx,dy);
//			printf("On linedef %d : %d,%d\n",n,fv,tv);
			cs->linedef = n;
			cs->dist = 0;
			cs->flip = 0;
			num_tsegs++;
			}
		if(linedefs[n].sidedef2 != -1)
			{										// create flipped seg
			ts = GetMemory( sizeof( struct Seg));				// get mem for Seg
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
//			printf("On linedef %d\n",n);
			cs->linedef = n;
			cs->dist = 0;
			cs->flip = 1;
			num_tsegs++;
			}
		}

	num = 0;

	for(n=0; n < num_verts; n++)
		{
		cs = fs;
		f = NULL;
		for(t=0; t< num_tsegs; t++)
			{
			if(n == cs->start || n == cs->end) f = 1;
			cs = cs->next;
			}
		if(!f) num++;
		}

	
//	printf("\nCreated %d Segs, but found %d unused Vertices.\n\n",num_tsegs,num);

	return fs;
}

/*--------------------------------------------------------------------------*/
// Find limits from a list of segs, does this by stepping through the segs
// and comparing the vertices at both ends.
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
//		printf("%d : %d,%d\n",n,vertices[n].x,vertices[n].y);
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

/*- get the directory from a wad file --------------------------------------*/

void OpenWadFile(char *filename)
{
	struct directory *dir;

	long n;

	if((infile = fopen(filename,"rb")) == NULL)
      ProgError( "Cannot find %s", filename);

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
//		Printname(dir);
//		printf(" of size %lu at %lu.\n",dir->length,dir->start);
		dir++;
		}
}

/*- read the things from the wad file and place in 'things' ----------------*/

void GetThings(void)
{
	int n;

	n = FindDir("THINGS");
	num_things = (direc[n].length) / (sizeof( struct Thing));
//	printf("Allocating %lu bytes for %d things\n",direc[n].length,num_things);
	things = GetMemory( direc[n].length);
	fseek(infile,direc[n].start,0);
	fread(things,direc[n].length,1,infile);
}

/*- read the vertices from the wad file and place in 'vertices' ------------*/

void GetVertexes(void)
{
	int n;

	n = FindDir("VERTEXES");
	num_verts = (direc[n].length) / (sizeof( struct Vertex));
//	printf("Allocating %lu bytes for %d vertices\n",direc[n].length,num_verts);
	vertices = GetMemory( direc[n].length);
	fseek(infile,direc[n].start,0);
	fread(vertices,direc[n].length,1,infile);
}

/*- read the linedefs from the wad file and place in 'linedefs' ------------*/

void GetLinedefs(void)
{
	int n;

	n = FindDir("LINEDEFS");
	num_lines = (direc[n].length) / (sizeof( struct LineDef));
//	printf("Allocating %lu bytes for %d Linedefs\n",direc[n].length,num_lines);
	linedefs = GetMemory( direc[n].length);
	fseek(infile,direc[n].start,0);
	fread(linedefs,direc[n].length,1,infile);
}

/*- read the sidedefs from the wad file and place in 'sidedefs' ------------*/

void GetSidedefs(void)
{
	int n;

	n = FindDir("SIDEDEFS");
	num_sides = (direc[n].length) / (sizeof( struct SideDef));
//	printf("Allocating %lu bytes for %d Sidedefs\n",direc[n].length,num_sides);
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
//	printf("Allocating %lu bytes for %d Sectors\n",direc[n].length,num_sects);
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
	return NULL;
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
			blockptrs[blocknum]=(blockoffs+4+(blockptrs_size/2));
			
			blocklists = ResizeMemory(blocklists,((blockoffs+1)*2));
			blocklists[blockoffs]=0;
			blockoffs++;
			for(n=0;n<num_lines;n++)
				{
				if(IsLineDefInside(n,(blockhead.minx+(x*128)),(blockhead.miny+(y*128)),(blockhead.minx+(x*128))+127,(blockhead.miny+(y*128))+127))
					{
//					printf("found line %d in block %d\n",n,blocknum);
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

int IsLineDefInside( int ldnum, int x0, int y0, int x1, int y1)
{
   int lx0 = vertices[ linedefs[ ldnum].start].x;
   int ly0 = vertices[ linedefs[ ldnum].start].y;
   int lx1 = vertices[ linedefs[ ldnum].end].x;
   int ly1 = vertices[ linedefs[ ldnum].end].y;

   return Clipper(lx0,ly0,lx1,ly1,x0,x1,y0,y1);
}

/*--------------------------------------------------------------------------*/

int Clipper(int x1,int y1,int x2,int y2,int xmin,int xmax,int ymin,int ymax)
{
	int outcode1,outcode2,t;

	while(1)
		{
		outcode1 = Outcodes(x1,y1,xmin,xmax,ymin,ymax);
		outcode2 = Outcodes(x2,y2,xmin,xmax,ymin,ymax);
		if(RejectCheck(outcode1,outcode2)) return FALSE;
		if(AcceptCheck(outcode1,outcode2)) return TRUE;

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

int RejectCheck(int outcode1,int outcode2)
{
	return (outcode1&outcode2);
}

int AcceptCheck(int outcode1,int outcode2)
{
	return !(outcode1&outcode2);
}

/*--------------------------------------------------------------------------*/

int Outcodes(int x,int y,int xmin,int xmax,int ymin,int ymax)
{
	int outcode=0;

	if(y<ymin) outcode|=1;
	if(y>ymax) outcode|=2;
	if(x>xmax) outcode|=4;
	if(x<xmin) outcode|=8;

	return outcode;
}

/*--------------------------------------------------------------------------*/

void progress()
{
	char *s="/-\\|/-\\|";

	if((pcnt&15) == 0) printf("%c\b",s[((pcnt)/16)&7]);
	pcnt++;

}

/*- end of file ------------------------------------------------------------*/
