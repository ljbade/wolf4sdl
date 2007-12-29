#include "wl_def.h"


pictabletype	*pictable;
SDL_Surface     *latchpics[NUMLATCHPICS];

int	    px,py;
byte	fontcolor,backcolor;
int	    fontnumber;

//==========================================================================

void VWB_DrawPropString(const char* string)
{
	fontstruct  *font;
	int		    width, step, height;
	byte	    *source, *dest;
	byte	    ch;

    byte *vbuf = LOCK();

	font = (fontstruct *) grsegs[STARTFONT+fontnumber];
	height = font->height;
	dest = vbuf + scaleFactor * (py * curPitch + px);

	while ((ch = *string++)!=0)
	{
		width = step = font->width[ch];
		source = ((byte *)font)+font->location[ch];
		while (width--)
		{
			for(int i=0;i<height;i++)
			{
				if(source[i*step])
                {
                    for(int sy=0; sy<scaleFactor; sy++)
                        for(int sx=0; sx<scaleFactor; sx++)
        					dest[(scaleFactor*i+sy)*curPitch+sx]=fontcolor;
                }
			}

			source++;
			px++;
			dest+=scaleFactor;
		}
	}

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
	SDL_BlitSurface(screenBuffer, NULL, screen, NULL);
	SDL_UpdateRect(screen, 0, 0, 0, 0);
}


void VWB_DrawTile8 (int x, int y, int tile)
{
		LatchDrawChar(x,y,tile);
}

void VWB_DrawTile8M (int x, int y, int tile)
{
		VL_MemToScreen (((byte *)grsegs[STARTTILE8M])+tile*64,8,8,x,y);
}

void VWB_DrawPic (int x, int y, int chunknum)
{
	int	picnum = chunknum - STARTPICS;
	unsigned width,height;

	x &= ~7;

	width = pictable[picnum].width;
	height = pictable[picnum].height;

		VL_MemToScreen (grsegs[chunknum],width,height,x,y);
}

void VWB_DrawPicScaledCoord (int scx, int scy, int chunknum)
{
	int	picnum = chunknum - STARTPICS;
	unsigned width,height;

	width = pictable[picnum].width;
	height = pictable[picnum].height;

    VL_MemToScreenScaledCoord (grsegs[chunknum],width,height,scx,scy);
}


void VWB_Bar (int x, int y, int width, int height, int color)
{
		VW_Bar (x,y,width,height,color);
}

void VWB_Plot (int x, int y, int color)
{
    if(scaleFactor == 1)
        VW_Plot(x,y,color);
    else
        VW_Bar(x, y, 1, 1, color);
}

void VWB_Hlin (int x1, int x2, int y, int color)
{
    if(scaleFactor == 1)
    	VW_Hlin(x1,x2,y,color);
    else
        VW_Bar(x1, y, x2-x1+1, 1, color);
}

void VWB_Vlin (int y1, int y2, int x, int color)
{
    if(scaleFactor == 1)
		VW_Vlin(y1,y2,x,color);
    else
        VW_Bar(x, y1, 1, y2-y1+1, color);
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

void LatchDrawPicScaledCoord (unsigned scx, unsigned scy, unsigned picnum)
{
	VL_LatchToScreenScaledCoord (latchpics[2+picnum-LATCHPICS_LUMP_START], scx*8, scy);
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
