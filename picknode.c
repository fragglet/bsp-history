/*- PICKNODE.C --------------------------------------------------------------*
 To be able to divide the nodes down, this routine must decide which is the
 best Seg to use as a nodeline. It does this by selecting the line with least
 splits and has least difference of Segs on either side of it.

 Credit to Raphael Quinet and DEU, this routine is a copy of the nodeline
 picker used in DEU5beta. I am using this method because the method I
 originally used was not so good.

 Rewritten by Lee Killough to significantly improve performance, while
 not affecting results one bit in >99% of cases (some tiny differences
 due to roundoff error may occur, but they are insignificant).
*---------------------------------------------------------------------------*/

#define FACTOR 8  /* This is the original "factor" used by previous versions
                     of the code -- it must be maintained in a macro to avoid
                     mistakes if we are to keep the tradition of using it,
                     and being able to modify it.
		   */

static int factor=2*FACTOR+1;

static __inline__ struct Seg *PickNode(struct Seg *ts)
{
 struct Seg *best=ts;	         /* Set best to first (failsafe measure)*/
 int bestgrade=~(1<<(8*sizeof(int)-1));  /* Portable definition of INT_MAX */
 struct Seg *part;
 int cnt;

 for (cnt=0,part=ts;part;part=part->next,cnt++) /* Count once and for all */
  {
   double dx=(long) (part->psx = vertices[part->start].x)
                  - (part->pex = vertices[part->end].x);
   double dy=(long) (part->psy = vertices[part->start].y)
                  - (part->pey = vertices[part->end].y);
   if (! (part->tmpdist=sqrt(dx*dx+dy*dy)))
     ProgError("Trouble in PickNode dx, dy");
  }

 for (part=ts;part;part = part->next)	/* Use each Seg as partition*/
  {
   short psx = part->psx;
   short psy = part->psy;
   long pdx  = (long) psx - part->pex;	/* Partition line DX,DY*/
   long pdy  = (long) psy - part->pey;
   long ptmp = pdx*psy-psx*pdy;		/* Used to decrease arithmetic */
                                        /* inside the inner loop       */
   struct Seg *check;
   int partflip=part->flip;
   int tot=cnt,grade=cnt,diff=cnt*2;    /* cnt computed before loops */

   progress();           	        /* Something for the user to look at.*/

   for (check=ts;check;check=check->next) /* Check partition against all Segs*/
    {
        /*     get state of lines' relation to each other    */

     register long a=pdx * check->psy - pdy * check->psx - ptmp;
     register long b=pdx * check->pey - pdy * check->pex - ptmp;

     if ((a^b) < 0)
       if (a && b)
	{                           /* Line is split; a,b nonzero, opposite sign */
	 long l=check->tmpdist;     /* Length of segment, previously computed */
	 long d=a*l/(a-b);          /* How far from start the intersection is */
	 if (d >= 2)                /* Only consider >=2 ; dist<2 is special */
	  {
	   grade+=factor;
	   if (grade > bestgrade)   /* This is the heart of my pruning idea - */
	     goto prune;            /* it catches bad segs early on. Killough */
	   tot++;
	  }
	 else                       /* Distance from start is less than 2; */
           if (l-d < 2 ? check->flip != partflip : b < 0)
             diff-=2;               /* Check distance from end */
	}
       else
         diff-=2;
     else
       if (a<=0 && (a!=0 || (check->flip!=partflip && b<=0)))
         diff-=2;
    }

   diff-=tot;

   if (diff < 0)        /* Take absolute value. diff is being used to obtain the */
     diff= -diff;       /* min/max values by way of: max(a,b)=(a+b+abs(a-b))/2   */

   if (diff < tot)	/* Make sure at least one Seg is*/
    {		        /* on either side of the partition*/
     grade+=diff;
     if (grade < bestgrade)
      {
       bestgrade = grade;
       best = part;	/* and remember which Seg*/
      }
    }
prune:;                 /* early exit and skip past the tests above */
  }
 return best;		/* all finished, return best Seg*/
}

/*---------------------------------------------------------------------------*
 Calculate the point of intersection of two lines. ps?->pe? & ls?->le?
 returns int xcoord, int ycoord
*---------------------------------------------------------------------------*/

static __inline__ void ComputeIntersection(short int *outx,short int *outy)
{
	double a,b,a2,b2,l2,w,d,z;

	long dx,dy,dx2,dy2;

	dx = pex - psx;
	dy = pey - psy;
	dx2 = lex - lsx;
	dy2 = ley - lsy;

	if (dx == 0 && dy == 0) ProgError("Trouble in ComputeIntersection dx,dy");
/*	l = (long)sqrt((float)((dx*dx) + (dy*dy)));  unnecessary - killough */
	if(dx2 == 0 && dy2 == 0) ProgError("Trouble in ComputeIntersection dx2,dy2");
	l2 = (long)sqrt((float)((dx2*dx2) + (dy2*dy2)));

	a = dx /* / l */;  /* no normalization of a,b necessary,   */
	b = dy /* / l */;  /* since division by d in formula for w */
	a2 = dx2 / l2;     /* cancels it out. */
	b2 = dy2 / l2;
	d = b * a2 - a * b2;
	w = lsx;
	z = lsy;
	if(d != 0.0)
		{
		w = (((a*(lsy-psy))+(b*(psx-lsx))) / d);

/*		printf("Intersection at (%f,%f)\n",x2+(a2*w),y2+(b2*w));*/

		a = lsx+(a2*w);
		b = lsy+(b2*w);
		modf((float)(a)+ ((a<0)?-0.5:0.5) ,&w);
		modf((float)(b)+ ((b<0)?-0.5:0.5) ,&z);
		}

	*outx = w;
	*outy = z;
}

/*---------------------------------------------------------------------------*
 Because this is (was) used a horrendous amount of times in the inner loops, the
 coordinate of the lines are setup outside of the routine in global variables
 psx,psy,pex,pey = partition line coordinates
 lsx,lsy,lex,ley = checking line coordinates
 The routine returns 'val' which has 3 bits assigned to the the start and 3
 to the end. These allow a decent evaluation of the lines state.
 bit 0,1,2 = checking lines starting point and bits 4,5,6 = end point
 these bits mean 	0,4 = point is on the same line
 						1,5 = point is to the left of the line
						2,6 = point is to the right of the line
 There are some failsafes in here, these mainly check for small errors in the
 side checker.
*---------------------------------------------------------------------------*/

static __inline__ int DoLinesIntersect()
{
	short int x,y,val = 0;

	long dx2,dy2,dx3,dy3,a,b,l;

	dx2 = psx - lsx;									/* Checking line -> partition*/
	dy2 = psy - lsy;
	dx3 = psx - lex;
	dy3 = psy - ley;

	a = pdy*dx2 - pdx*dy2;
	b = pdy*dx3 - pdx*dy3;
	if ( (a ^ b) < 0 && a && b)								/* Line is split, just check that*/
		{
		ComputeIntersection(&x,&y);
		dx2 = lsx - x;									/* Find distance from line start*/
		dy2 = lsy - y;									/* to split point*/
		if(dx2 == 0 && dy2 == 0) a = 0;
		else
			{
			l = (long) dx2*dx2+(long) dy2*dy2;		                        /* If either ends of the split*/
			if (l < 4) a = 0;							/* are smaller than 2 pixs then*/
			}												/* assume this starts on part line*/
		dx3 = lex - x;									/* Find distance from line end*/
		dy3 = ley - y;									/* to split point*/
		if(dx3 == 0 && dy3 == 0) b = 0;
		else
			{
			l = (long) dx3*dx3 + (long) dy3*dy3;					/* same as start of line*/
			if (l < 4) b = 0;
			}
		}

	if(a == 0) val = val | 16;								/* start is on middle*/
         else
	if(a < 0) val = val | 32;						/* start is on left side*/
        else
	/* if(a > 0) */ val = val | 64;						/* start is on right side*/

	if (b == 0) val = val | 1;						/* end is on middle*/
        else
	if(b < 0) val = val | 2;						/* end is on left side*/
        else
	/* if(b > 0) */ val = val | 4;						/* end is on right side*/

	return val;
}

