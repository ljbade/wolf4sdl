// ID_VH.H

#pragma pack(1)

#define WHITE			15			// graphics mode independant colors
#define BLACK			0
#define FIRSTCOLOR		1
#define SECONDCOLOR		12
#define F_WHITE			15
#define F_BLACK			0
#define F_FIRSTCOLOR	1
#define F_SECONDCOLOR	12

//===========================================================================

#define MAXSHIFTS	1

typedef struct
{
  short	width,
	height,
	orgx,orgy,
	xl,yl,xh,yh,
	shifts;
} spritetabletype;

typedef	struct
{
	word sourceoffset[MAXSHIFTS];
	word planesize[MAXSHIFTS];
	word width[MAXSHIFTS];
	byte data[];
} spritetype;		// the memptr for each sprite points to this

typedef struct
{
	short width,height;
} pictabletype;


typedef struct
{
	short height;
	short location[256];
	char width[256];
} fontstruct;


//===========================================================================


extern	pictabletype	*pictable;
extern	pictabletype	 *picmtable;
extern	spritetabletype *spritetable;

extern	byte	fontcolor;
extern	int	fontnumber;
extern	int	px,py;

//
// Double buffer management routines
//

int	 VW_MarkUpdateBlock (int x1, int y1, int x2, int y2);

//
// mode independant routines
// coordinates in pixels, rounded to best screen res
// regions marked in double buffer
//

void VWB_DrawPropString	 (char *string);

void VWB_DrawTile8 (int x, int y, int tile);
void VWB_DrawTile8M (byte *vbuf, int x, int y, int tile);
void VWB_DrawTile16 (int x, int y, int tile);
void VWB_DrawTile16M (int x, int y, int tile);
void VWB_DrawPic (byte *vbuf, int pitch, int x, int y, int chunknum);
void VWB_DrawMPic(int x, int y, int chunknum);
void VWB_Bar (byte *vbuf, int pitch, int x, int y, int width, int height, int color);
void VWB_Plot (byte *vbuf, int pitch, int x, int y, int color);
void VWB_Hlin (byte *vbuf, int pitch, int x1, int x2, int y, int color);
void VWB_Vlin (byte *vbuf, int pitch, int y1, int y2, int x, int color);

#define VWBL_DrawPic(x,y,chunknum) VWB_DrawPic(VL_LockSurface(backgroundSurface), backgroundPitch, (x), (y), (chunknum)),\
    VL_UnlockSurface(backgroundSurface)
#define VWBL_Bar(x,y,width,height,color) VWB_Bar(VL_LockSurface(backgroundSurface), backgroundPitch, (x), (y), (width), (height), (color)),\
    VL_UnlockSurface(backgroundSurface)
#define VWBL_Hlin(x1,x2,y,color) VWB_Hlin(VL_LockSurface(backgroundSurface), backgroundPitch, (x1), (x2), (y), (color)),\
    VL_UnlockSurface(backgroundSurface)
#define VWBL_Vlin(y1,y2,x,color) VWB_Vlin(VL_LockSurface(backgroundSurface), backgroundPitch, (y1), (y2), (x), (color)),\
    VL_UnlockSurface(backgroundSurface)

void VH_UpdateScreen();
#define VW_UpdateScreen VH_UpdateScreen

//
// wolfenstein EGA compatability stuff
//
extern SDL_Color gamepal[256];

void VH_SetDefaultColors (void);

#define VWL_Hlin(x,z,y,c)	VL_Hlin(VL_LockSurface(screen), screenpitch, x,y,(z)-(x)+1,c),\
    VL_UnlockSurface(screen)
#define VWL_Vlin(y,z,x,c)	VL_Vlin(VL_LockSurface(screen), screenpitch, x,y,(z)-(y)+1,c),\
    VL_UnlockSurface(screen)
#define VWL_Bar(x,y,w,h,c)  VL_Bar(VL_LockSurface(screen), screenpitch, x, y, w, h, c),\
    VL_UnlockSurface(screen)

#define VW_Startup		VL_Startup
#define VW_Shutdown		VL_Shutdown
#define VW_SetCRTC		VL_SetCRTC
#define VW_SetScreen	VL_SetScreen
#define VW_Bar			VL_Bar
#define VW_Plot			VL_Plot
#define VW_Hlin(vbuf,pitch,x,z,y,c)	VL_Hlin(vbuf,pitch,x,y,(z)-(x)+1,c)
#define VW_Vlin(vbuf,pitch,y,z,x,c)	VL_Vlin(vbuf,pitch,x,y,(z)-(y)+1,c)
#define VW_DrawPic		VH_DrawPic
#define VW_SetSplitScreen	VL_SetSplitScreen
#define VW_SetLineWidth		VL_SetLineWidth
#define VW_ColorBorder	VL_ColorBorder
#define VW_WaitVBL		VL_WaitVBL
#define VW_FadeIn()		VL_FadeIn(0,255,gamepal,30);
#define VW_FadeOut()	VL_FadeOut(0,255,0,0,0,30);
#define VW_ScreenToScreen	VL_ScreenToScreen
#define VW_SetDefaultColors	VH_SetDefaultColors
void	VW_MeasurePropString (char *string, word *width, word *height);
#define EGAMAPMASK(x)	VGAMAPMASK(x)
#define EGAWRITEMODE(x)	VGAWRITEMODE(x)

//#define VW_MemToScreen	VL_MemToLatch

#define MS_Quit			Quit


#define LatchDrawChar(x,y,p) VL_LatchToScreen(latchpics[0],((p)&7)*8,((p)>>3)*8*64,8,8,x,y)
#define LatchDrawTile(x,y,p) VL_LatchToScreen(latchpics[1],(p)*64,0,16,16,x,y)

void LatchDrawPic (unsigned x, unsigned y, unsigned picnum);
void LoadLatchMem (void);

void InitFizzleFade();
boolean FizzleFade (int x, int y, unsigned width, unsigned height,
    unsigned frames, boolean abortable);


#define NUMLATCHPICS	100
extern	SDL_Surface *latchpics[NUMLATCHPICS];
extern	unsigned freelatch;

extern int fizzleA, fizzleC, fizzleM;
