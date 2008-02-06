#ifndef __ID_PM__
#define __ID_PM__

#ifdef USE_HIRES
#define PMPageSize 16384
#else
#define PMPageSize 4096
#endif

extern int ChunksInFile;
extern int PMSpriteStart;
extern int PMSoundStart;
extern word *PageLengths;
extern uint32_t *PMPages;

void PM_Startup();
void PM_Shutdown();

static inline uint32_t *PM_GetPage(int page)
{
    return PMPages + page * (PMPageSize / 4);
}

static inline byte *PM_GetTexture(int wallpic)
{
    return (byte *) PM_GetPage(wallpic);
}

static inline uint32_t *PM_GetSprite(int shapenum)
{
    return PM_GetPage(PMSpriteStart + shapenum);
}

static inline byte *PM_GetSound(int soundnum)
{
    return (byte *) PM_GetPage(PMSoundStart + soundnum);
}

#endif
