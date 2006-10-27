// ID_VH.C

#include "wl_def.h"

#define	SCREENWIDTH		80
#define CHARWIDTH		2
#define TILEWIDTH		4
#define GRPLANES		4
#define BYTEPIXELS		4

#define SCREENXMASK		(~3)
#define SCREENXPLUS		(3)
#define SCREENXDIV		(4)

#define VIEWWIDTH		80

#define PIXTOBLOCK		4		// 16 pixels to an update block

byte	update[UPDATEHIGH][UPDATEWIDE];

//==========================================================================

pictabletype	*pictable;
//unsigned latchpics[NUMLATCHPICS];
SDL_Surface *latchpics[NUMLATCHPICS];

int fizzleA, fizzleC, fizzleM;

int	px,py;
byte	fontcolor,backcolor;
int	fontnumber;
int bufferwidth,bufferheight;

//==========================================================================

void VW_DrawPropString (char *string)
{
	fontstruct *font;
	int		width,step,height;
	byte	*source, *dest, *origdest;
	byte	ch;

    byte *vbuf = VL_LockSurface(backgroundSurface);

	font = (fontstruct *)grsegs[STARTFONT+fontnumber];
	height = bufferheight = font->height;
	dest = origdest = vbuf+py*backgroundPitch+px;

	while ((ch = *string++)!=0)
	{
		width = step = font->width[ch];
		source = ((byte *)font)+font->location[ch];
		while (width--)
		{
			for(int i=0;i<height;i++)
			{
				if(source[i*step])
					dest[i*backgroundPitch]=fontcolor;
			}

			source++;
			px++;
			dest++;
		}
	}
	bufferheight = height;
	bufferwidth = ((dest+1)-origdest)*4;

	VL_UnlockSurface(backgroundSurface);
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
#ifdef NOTYET
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
#endif
}

void VWL_MeasureString (char *string, word *width, word *height, fontstruct *font)
{
	*height = font->height;
	for (*width = 0;*string;string++)
		*width += font->width[*((byte *)string)];	// proportional width
}

void VW_MeasurePropString (char *string, word *width, word *height)
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
	#endif
	SDL_BlitSurface(backgroundSurface, NULL, screen, NULL);
	SDL_Flip(screen);
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

void VWB_DrawTile8M (byte *vbuf, int pitch, int x, int y, int tile)
{
//	if (VW_MarkUpdateBlock (x,y,x+7,y+7))
		VL_MemToScreen (vbuf, pitch, ((byte *)grsegs[STARTTILE8M])+tile*64,8,8,x,y);
}

void VWB_DrawPic (byte *vbuf, int pitch, int x, int y, int chunknum)
{
	int	picnum = chunknum - STARTPICS;
	unsigned width,height;

	x &= ~7;

	width = pictable[picnum].width;
	height = pictable[picnum].height;

//	if (VW_MarkUpdateBlock (x,y,x+width-1,y+height-1))
		VL_MemToScreen (vbuf, pitch, grsegs[chunknum],width,height,x,y);
}

void VWB_DrawPropString	 (char *string)
{
	int x;
	x=px;
	VW_DrawPropString (string);
//	VW_MarkUpdateBlock(x,py,px-1,py+bufferheight-1);
}

void VWB_Bar (byte *vbuf, int pitch, int x, int y, int width, int height, int color)
{
//	if (VW_MarkUpdateBlock (x,y,x+width,y+height-1) )
		VW_Bar (vbuf, pitch,x,y,width,height,color);
}

void VWB_Plot (byte *vbuf, int pitch, int x, int y, int color)
{
//	if (VW_MarkUpdateBlock (x,y,x,y))
		VW_Plot(vbuf,pitch,x,y,color);
}

void VWB_Hlin (byte *vbuf, int pitch, int x1, int x2, int y, int color)
{
//	if (VW_MarkUpdateBlock (x1,y,x2,y))
		VW_Hlin(vbuf,pitch,x1,x2,y,color);
}

void VWB_Vlin (byte *vbuf, int pitch, int y1, int y2, int x, int color)
{
//	if (VW_MarkUpdateBlock (x,y1,x,y2))
		VW_Vlin(vbuf,pitch,y1,y2,x,color);
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
    VL_LatchToScreen(latchpics[2+picnum-LATCHPICS_LUMP_START], x*8, y);
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
	byte	*src;
	unsigned destoff;

//
// tile 8s
//
    SDL_Surface *surf = SDL_CreateRGBSurface(SDL_HWSURFACE, 8*8,
        ((NUMTILE8+7)/8)*8, 8, 0, 0, 0, 0);
    if(surf == NULL)
    {
        Quit("Unable to create surface for picture!");
    }
    SDL_SetColors(surf, gamepal, 0, 256);
	latchpics[0] = surf;
	CA_CacheGrChunk (STARTTILE8);
	src = grsegs[STARTTILE8];
	destoff = freelatch;

	byte *surfmem = VL_LockSurface(surf);

	for (i=0;i<NUMTILE8;i++)
	{
		int offset = (i>>3)*8*64+(i&7)*8;
		for(int y=0; y<8; y++)
		{
            for(int x=0; x<8; x++)
            {
                surfmem[y*surf->pitch+x+offset] = src[(y*2+(x>>2))+(x&3)*2*8];
            }
		}
		src += 64;
	}
	VL_UnlockSurface(surf);

	UNCACHEGRCHUNK (STARTTILE8);

#if 0	// ran out of latch space!
//
// tile 16s
//
	src = (byte _seg *)grsegs[STARTTILE16];
	latchpics[1] = destoff;

	for (i=0;i<NUMTILE16;i++)
	{
		CA_CacheGrChunk (STARTTILE16+i);
		src = (byte _seg *)grsegs[STARTTILE16+i];
		VL_MemToLatch (src,16,16,destoff);
		destoff+=64;
		if (src)
			UNCACHEGRCHUNK (STARTTILE16+i);
	}
#endif

//
// pics
//
	start = LATCHPICS_LUMP_START;
	end = LATCHPICS_LUMP_END;

	for (i=start;i<=end;i++)
	{
		width = pictable[i-STARTPICS].width;
		height = pictable[i-STARTPICS].height;
		SDL_Surface *surf = SDL_CreateRGBSurface(SDL_HWSURFACE, width,
            height, 8, 0, 0, 0, 0);
        if(surf == NULL)
        {
            Quit("Unable to create surface for picture!");
        }
        SDL_SetColors(surf, gamepal, 0, 256);
        latchpics[2+i-start] = surf;
		CA_CacheGrChunk (i);
		byte *surfmem = VL_LockSurface(surf);
		for(int y=0; y<height; y++)
		{
            for(int x=0; x<width; x++)
            {
                surfmem[y*surf->pitch+x] = grsegs[i][(y*(width>>2)+(x>>2))+(x&3)*(width>>2)*height];
            }
		}
		VL_UnlockSurface(surf);
		UNCACHEGRCHUNK(i);
	}
}

//==========================================================================

void InitFizzleFade()
{
    fizzleTempSurface = screen;
    fizzleTempPitch = screenpitch;
    screen = fizzleSurface;
    screenpitch = fizzlePitch;
    screenofs = viewscreeny * fizzlePitch + viewscreenx;
    fizzlein = true;
}

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

boolean FizzleFade (int sx, int sy, unsigned width, unsigned height,
    unsigned frames, boolean abortable)
{
#ifndef NOTYET          // only for 320*200
	int			pixperframe;
	unsigned drawofs;
	unsigned x,y,p,frame;
	long		rndval;

    if(!fizzlein)
    {
        printf("FizzleFade without InitFizzleIn!!");
        return false;
    }
    screen = fizzleTempSurface;     // restore screen surface and pitch
    screenpitch = fizzleTempPitch;
    screenofs = viewscreeny * screenpitch + viewscreenx;
    fizzlein = false;

//	rndval = 1;
    rndval = 0;
//	x = 0;
//	y = 0;
	pixperframe = (screenwidth*screenheight)/frames;

	IN_StartAck ();

    SDL_BlitSurface(screen, NULL, fizzleSurface2, NULL);
    byte *srcptr = VL_LockSurface(fizzleSurface);
	frame=GetTicks();
	do
	{
		if (abortable && IN_CheckAck())
		{
            VL_UnlockSurface(fizzleSurface);
			return true;
		}

        byte *destptr = VL_LockSurface(fizzleSurface2);

		for (p=0; p<pixperframe; p++)
		{
			//
			// seperate random value into x/y pair
			//
			x = rndval >> yb;
			y = rndval & ((1 << yb) - 1);
			rndval = (rndval >> 1) ^ (rndval & 1 ? 0 : rndmask);

/*			x = (rndval&0xffff00)>>8;
            y = (rndval&0xff)-1;
            if (rndval&1)
                rndval = (rndval>>1) ^ 0x00012000;
            else
                rndval >>= 1;*/

/*            x = rndval / screenheight;
            y = rndval % screenheight;

            // perhaps ((fizzleA * rndval) % fizzleM + fizzleC) % fizzleM
            // to avoid overflow?
            rndval = (fizzleA * rndval + fizzleC) % fizzleM;*/

			if (x>width || y>height)
			{
			    p--;
				continue;
			}

			//
			// copy one pixel
			//

			*(byte *)(destptr+(sy+y)*backgroundPitch + sx + x)
                = *(byte *)(srcptr+(sy+y)*fizzlePitch + sx + x);

//			if (rndval == 1)		// entire sequence has been completed
            if (rndval == 0)		// entire sequence has been completed
			{
                VL_UnlockSurface(fizzleSurface);
                VL_UnlockSurface(fizzleSurface2);
                SDL_BlitSurface(fizzleSurface, NULL, screen, NULL);
                SDL_Flip(screen);
                SDL_BlitSurface(fizzleSurface, NULL, screen, NULL);
				return false;
			}
		}
		VL_UnlockSurface(fizzleSurface2);
		SDL_BlitSurface(fizzleSurface2, NULL, screen, NULL);
		SDL_Flip(screen);
		frame++;
		while (GetTicks()<frame)		// don't go too fast
            ;
	} while (1);
#else
	int			pixperframe;
	unsigned drawofs,pagedelta;
	byte 		mask;
	unsigned x,y,p,frame;
	long		rndval;

	pagedelta = dest-source;
	rndval = 1;
	x = 0;
	y = 0;
	pixperframe = 64000/frames;

	IN_StartAck ();

	TimeCount=frame=0;
	do
	{
		if (abortable && IN_CheckAck () )
			return true;

		for (p=0;p<pixperframe;p++)
		{
			//
			// seperate random value into x/y pair
			//
			_asm {
					mov	ax,[WORD PTR rndval]
					mov	dx,[WORD PTR rndval+2]
					mov	ebx,eax
					dec	bl
					mov	[BYTE PTR y],bl			// low 8 bits - 1 = y xoordinate
					mov	ebx,eax
					mov	ecx,edx
					mov	[BYTE PTR x],ah			// next 9 bits = x xoordinate
					mov	[BYTE PTR x+1],dl
			//
			// advance to next random element
			//
					shr	dx,1
					rcr	ax,1
					jnc	noxor
					xor	edx,0x0001
					xor	eax,0x2000
noxor:
					mov	[WORD PTR rndval],ax
					mov	[WORD PTR rndval+2],dx
			}

			if (x>width || y>height)
				continue;
			drawofs = source + y*80 + (x>>2);

			//
			// copy one pixel
			//
			mask = (byte) x&3;
			VGAREADMAP(mask);
			VGAMAPMASK(1<<mask);

			*(byte *)(0xa0000+drawofs+pagedelta)=*(byte *)(0xa0000+drawofs);

			if (rndval == 1)		// entire sequence has been completed
				return false;
		}
		frame++;
		while (TimeCount<frame)		// don't go too fast
		;
	} while (1);
#endif
    return false;
}
