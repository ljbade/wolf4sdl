// ID_VL.H

// wolf compatability

#define MS_Quit	Quit

void Quit (const char *error,...);

//===========================================================================


#define SC_INDEX			0x3C4
#define SC_RESET			0
#define SC_CLOCK			1
#define SC_MAPMASK			2
#define SC_CHARMAP			3
#define SC_MEMMODE			4

#define CRTC_INDEX			0x3D4
#define CRTC_H_TOTAL		0
#define CRTC_H_DISPEND		1
#define CRTC_H_BLANK		2
#define CRTC_H_ENDBLANK		3
#define CRTC_H_RETRACE		4
#define CRTC_H_ENDRETRACE 	5
#define CRTC_V_TOTAL		6
#define CRTC_OVERFLOW		7
#define CRTC_ROWSCAN		8
#define CRTC_MAXSCANLINE 	9
#define CRTC_CURSORSTART 	10
#define CRTC_CURSOREND		11
#define CRTC_STARTHIGH		12
#define CRTC_STARTLOW		13
#define CRTC_CURSORHIGH		14
#define CRTC_CURSORLOW		15
#define CRTC_V_RETRACE		16
#define CRTC_V_ENDRETRACE 	17
#define CRTC_V_DISPEND		18
#define CRTC_OFFSET			19
#define CRTC_UNDERLINE		20
#define CRTC_V_BLANK		21
#define CRTC_V_ENDBLANK		22
#define CRTC_MODE			23
#define CRTC_LINECOMPARE 	24


#define GC_INDEX			0x3CE
#define GC_SETRESET			0
#define GC_ENABLESETRESET 	1
#define GC_COLORCOMPARE		2
#define GC_DATAROTATE		3
#define GC_READMAP			4
#define GC_MODE				5
#define GC_MISCELLANEOUS 	6
#define GC_COLORDONTCARE 	7
#define GC_BITMASK			8

#define ATR_INDEX			0x3c0
#define ATR_MODE			16
#define ATR_OVERSCAN		17
#define ATR_COLORPLANEENABLE 18
#define ATR_PELPAN			19
#define ATR_COLORSELECT		20

#define	STATUS_REGISTER_1   0x3da

#define PEL_WRITE_ADR		0x3c8
#define PEL_READ_ADR		0x3c7
#define PEL_DATA			0x3c9


//===========================================================================

#define CHARWIDTH		2
#define TILEWIDTH		4

//===========================================================================

extern SDL_Surface *screen, *screenBuffer, *curSurface;

//extern	unsigned bufferofs;			// all drawing is reletive to this
extern	unsigned isplayofs,pelpan;	    // last setscreen coordinates

//extern	unsigned screenseg;			// set to 0xa000 for asm convenience

extern	unsigned screenWidth, screenHeight, screenPitch, bufferPitch, curPitch;
//extern	unsigned ylookup[MAXSCANLINES];

extern	boolean  screenfaded;
extern	unsigned bordercolor;

//===========================================================================

//
// VGA hardware routines
//

#define VL_WaitVBL(a) SDL_Delay((a)*20)

/*void VL_Startup (void);

void VL_SetVGAPlane (void);
void VL_DePlaneVGA (void);
void VL_ClearVideo (byte color);

void VL_SetLineWidth (unsigned width);
void VL_SetSplitScreen (int linenum);

void VL_WaitVBL (int vbls);
void VL_CrtcStart (int crtc);
void VL_SetScreen (int crtc, int pelpan);*/

void VL_SetVGAPlaneMode (void);
void VL_SetTextMode (void);
void VL_Shutdown (void);

void VL_FillPalette (int red, int green, int blue);
void VL_SetColor	(int color, int red, int green, int blue);
void VL_GetColor	(int color, int *red, int *green, int *blue);
void VL_SetPalette (SDL_Color *palette);
void VL_GetPalette (SDL_Color *palette);
void VL_FadeOut (int start, int end, int red, int green, int blue, int steps);
void VL_FadeIn (int start, int end, SDL_Color *palette, int steps);
//void VL_ColorBorder (int color);

byte *VL_LockSurface(SDL_Surface *surface);
void VL_UnlockSurface(SDL_Surface *surface);

#define LOCK() VL_LockSurface(curSurface)
#define UNLOCK() VL_UnlockSurface(curSurface)

byte VL_GetPixel (int x, int y);
void VL_Plot (int x, int y, int color);
void VL_Hlin (unsigned x, unsigned y, unsigned width, int color);
void VL_Vlin (int x, int y, int height, int color);
void VL_Bar (int x, int y, int width, int height, int color);

void VL_MungePic (byte *source, unsigned width, unsigned height);
void VL_DrawPicBare (int x, int y, byte *pic, int width, int height);
void VL_MemToLatch(byte *source, int width, int height,
    SDL_Surface *destSurface, int x, int y);
void VL_ScreenToScreen (SDL_Surface *source, SDL_Surface *dest);
void VL_MemToScreen (byte *source, int width, int height, int x, int y);
void VL_MaskedToScreen (byte *source, int width, int height, int x, int y);
void VL_LatchToScreen (SDL_Surface *source, int x, int y);
void VL_LatchToScreen (SDL_Surface *source, int xsrc, int ysrc,
    int width, int height, int xdest, int ydest);

/*void VL_DrawTile8String (char *str, char *tile8ptr, int printx, int printy);
void VL_DrawLatch8String (char *str, unsigned tile8ptr, int printx, int printy);
void VL_SizeTile8String (char *str, int *width, int *height);
void VL_DrawPropString (char *str, unsigned tile8ptr, int printx, int printy);
void VL_SizePropString (char *str, int *width, int *height, char far *font);

void VL_TestPaletteSet (void);*/
