// ID_VL.C

#include <dos.h>
#include <mem.h>
#include <string.h>
#include <conio.h>
#include "wl_def.h"
#pragma hdrstop

//
// SC_INDEX is expected to stay at SC_MAPMASK for proper operation
//

//SDL_Surface *realscreen = NULL;
SDL_Surface *screen = NULL;
SDL_Surface *backgroundSurface = NULL;
SDL_Surface *fizzleSurface = NULL;
SDL_Surface *fizzleSurface2 = NULL;
SDL_Surface *fizzleTempSurface = NULL;
int fizzleTempPitch = 0;

//byte *vbuf = NULL;

boolean fullscreen = false;
int screenwidth = 640; //320;
int screenheight = 400; //200;
int screenpitch;
int backgroundPitch;
int fizzlePitch;

boolean screenfaded;
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
    //SDL_Quit();   ??
}


/*
=======================
=
= VL_SetVGAPlaneMode
=
=======================
*/

void VL_SetVGAPlaneMode (void)
{
    screen = SDL_SetVideoMode(screenwidth, screenheight, 8,
        SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_HWPALETTE | (fullscreen ? SDL_FULLSCREEN : 0));
    if(!screen)
    {
        printf("Unable to set %ix%i video: %s\n", screenwidth, screenheight, SDL_GetError());
        exit(1);
    }
    SDL_ShowCursor(SDL_DISABLE);

    for(int i=0; i<256; i++)
    {
        gamepal[i].r = (gamepal[i].r * 255) / 63;
        gamepal[i].g = (gamepal[i].g * 255) / 63;
        gamepal[i].b = (gamepal[i].b * 255) / 63;
    }
    int ret = SDL_SetColors(screen, gamepal, 0, 256);
    printf("SDL_SetColors(screen, gamepal, 0, 256) returned %i\n",ret);

    backgroundSurface = SDL_CreateRGBSurface(SDL_HWSURFACE, screenwidth,
        screenheight, 8, 0, 0, 0, 0);
    if(!backgroundSurface)
    {
        printf("Unable to create background surface: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_SetColors(backgroundSurface, gamepal, 0, 256);

	fizzleSurface = SDL_CreateRGBSurface(SDL_HWSURFACE, screenwidth,
        screenheight, 8, 0, 0, 0, 0);
	fizzleSurface2 = SDL_CreateRGBSurface(SDL_HWSURFACE, screenwidth,
        screenheight, 8, 0, 0, 0, 0);
    if(!fizzleSurface || !fizzleSurface2)
    {
        printf("Unable to create two fizzle surfaces: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_SetColors(fizzleSurface, gamepal, 0, 256);
    SDL_SetColors(fizzleSurface2, gamepal, 0, 256);
    memcpy(curpal, gamepal, sizeof(SDL_Color) * 256);

    printf("screen->pitch = %i\nbackgroundSurface->pitch = %i\n"
        "fizzleSurface->pitch = %i\n", screen->pitch,
        backgroundSurface->pitch, fizzleSurface->pitch);

    screenpitch = screen->pitch;
    backgroundPitch = backgroundSurface->pitch;
    fizzlePitch = fizzleSurface->pitch;
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

	for (i=0;i<256;i++)
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

void VL_SetColor (int color, int red, int green, int blue)
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
	//SDL_SetColors(screen, palette, 0, 256);
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
	int i,j,orig,delta;
	SDL_Color *origptr, *newptr;

	VL_WaitVBL(1);
	VL_GetPalette (palette1);
	memcpy (palette2, palette1, sizeof(SDL_Color) * 256);

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
        SDL_SetPalette(screen, SDL_PHYSPAL, palette2, 0, 256);
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
	int		i,j,delta;

	VL_WaitVBL(1);
	VL_GetPalette (palette1);
	memcpy (palette2, palette1, sizeof(SDL_Color) * 256);

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
        SDL_SetPalette(screen, SDL_PHYSPAL, palette2, 0, 256);
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

byte *VL_LockSurface(SDL_Surface *screen)
{
    if(SDL_MUSTLOCK(screen))
    {
        if(SDL_LockSurface(screen) < 0)
            return NULL;
    }
    return (byte *) screen->pixels;
}

void VL_UnlockSurface(SDL_Surface *screen)
{
    if(SDL_MUSTLOCK(screen))
    {
        SDL_UnlockSurface(screen);
    }
}

/*
=================
=
= VL_Plot   ( screen must be locked!! )
=
=================
*/

void VL_Plot (byte *vbuf, int pitch, int x, int y, int color)
{
    vbuf[y*pitch + x] = color;
}


/*
=================
=
= VL_Hlin   ( screen must be locked!! )
=
=================
*/

void VL_Hlin (byte *vbuf, int pitch, unsigned x, unsigned y, unsigned width, int color)
{
	Uint8 *dest = vbuf + y*pitch+ x;
	memset(dest, color, width);
}


/*
=================
=
= VL_Vlin   ( screen must be locked!! )
=
=================
*/

void VL_Vlin (byte *vbuf, int pitch, int x, int y, int height, int color)
{
	Uint8 *dest = vbuf + y*pitch + x;

	while (height--)
	{
		*dest = color;
		dest += pitch;
	}
}


/*
=================
=
= VL_Bar
=
=================
*/

void VL_Bar (byte *vbuf, int pitch, int x, int y, int width, int height, int color)
{
	Uint8 *dest = vbuf + y*pitch + x;

	while (height--)
	{
		memset (dest,color,width);
		dest += pitch;
	}
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
=================
*/

void VL_MemToLatch (byte *source, int width, int height, unsigned dest)
{
#ifdef NOTYET
	unsigned count;
	byte	plane,mask;

	count = ((width+3)/4)*height;
	mask = 1;
	for (plane = 0; plane<4 ; plane++)
	{
		VGAMAPMASK(mask);
		mask <<= 1;

		memcpy((byte *)(0xa0000+dest),source,count);

		source+= count;
	}
#endif
}


//===========================================================================


/*
=================
=
= VL_MemToScreen
=
= Draws a block of data to the screen.
=
=================
*/

void VL_MemToScreen (byte *vbuf, int pitch, byte *source, int width, int height, int x, int y)
{
    for(int j=0; j<height; j++)
    {
        for(int i=0; i<width; i++)
        {
            vbuf[(j+y)*pitch+i+x] = source[(j*(width>>2)+(i>>2))+(i&3)*(width>>2)*height];
        }
	}
}

//==========================================================================

/*
=================
=
= VL_LatchToScreen
=
=================
*/

#if 0
void VL_LatchToScreen (unsigned source, int width, int height, int x, int y)
{
	VGAWRITEMODE(1);
	VGAMAPMASK(15);

	byte *dest=vbuf+y*80+(x>>2);
	byte *src=(byte *)(0xa0000+source);

	_asm {
		mov	edi,dest
		mov	esi,src
		mov	ebx,linewidth
		mov	eax,width
		sub	ebx,eax
		mov	edx,height

drawline:
		mov	ecx,eax
		rep	movsb
		add	edi,ebx
		dec	edx
		jnz	drawline
	}

	VGAWRITEMODE(0);
}
#endif

void VL_LatchToScreen(SDL_Surface *source, int x, int y)
{
    SDL_Rect destrect = { x, y, 0, 0 };
//    SDL_BlitSurface(source, NULL, screen, &destrect);
    SDL_BlitSurface(source, NULL, backgroundSurface, &destrect);
}

void VL_LatchToScreen(SDL_Surface *source, int x1, int y1, int width, int height, int x2, int y2)
{
    SDL_Rect srcrect = { x1, y1, width, height };
    SDL_Rect destrect = { x2, y2, 0, 0 };
//    SDL_BlitSurface(source, &srcrect, screen, &destrect);
    SDL_BlitSurface(source, &srcrect, backgroundSurface, &destrect);
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
