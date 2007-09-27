#include "wl_def.h"

// defines

/*FILE *grfile=NULL;
FILE *mapfile=NULL;*/

int ChunksInFile;
int PMSpriteStart;
int PMSoundStart;
int32_t *PageOffsets;
word *PageLengths;
byte *Pages;

/*float radtoint;

short minheightdiv;

word wallspot[SCRWIDTH+1];*/

void PM_Startup()
{
        char fname[13] = "vswap.";
        strcat(fname,extension);

        FILE *file=fopen(fname,"rb");
        if(!file)
                CA_CannotOpen(fname);

        ChunksInFile=0;
        fread(&ChunksInFile,sizeof(word),1,file);
        PMSpriteStart=0;
        fread(&PMSpriteStart,sizeof(word),1,file);
        PMSoundStart=0;
        fread(&PMSoundStart,sizeof(word),1,file);

        PageOffsets=(int32_t *) malloc(ChunksInFile*sizeof(int32_t));
        fread(PageOffsets,sizeof(int32_t),ChunksInFile,file);

        PageLengths=(word *) malloc(ChunksInFile*sizeof(word));
        fread(PageLengths,sizeof(word),ChunksInFile,file);

        // TODO: Doesn't support variable page lengths as used by the sounds (page length always <=4096 there)

        Pages=(byte *) malloc(ChunksInFile*4096);
        for(int i=0;i<ChunksInFile;i++)
        {
                fseek(file,PageOffsets[i],SEEK_SET);
                fread(Pages+i*4096,4096,1,file);
        }

        fclose(file);
}

void PM_Shutdown()
{
   free(PageOffsets);
   free(PageLengths);
        free(Pages);
}
