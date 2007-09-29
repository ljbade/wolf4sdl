// ID_VH.C

#include "wl_def.h"

//#define	SCREENWIDTH		80
#define CHARWIDTH		2
#define TILEWIDTH		4
#define GRPLANES		4
#define BYTEPIXELS		4

#define SCREENXMASK		(~3)
#define SCREENXPLUS		(3)
#define SCREENXDIV		(4)

//#define VIEWWIDTH		80

// TODO: Check this for higher resolutions
#define PIXTOBLOCK		4		// 16 pixels to an update block

byte	update[UPDATEHIGH][UPDATEWIDE];

//==========================================================================

pictabletype	*pictable;
SDL_Surface     *latchpics[NUMLATCHPICS];

int	    px,py;
byte	fontcolor,backcolor;
int	    fontnumber;
int     bufferwidth,bufferheight;

//==========================================================================

void VWB_DrawPropString(const char* string)
{
	fontstruct  *font;
	int		    width, step, height;
	byte	    *source, *dest, *origdest;
	byte	    ch, mask;

    byte *vbuf = LOCK();

	font = (fontstruct *) grsegs[STARTFONT+fontnumber];
	height = bufferheight = font->height;
	dest = origdest = vbuf + py * curPitch + px;

	while ((ch = *string++)!=0)
	{
		width = step = font->width[ch];
		source = ((byte *)font)+font->location[ch];
		while (width--)
		{
			for(int i=0;i<height;i++)
			{
				if(source[i*step])
					dest[i*curPitch]=fontcolor;
			}

			source++;
			px++;
			dest++;
		}
	}
	bufferheight = height;
	bufferwidth = ((dest+1)-origdest)*4;

	UNLOCK();
}

/*
=================
=
= VL_MungePic
=
=================
*/

void VL_MungePic (byte *source, unsigned width, unsigned height)
{
	unsigned x,y,plane,size,pwidth;
	byte *temp, *dest, *srcline;

	size = width*height;

	if (width&3)
		Quit ("VL_MungePic: Not divisable by 4!");

//
// copy the pic to a temp buffer
//
	temp=(byte *) malloc(size);
	memcpy (temp,source,size);

//
// munge it back into the original buffer
//
	dest = source;
	pwidth = width/4;

	for (plane=0;plane<4;plane++)
	{
		srcline = temp;
		for (y=0;y<height;y++)
		{
			for (x=0;x<pwidth;x++)
				*dest++ = *(srcline+x*4+plane);
			srcline+=width;
		}
	}

	free(temp);
}

void VWL_MeasureString (const char *string, word *width, word *height, fontstruct *font)
{
	*height = font->height;
	for (*width = 0;*string;string++)
		*width += font->width[*((byte *)string)];	// proportional width
}

void VW_MeasurePropString (const char *string, word *width, word *height)
{
	VWL_MeasureString(string,width,height,(fontstruct *)grsegs[STARTFONT+fontnumber]);
}

/*
=============================================================================

				Double buffer management routines

=============================================================================
*/

void VH_UpdateScreen()
{
#ifdef NOTYET
	VGAMAPMASK(15);
	VGAWRITEMODE(1);
	byte *updateptr=(byte *) update;
	for(int y=0;y<UPDATEHIGH;y++)
	{
		for(int x=0;x<UPDATEWIDE;x++,updateptr++)
		{
			if(*updateptr)
			{
				*updateptr=0;
				int offs=y*16*SCREENWIDTH+x*TILEWIDTH;
				for(int i=0;i<16;i++,offs+=linewidth)
				{
					*(vdisp+offs)=*(vbuf+offs);
					*(vdisp+offs+1)=*(vbuf+offs+1);
					*(vdisp+offs+2)=*(vbuf+offs+2);
					*(vdisp+offs+3)=*(vbuf+offs+3);
				}
			}
		}
	}
	VGAWRITEMODE(0);
#else
    SDL_BlitSurface(screenBuffer, NULL, screen, NULL);
    SDL_UpdateRect(screen, 0, 0, 0, 0);
#endif
}

/*
=======================
=
= VW_MarkUpdateBlock
=
= Takes a pixel bounded block and marks the tiles in bufferblocks
= Returns 0 if the entire block is off the buffer screen
=
=======================
*/

int VW_MarkUpdateBlock (int x1, int y1, int x2, int y2)
{
	int	x,y,xt1,yt1,xt2,yt2,nextline;
	byte *mark;

	xt1 = x1>>PIXTOBLOCK;
	yt1 = y1>>PIXTOBLOCK;

	xt2 = x2>>PIXTOBLOCK;
	yt2 = y2>>PIXTOBLOCK;

	if (xt1<0)
		xt1=0;
	else if (xt1>=UPDATEWIDE)
		return 0;

	if (yt1<0)
		yt1=0;
	else if (yt1>UPDATEHIGH)
		return 0;

	if (xt2<0)
		return 0;
	else if (xt2>=UPDATEWIDE)
		xt2 = UPDATEWIDE-1;

	if (yt2<0)
		return 0;
	else if (yt2>=UPDATEHIGH)
		yt2 = UPDATEHIGH-1;

	mark = (byte *) update + yt1*UPDATEWIDE + xt1;
	nextline = UPDATEWIDE - (xt2-xt1) - 1;

	for (y=yt1;y<=yt2;y++)
	{
		for (x=xt1;x<=xt2;x++)
			*mark++ = 1;			// this tile will need to be updated

		mark += nextline;
	}

	return 1;
}

void VWB_DrawTile8 (int x, int y, int tile)
{
//	if (VW_MarkUpdateBlock (x,y,x+7,y+7))
		LatchDrawChar(x,y,tile);
}

void VWB_DrawTile8M (int x, int y, int tile)
{
//	if (VW_MarkUpdateBlock (x,y,x+7,y+7))
		VL_MemToScreen (((byte *)grsegs[STARTTILE8M])+tile*64,8,8,x,y);
}

void VWB_DrawPic (int x, int y, int chunknum)
{
	int	picnum = chunknum - STARTPICS;
	unsigned width,height;

	x &= ~7;

	width = pictable[picnum].width;
	height = pictable[picnum].height;

//	if (VW_MarkUpdateBlock (x,y,x+width-1,y+height-1))
		VL_MemToScreen (grsegs[chunknum],width,height,x,y);
}


void VWB_Bar (int x, int y, int width, int height, int color)
{
//	if (VW_MarkUpdateBlock (x,y,x+width,y+height-1) )
		VW_Bar (x,y,width,height,color);
}

void VWB_Plot (int x, int y, int color)
{
//	if (VW_MarkUpdateBlock (x,y,x,y))
		VW_Plot(x,y,color);
}

void VWB_Hlin (int x1, int x2, int y, int color)
{
//	if (VW_MarkUpdateBlock (x1,y,x2,y))
		VW_Hlin(x1,x2,y,color);
}

void VWB_Vlin (int y1, int y2, int x, int color)
{
//	if (VW_MarkUpdateBlock (x,y1,x,y2))
		VW_Vlin(y1,y2,x,color);
}


/*
=============================================================================

						WOLFENSTEIN STUFF

=============================================================================
*/

/*
=====================
=
= LatchDrawPic
=
=====================
*/

void LatchDrawPic (unsigned x, unsigned y, unsigned picnum)
{
	VL_LatchToScreen (latchpics[2+picnum-LATCHPICS_LUMP_START], x*8, y);
}


//==========================================================================

/*
===================
=
= LoadLatchMem
=
===================
*/

void LoadLatchMem (void)
{
	int	i,width,height,start,end;
	byte *src;
	SDL_Surface *surf;

//
// tile 8s
//
    surf = SDL_CreateRGBSurface(SDL_HWSURFACE, 8*8,
        ((NUMTILE8 + 7) / 8) * 8, 8, 0, 0, 0, 0);
    if(surf == NULL)
    {
        Quit("Unable to create surface for tiles!");
    }
    SDL_SetColors(surf, gamepal, 0, 256);

	latchpics[0] = surf;
	CA_CacheGrChunk (STARTTILE8);
	src = grsegs[STARTTILE8];

    byte *surfmem = VL_LockSurface(surf);

	for (i=0;i<NUMTILE8;i++)
	{
		VL_MemToLatch (src, 8, 8, surf, (i & 7) * 8, (i >> 3) * 8);
		src += 64;
	}
	UNCACHEGRCHUNK (STARTTILE8);

//
// pics
//
	start = LATCHPICS_LUMP_START;
	end = LATCHPICS_LUMP_END;

	for (i=start;i<=end;i++)
	{
		width = pictable[i-STARTPICS].width;
		height = pictable[i-STARTPICS].height;
		surf = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, 8, 0, 0, 0, 0);
        if(surf == NULL)
        {
            Quit("Unable to create surface for picture!");
        }
        SDL_SetColors(surf, gamepal, 0, 256);

		latchpics[2+i-start] = surf;
		CA_CacheGrChunk (i);
		VL_MemToLatch (grsegs[i], width, height, surf, 0, 0);
		UNCACHEGRCHUNK(i);
	}
}

//==========================================================================

/*
===================
=
= FizzleFade
=
= returns true if aborted
=
===================
*/

#ifdef BIGVIDEOSIZE
const unsigned int xb = 9;
const unsigned int yb = 8;
#else
const unsigned int xb = 10;
const unsigned int yb = 10;
#endif
const unsigned int rndmask = 9 << (xb + yb - 4);

boolean FizzleFade (SDL_Surface *source, SDL_Surface *dest,	int x1, int y1,
    unsigned width, unsigned height, unsigned frames, boolean abortable)
{
	int		 pixperframe;
	unsigned x,y,p,frame;
	int32_t  rndval;

	rndval = 0;
	pixperframe = width * height / frames; //64000/frames;

	IN_StartAck ();

	frame = GetTimeCount();
	byte *srcptr = VL_LockSurface(source);
	do
	{
		if (abortable && IN_CheckAck ())
		{
		    VL_UnlockSurface(source);
            SDL_BlitSurface(screenBuffer, NULL, screen, NULL);
            SDL_UpdateRect(screen, 0, 0, 0, 0);
			return true;
		}

		byte *destptr = VL_LockSurface(dest);

		for (p=0;p<pixperframe;p++)
		{
			//
			// seperate random value into x/y pair
			//

			x = rndval >> yb;
			y = rndval & ((1 << yb) - 1);

			//
			// advance to next random element
			//

			rndval = (rndval >> 1) ^ (rndval & 1 ? 0 : rndmask);

			if (x>=width || y>=height)
			{
                if(rndval == 0)     // entire sequence has been completed
                    goto finished;
			    p--;
				continue;
			}

			//
			// copy one pixel
			//

            *(destptr + (y1 + y) * dest->pitch + x1 + x)
                = *(srcptr + (y1 + y) * source->pitch + x1 + x);

			if (rndval == 0)		// entire sequence has been completed
                goto finished;
		}
        VL_UnlockSurface(dest);
        SDL_UpdateRect(dest, 0, 0, 0, 0);
		frame++;
		while (GetTimeCount()<frame)		// don't go too fast
            SDL_Delay(5);
	} while (1);

finished:
    VL_UnlockSurface(source);
    VL_UnlockSurface(dest);
    SDL_UpdateRect(dest, 0, 0, 0, 0);
	return false;
}
