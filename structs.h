/*- Doom Structures .. Colin Reed 1994 -------------------------------------*/

struct wad_header 					// Linked wad files list.
{
	char type[4];
	long int num_entries;
	long int dir_start;
};

struct directory 						// The directory entry header
{
	long int start;
	long int length;
	char name[8];
};

struct Block
{
	int minx;
	int miny;
	int xblocks;
	int yblocks;
};

/*- The level structures ---------------------------------------------------*/

struct Thing
{
   int xpos;      /* x position */
   int ypos;      /* y position */
   int angle;     /* facing angle */
   int type;      /* thing type */
   int when;      /* appears when? */
};

struct Vertex
{
   int x;         /* X coordinate */
   int y;         /* Y coordinate */
};

struct LineDef
{
   int start;     /* from this vertex ... */
   int end;       /* ... to this vertex */
   int flags;     /* see NAMES.C for more info */
   int type;      /* see NAMES.C for more info */
   int tag;       /* crossing this linedef activates the sector with the same tag */
   int sidedef1;  /* sidedef */
   int sidedef2;  /* only if this line adjoins 2 sectors */
};

struct SideDef
{
   int xoff;      /* X offset for texture */
   int yoff;      /* Y offset for texture */
   char tex1[8];  /* texture name for the part above */
   char tex2[8];  /* texture name for the part below */
   char tex3[8];  /* texture name for the regular part */
   int sector;    /* adjacent sector */
};

struct Sector
{
   int floorh;    /* floor height */
   int ceilh;     /* ceiling height */
   char floort[8];/* floor texture */
   char ceilt[8]; /* ceiling texture */
   int light;     /* light level (0-255) */
   int special;   /* special behaviour (0 = normal, 9 = secret, ...) */
   int tag;       /* sector activated by a linedef with the same tag */
};

/*--------------------------------------------------------------------------*/
/* These are the structure used for creating the NODE bsp tree.
/*--------------------------------------------------------------------------*/

struct Seg
{
   int start;     /* from this vertex ... */
   int end;       /* ... to this vertex */
   unsigned angle;/* angle (0 = east, 16384 = north, ...) */
   int linedef;   /* linedef that this seg goes along*/
   int flip;      /* true if not the same direction as linedef */
   unsigned dist; /* distance from starting point */
	struct Seg *next;
};

struct Pseg
{
   int start;     /* from this vertex ... */
   int end;       /* ... to this vertex */
   unsigned angle;/* angle (0 = east, 16384 = north, ...) */
   int linedef;   /* linedef that this seg goes along*/
   int flip;      /* true if not the same direction as linedef */
   unsigned dist; /* distance from starting point */
};

struct Node
{
   int x, y;									// starting point
   int dx, dy;									// offset to ending point
   int maxy1, miny1, minx1, maxx1;		// bounding rectangle 1
   int maxy2, miny2, minx2, maxx2;		// bounding rectangle 2
   int chright, chleft;						// Node or SSector (if high bit is set)
	struct Node *nextr,*nextl;
	int node_num;								// starting at 0 (but reversed when done)
};

struct Pnode
{
   int x, y;									// starting point
   int dx, dy;									// offset to ending point
   int maxy1, miny1, minx1, maxx1;		// bounding rectangle 1
   int maxy2, miny2, minx2, maxx2;		// bounding rectangle 2
   int chright, chleft;						// Node or SSector (if high bit is set)
};

struct SSector
{
   int num;       /* number of Segs in this Sub-Sector */
   int first;     /* first Seg */
};

struct splitter
{
	int halfx;
	int halfy;
	int halfsx;
	int halfsy;
};

/*--------------------------------------------------------------------------*/
