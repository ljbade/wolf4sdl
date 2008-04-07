#include "wl_def.h"

int ChunksInFile;
int PMSpriteStart;
int PMSoundStart;
word *PageLengths;
uint32_t *PMPages;

void PM_Startup()
{
    char fname[13] = "vswap.";
    strcat(fname,extension);

    FILE *file = fopen(fname,"rb");
    if(!file)
        CA_CannotOpen(fname);

    ChunksInFile = 0;
    fread(&ChunksInFile, sizeof(word), 1, file);
    PMSpriteStart = 0;
    fread(&PMSpriteStart, sizeof(word), 1, file);
    PMSoundStart = 0;
    fread(&PMSoundStart, sizeof(word), 1, file);

    int32_t* pageOffsets = (int32_t *) malloc(ChunksInFile * sizeof(int32_t));
    CHECKMALLOCRESULT(pageOffsets);
    fread(pageOffsets, sizeof(int32_t), ChunksInFile, file);

    PageLengths = (word *) malloc(ChunksInFile * sizeof(word));
    CHECKMALLOCRESULT(PageLengths);
    fread(PageLengths, sizeof(word), ChunksInFile, file);

    // TODO: Doesn't support variable page lengths as used by the sounds (page length always <=4096 there)

    PMPages = (uint32_t *) malloc(ChunksInFile * PMPageSize);
    CHECKMALLOCRESULT(PMPages);
    for(int i = 0; i < ChunksInFile; i++)
    {
        fseek(file, pageOffsets[i], SEEK_SET);
        fread(PMPages + i * (PMPageSize / 4), PMPageSize, 1, file);
    }

    free(pageOffsets);
    fclose(file);
}

void PM_Shutdown()
{
    free(PageLengths);
    free(PMPages);
}
