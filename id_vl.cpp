// ID_VL.C

#include <string.h>
#include "wl_def.h"
#pragma hdrstop

boolean fullscreen = true;
unsigned screenWidth = 640;
unsigned screenHeight = 400;

SDL_Surface *screen = NULL;
unsigned screenPitch;

SDL_Surface *screenBuffer = NULL;
unsigned bufferPitch;

SDL_Surface *curSurface = NULL;
unsigned curPitch;

unsigned scaleFactor;

boolean	 screenfaded;
unsigned bordercolor;

SDL_Color palette1[256], palette2[256];
SDL_Color curpal[256];

//===========================================================================


/*
=======================
=
= VL_Shutdown
=
=======================
*/

void	VL_Shutdown (void)
{
	//VL_SetTextMode ();
}


/*
=======================
=
= VL_SetVGAPlaneMode
=
=======================
*/

void	VL_SetVGAPlaneMode (void)
{
    SDL_WM_SetCaption("Wolfenstein 3D", NULL);

    screen = SDL_SetVideoMode(screenWidth, screenHeight, 8,
        SDL_SWSURFACE | SDL_HWPALETTE | (fullscreen ? SDL_FULLSCREEN : 0));
    if(!screen)
    {
        printf("Unable to set %ix%ix8 video mode: %s\n", screenWidth,
            screenHeight, SDL_GetError());
        exit(1);
    }
    SDL_ShowCursor(SDL_DISABLE);

    for(int i = 0; i < 256; i++)
    {
        gamepal[i].r = (gamepal[i].r * 255) / 63;
        gamepal[i].g = (gamepal[i].g * 255) / 63;
        gamepal[i].b = (gamepal[i].b * 255) / 63;
    }
    SDL_SetColors(screen, gamepal, 0, 256);
    memcpy(curpal, gamepal, sizeof(SDL_Color) * 256);

    screenBuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, screenWidth,
        screenHeight, 8, 0, 0, 0, 0);
    if(!screenBuffer)
    {
        printf("Unable to create screen buffer surface: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_SetColors(screenBuffer, gamepal, 0, 256);

    screenPitch = screen->pitch;
    bufferPitch = screenBuffer->pitch;

//    curSurface = screen;
//    curPitch = screenPitch;

    curSurface = screenBuffer;
    curPitch = bufferPitch;

    scaleFactor = screenWidth/320;
    if(screenHeight/200 < scaleFactor) scaleFactor = screenHeight/200;

    pixelangle = (short *) malloc(screenWidth * sizeof(short));
    wallheight = (int *) malloc(screenWidth * sizeof(int));
}

/*
=============================================================================

						PALETTE OPS

		To avoid snow, do a WaitVBL BEFORE calling these

=============================================================================
*/


/*
=================
=
= VL_FillPalette
=
=================
*/

void VL_FillPalette (int red, int green, int blue)
{
	int	i;
	SDL_Color pal[256];

	for(i=0; i<256; i++)
	{
	    pal[i].r = red;
	    pal[i].g = green;
	    pal[i].b = blue;
	}

	SDL_SetPalette(screen, SDL_PHYSPAL, pal, 0, 256);
    memcpy(curpal, pal, sizeof(SDL_Color) * 256);
}

//===========================================================================

/*
=================
=
= VL_SetColor
=
=================
*/

void VL_SetColor	(int color, int red, int green, int blue)
{
    SDL_Color col = { red, green, blue };
    SDL_SetPalette(screen, SDL_PHYSPAL, &col, color, 1);
    curpal[color] = col;
}

//===========================================================================

/*
=================
=
= VL_GetColor
=
=================
*/

void VL_GetColor	(int color, int *red, int *green, int *blue)
{
    SDL_Color *col = &curpal[color];
    *red = col->r;
    *green = col->g;
    *blue = col->b;
}

//===========================================================================

/*
=================
=
= VL_SetPalette
=
=================
*/

void VL_SetPalette (SDL_Color *palette)
{
    SDL_SetPalette(screen, SDL_PHYSPAL, palette, 0, 256);
    memcpy(curpal, palette, sizeof(SDL_Color) * 256);
}


//===========================================================================

/*
=================
=
= VL_GetPalette
=
=================
*/

void VL_GetPalette (SDL_Color *palette)
{
//    memcpy(palette, screen->format->palette, sizeof(SDL_Color) * 256);
    memcpy(palette, curpal, sizeof(SDL_Color) * 256);
}


//===========================================================================

/*
=================
=
= VL_FadeOut
=
= Fades the current palette to the given color in the given number of steps
=
=================
*/

void VL_FadeOut (int start, int end, int red, int green, int blue, int steps)
{
	int		    i,j,orig,delta;
	SDL_Color   *origptr, *newptr;

	VL_WaitVBL(1);
	VL_GetPalette(palette1);
	memcpy(palette2, palette1, sizeof(SDL_Color) * 256);

//
// fade through intermediate frames
//
	for (i=0;i<steps;i++)
	{
		origptr = &palette1[start];
		newptr = &palette2[start];
		for (j=start;j<=end;j++)
		{
			orig = origptr->r;
			delta = red-orig;
			newptr->r = orig + delta * i / steps;
			orig = origptr->g;
			delta = green-orig;
			newptr->g = orig + delta * i / steps;
			orig = origptr->b;
			delta = blue-orig;
			newptr->b = orig + delta * i / steps;
			origptr++;
			newptr++;
		}

		VL_WaitVBL(1);
		VL_SetPalette (palette2);
	}

//
// final color
//
	VL_FillPalette (red,green,blue);

	screenfaded = true;
}


/*
=================
=
= VL_FadeIn
=
=================
*/

void VL_FadeIn (int start, int end, SDL_Color *palette, int steps)
{
	int i,j,delta;

	VL_WaitVBL(1);
	VL_GetPalette(palette1);
	memcpy(palette2, palette1, sizeof(SDL_Color) * 256);

//
// fade through intermediate frames
//
	for (i=0;i<steps;i++)
	{
		for (j=start;j<=end;j++)
		{
			delta = palette[j].r-palette1[j].r;
			palette2[j].r = palette1[j].r + delta * i / steps;
			delta = palette[j].g-palette1[j].g;
			palette2[j].g = palette1[j].g + delta * i / steps;
			delta = palette[j].b-palette1[j].b;
			palette2[j].b = palette1[j].b + delta * i / steps;
		}

		VL_WaitVBL(1);
		VL_SetPalette(palette2);
	}

//
// final color
//
	VL_SetPalette (palette);
	screenfaded = false;
}

/*
=============================================================================

							PIXEL OPS

=============================================================================
*/

byte *VL_LockSurface(SDL_Surface *surface)
{
    if(SDL_MUSTLOCK(surface))
    {
        if(SDL_LockSurface(surface) < 0)
            return NULL;
    }
    return (byte *) surface->pixels;
}

void VL_UnlockSurface(SDL_Surface *surface)
{
    if(SDL_MUSTLOCK(surface))
    {
        SDL_UnlockSurface(surface);
    }
}

/*
=================
=
= VL_Plot
=
= curScreen must be locked!!!
=
=================
*/

void VL_Plot (int x, int y, int color)
{
    VL_LockSurface(curSurface);
	((byte *) curSurface->pixels)[y * curPitch + x] = color;
    VL_UnlockSurface(curSurface);
}

/*
=================
=
= VL_GetPixel
=
= curScreen must be locked!!!
=
=================
*/

byte VL_GetPixel (int x, int y)
{
    VL_LockSurface(curSurface);
	byte col = ((byte *) curSurface->pixels)[y * curPitch + x];
    VL_UnlockSurface(curSurface);
	return col;
}


/*
=================
=
= VL_Hlin
=
= curScreen must be locked!!!
=
=================
*/

void VL_Hlin (unsigned x, unsigned y, unsigned width, int color)
{
    VL_LockSurface(curSurface);
    Uint8 *dest = ((byte *) curSurface->pixels) + y * curPitch + x;
    memset(dest, color, width);
    VL_UnlockSurface(curSurface);
}


/*
=================
=
= VL_Vlin
=
= curSurface must be locked!!!
=
=================
*/

void VL_Vlin (int x, int y, int height, int color)
{
    VL_LockSurface(curSurface);
    Uint8 *dest = ((byte *) curSurface->pixels) + y * curPitch + x;

	while (height--)
	{
		*dest = color;
		dest += curPitch;
	}
    VL_UnlockSurface(curSurface);
}


/*
=================
=
= VL_Bar
=
= curSurface must be locked!!!
=
=================
*/

void VL_BarScaledCoord (int scx, int scy, int scwidth, int scheight, int color)
{
    VL_LockSurface(curSurface);
    Uint8 *dest = ((byte *) curSurface->pixels) + scy * curPitch + scx;

	while (scheight--)
	{
		memset(dest, color, scwidth);
		dest += curPitch;
	}
    VL_UnlockSurface(curSurface);
}

/*
============================================================================

							MEMORY OPS

============================================================================
*/

/*
=================
=
= VL_MemToLatch
=
= destSurface must be locked!
=
=================
*/

void VL_MemToLatch(byte *source, int width, int height,
    SDL_Surface *destSurface, int x, int y)
{
    VL_LockSurface(destSurface);
    int pitch = destSurface->pitch;
    byte *dest = (byte *) destSurface->pixels + y * pitch + x;
    for(int ysrc = 0; ysrc < height; ysrc++)
    {
        for(int xsrc = 0; xsrc < width; xsrc++)
        {
            dest[ysrc * pitch + xsrc] = source[(ysrc * (width >> 2) + (xsrc >> 2))
                + (xsrc & 3) * (width >> 2) * height];
        }
    }
    VL_UnlockSurface(curSurface);
}

//===========================================================================


/*
=================
=
= VL_MemToScreen
=
= Draws a block of data to the screen.
=
= curSurface must be locked!!!
=
=================
*/

void VL_MemToScreenScaledCoord (byte *source, int width, int height, int scx, int scy)
{
    VL_LockSurface(curSurface);
    byte *vbuf = (byte *) curSurface->pixels;
    for(int j=0,scj=0; j<height; j++, scj+=scaleFactor)
    {
        for(int i=0,sci=0; i<width; i++, sci+=scaleFactor)
        {
            for(unsigned m=0; m<scaleFactor; m++)
            {
                for(unsigned n=0; n<scaleFactor; n++)
                {
                    vbuf[(scj+m+scy)*curPitch+sci+n+scx] = source[(j*(width>>2)+(i>>2))+(i&3)*(width>>2)*height];
                }
            }
        }
    }
    VL_UnlockSurface(curSurface);
}

//==========================================================================

/*
=================
=
= VL_LatchToScreen
=
=================
*/

void VL_LatchToScreenScaledCoord(SDL_Surface *source, int xsrc, int ysrc,
    int width, int height, int scxdest, int scydest)
{
    if(scaleFactor == 1)
    {
        SDL_Rect srcrect = { xsrc, ysrc, width, height };
        SDL_Rect destrect = { scxdest, scydest, 0, 0 }; // width and height are ignored
        SDL_BlitSurface(source, &srcrect, curSurface, &destrect);
    }
    else
    {
        VL_LockSurface(source);
        byte *src = (byte *) source->pixels;
        unsigned srcPitch = source->pitch;

        VL_LockSurface(curSurface);
        byte *vbuf = (byte *) curSurface->pixels;
        for(int j=0,scj=0; j<height; j++, scj+=scaleFactor)
        {
            for(int i=0,sci=0; i<width; i++, sci+=scaleFactor)
            {
                for(unsigned m=0; m<scaleFactor; m++)
                {
                    for(unsigned n=0; n<scaleFactor; n++)
                    {
                        vbuf[(scydest+scj+m)*curPitch+scxdest+sci+n] = src[(ysrc + j)*srcPitch + xsrc + i];
                    }
                }
            }
        }
        VL_UnlockSurface(curSurface);
        VL_UnlockSurface(source);
    }
}

//===========================================================================

/*
=================
=
= VL_ScreenToScreen
=
=================
*/

void VL_ScreenToScreen (SDL_Surface *source, SDL_Surface *dest)
{
    SDL_BlitSurface(source, NULL, dest, NULL);
}
