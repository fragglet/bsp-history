/*- PICKNODE.C --------------------------------------------------------------*
 To be able to divide the nodes down, this routine must decide which is the
 best Seg to use as a nodeline. It does this by selecting the line with least
 splits and has least difference of Segs on either side of it.

 Credit to Raphael Quinet and DEU, this routine is a copy of the nodeline
 picker used in DEU5beta. I am using this method because the method I 
 originally used was not so good.
*---------------------------------------------------------------------------*/

struct Seg *PickNode(struct Seg *ts)
{
	int num_splits,num_left,num_right;
	int min_splits,min_diff,val;
	struct Seg *part,*check,*best;
	int max,new,grade = 0,bestgrade = 32767,seg_count;

   min_splits = 32767;
   min_diff = 32767;
   best = ts;									/* Set best to first (failsafe measure)*/

	for(part=ts;part;part = part->next)	/* Use each Seg as partition*/
		{
		progress();								/* Something for the user to look at.*/
		
		psx = vertices[part->start].x;	/* Calculate this here, cos it doesn't*/
		psy = vertices[part->start].y;	/* change for all the interations of*/
		pex = vertices[part->end].x;		/* the inner loop!*/
		pey = vertices[part->end].y;
		pdx = psx - pex;						/* Partition line DX,DY*/
		pdy = psy - pey;
	
		num_splits = 0;
		num_left = 0;
		num_right = 0;

		seg_count = 0;
		for(check=ts;check;check=check->next)	/* Check partition against all Segs*/
			{
			seg_count++;
			if(check == part) num_right++;		/* If same as partition, inc right count*/
			else
				{
				lsx = vertices[check->start].x;	/* Calculate this here, cos it doesn't*/
				lsy = vertices[check->start].y;	/* change for all the interations of*/
				lex = vertices[check->end].x;		/* the inner loop!*/
				ley = vertices[check->end].y;
				val = DoLinesIntersect();			/* get state of lines relation to each other*/
				if((val&2 && val&64) || (val&4 && val&32))
					{
					num_splits++;						/* If line is split, inc splits*/
					num_left++;							/* and add one line into both*/
					num_right++;						/* sides*/
					}
				else
					{
					if(val&1 && val&16)				/* If line is totally in same*/
						{									/* direction*/
						if(check->flip == part->flip) num_right++;
						else num_left++;				/* add to side according to flip*/
						}
					else									/* So, now decide which side*/
						{									/* the line is on*/
						if(val&34) num_left++;		/* and inc the appropriate*/
						if(val&68) num_right++;		/* count*/
						}
					}
				}
/*			if(num_splits > min_splits) break;	/* If more splits than last, reject*/
			}

		if(num_right > 0 && num_left > 0)		/* Make sure at least one Seg is*/
			{												/* on either side of the partition*/
			max = max(num_right,num_left);
			new = (num_right + num_left) - seg_count;
			grade = max+new*8;

			if(grade < bestgrade)
				{
				bestgrade = grade;
				best = part;							/* and remember which Seg*/
				}
			}
		}
	return best;										/* all finished, return best Seg*/
}

/*---------------------------------------------------------------------------*
 Because this is used a horrendous amount of times in the inner loops, the
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

int DoLinesIntersect()
{
	short int x,y,val = 0;

	long dx2,dy2,dx3,dy3,a,b,l;
	
	dx2 = psx - lsx;									/* Checking line -> partition*/
	dy2 = psy - lsy;
	dx3 = psx - lex;
	dy3 = psy - ley;
	
	a = pdy*dx2 - pdx*dy2;
	b = pdy*dx3 - pdx*dy3;

	if((a<0 && b>0) || (a>0 && b<0))				/* Line is split, just check that*/
		{
		ComputeIntersection(&x,&y);
		dx2 = lsx - x;									/* Find distance from line start*/
		dy2 = lsy - y;									/* to split point*/
		if(dx2 == 0 && dy2 == 0) a = 0;
		else
			{
			l = (long)sqrt((float)((dx2*dx2)+(dy2*dy2)));		/* If either ends of the split*/
			if(l < 2) a = 0;							/* are smaller than 2 pixs then*/
			}												/* assume this starts on part line*/
		dx3 = lex - x;									/* Find distance from line end*/
		dy3 = ley - y;									/* to split point*/
		if(dx3 == 0 && dy3 == 0) b = 0;
		else
			{
			l = (long)sqrt((float)((dx3*dx3)+(dy3*dy3)));		/* same as start of line*/
			if(l < 2) b = 0;
			}
		}

	if(a == 0) val = val | 16;						/* start is on middle*/
	if(a < 0) val = val | 32;						/* start is on left side*/
	if(a > 0) val = val | 64;						/* start is on right side*/

	if(b == 0) val = val | 1;						/* end is on middle*/
	if(b < 0) val = val | 2;						/* end is on left side*/
	if(b > 0) val = val | 4;						/* end is on right side*/

	return val;
}

/*---------------------------------------------------------------------------*
 Calculate the point of intersection of two lines. ps?->pe? & ls?->le?
 returns int xcoord, int ycoord
*---------------------------------------------------------------------------*/

void ComputeIntersection(short int *outx,short int *outy)
{
	double a,b,a2,b2,l,l2,w,d,z;
	
	long dx,dy,dx2,dy2;

	dx = pex - psx;
	dy = pey - psy;
	dx2 = lex - lsx;
	dy2 = ley - lsy;

	if(dx == 0 && dy == 0) ProgError("Trouble in ComputeIntersection dx,dy");
	l = (long)sqrt((float)((dx*dx) + (dy*dy)));
	if(dx2 == 0 && dy2 == 0) ProgError("Trouble in ComputeIntersection dx2,dy2");
	l2 = (long)sqrt((float)((dx2*dx2) + (dy2*dy2)));
	
	a = dx / l;
	b = dy / l;
	a2 = dx2 / l2;
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

/*--------------------------------------------------------------------------*/

