// ID_CA.H
//===========================================================================

#define NUMMAPS         60
#define MAPPLANES       2

#define UNCACHEGRCHUNK(chunk) {if(grsegs[chunk]) {free(grsegs[chunk]); grsegs[chunk]=NULL;}}
#define UNCACHEAUDIOCHUNK(chunk) {if(audiosegs[chunk]) {free(audiosegs[chunk]); audiosegs[chunk]=NULL;}}

//===========================================================================

typedef struct
{
        int32_t            planestart[3];
        word planelength[3];
        word width,height;
        char            name[16];
} maptype;

//===========================================================================

#define BUFFERSIZE 0x1000
extern byte bufferseg[BUFFERSIZE];

extern  char            audioname[13];

extern  byte            *tinf;
extern  int                     mapon;

extern  word *mapsegs[MAPPLANES];
extern  maptype *mapheaderseg[NUMMAPS];
extern  byte *audiosegs[NUMSNDCHUNKS];
extern  byte *grsegs[NUMCHUNKS];

extern  byte            grneeded[NUMCHUNKS];

extern  char            *titleptr[8];

extern  char            extension[5];

extern byte grstarts[(NUMCHUNKS+1)*3];  // array of offsets in egagraph, -1 for sparse
extern int32_t      *audiostarts;  // array of offsets in audio / audiot

//===========================================================================

//#define CA_FarRead(handle,dest,length) (read(handle,dest,length)==length)
//#define CA_FarWrite(handle,source,length) (write(handle,source,length)==length)
boolean CA_LoadFile (const char *filename, memptr *ptr);
boolean CA_WriteFile (const char *filename, void *ptr, int32_t length);

int32_t CA_RLEWCompress (word *source, int32_t length, word *dest, word rlewtag);

void CA_RLEWexpand (word *source, word *dest, int32_t length, word rlewtag);

void CA_Startup (void);
void CA_Shutdown (void);

void CA_CacheAudioChunk (int chunk);
void CA_LoadAllSounds (void);

void CA_CacheGrChunk (int chunk);
void CA_CacheMap (int mapnum);

void CA_CacheScreen (int chunk);

void CA_CannotOpen(const char *name);
