#ifdef GP2x
#include <sys/mman.h>
#include <unistd.h>
#endif
#include "wl_def.h"

int ChunksInFile;
int PMSpriteStart;
int PMSoundStart;
word *PageLengths;
uint32_t *PMPages;

#ifdef GP2x         // Use upper memory on GP2X

static void* map_upper_mem()
{
    int fd = open("/dev/mem", O_RDWR);
    if (fd == -1) return 0;
    void *upper_mem = mmap(0, 32 * 1024 * 1024, PROT_READ | PROT_WRITE,
        MAP_SHARED, fd, 0x02000000);
    close(fd);
    if (upper_mem == MAP_FAILED)
        return 0;
    return upper_mem;
}

static void unmap_upper_mem(void *mem)
{
    munmap(mem, 32 * 1024 * 1024);
}

static void PML_AllocPagesBuffer(int numChunks)
{
    if(numChunks * 4096 > 16 * 1024 * 1024)
        Quit("Page file needs more than 16 MB!");
    PMPages = (uint32_t *) map_upper_mem();       // use 0x02000000 - 0x03000000 for pages
}

static void PML_FreePagesBuffer()
{
    unmap_upper_mem(PMPages);
}

#else

static void PML_AllocPagesBuffer(int numChunks)
{
    PMPages = (uint32_t *) malloc(numChunks * PMPageSize);
}

static void PML_FreePagesBuffer()
{
    free(PMPages);
}

#endif

void PM_Startup()
{
    char fname[13] = "vswap.";
    strcat(fname, extension);

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

    PML_AllocPagesBuffer(ChunksInFile);
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
    PML_FreePagesBuffer();
}
