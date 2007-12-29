//
//      ID Engine
//      ID_SD.c - Sound Manager for Wolfenstein 3D
//      v1.2
//      By Jason Blochowiak
//

//
//      This module handles dealing with generating sound on the appropriate
//              hardware
//
//      Depends on: User Mgr (for parm checking)
//
//      Globals:
//              For User Mgr:
//                      SoundSourcePresent - Sound Source thingie present?
//                      SoundBlasterPresent - SoundBlaster card present?
//                      AdLibPresent - AdLib card present?
//                      SoundMode - What device is used for sound effects
//                              (Use SM_SetSoundMode() to set)
//                      MusicMode - What device is used for music
//                              (Use SM_SetMusicMode() to set)
//                      DigiMode - What device is used for digitized sound effects
//                              (Use SM_SetDigiDevice() to set)
//
//              For Cache Mgr:
//                      NeedsDigitized - load digitized sounds?
//                      NeedsMusic - load music?
//

#include "wl_def.h"
#include <SDL_mixer.h>
#include "fmopl.h"

#pragma hdrstop

#ifdef  nil
#undef  nil
#endif
#define nil     0

//#define BUFFERDMA
#define NOSEGMENTATION
//#define SHOWSDDEBUG
#define SAMPLERATE 44100
#define ORIGSAMPLERATE 7042

typedef struct
{
	char RIFF[4];
	longword filelenminus8;
	char WAVE[4];
	char fmt_[4];
	longword formatlen;
	word val0x0001;
	word channels;
	longword samplerate;
	longword bytespersec;
	word bytespersample;
	word bitspersample;
} headchunk;

typedef struct
{
	char chunkid[4];
	longword chunklength;
} wavechunk;

static byte *wavebuffer = NULL;

Mix_Chunk *SoundChunks[STARTMUSIC - STARTDIGISOUNDS];
byte *SoundBuffers[STARTMUSIC - STARTDIGISOUNDS];

globalsoundpos channelSoundPos[MIX_CHANNELS];

#define SDL_SoundFinished()     {SoundNumber = (soundnames) 0; SoundPriority = 0;}

//      Global variables
        boolean         SoundSourcePresent,
                        AdLibPresent,
                        SoundBlasterPresent,SBProPresent,
                        NeedsDigitized,NeedsMusic,
                        SoundPositioned;
        SDMode          SoundMode;
        SMMode          MusicMode;
        SDSMode         DigiMode;
//volatile longword       TimeCount;
        byte            **SoundTable;
        boolean         ssIsTandy;
        word            ssPort = 2;
        int             DigiMap[LASTSOUND];
        int             DigiChannel[STARTMUSIC - STARTDIGISOUNDS];

//      Internal variables
static  boolean         SD_Started;
        boolean         nextsoundpos;
        longword        TimerDivisor,TimerCount;
static  const char* const ParmStrings[] =
                        {
                            "noal",
                            "nosb",
                            "nopro",
                            "noss",
                            "sst",
                            "ss1",
                            "ss2",
                            "ss3",
                            nil
                        };
//static  void                    (*SoundUserHook)(void);
        soundnames              SoundNumber,DigiNumber;
        word                    SoundPriority,DigiPriority;
        int                     LeftPosition,RightPosition;
        long                    LocalTime;
        word                    TimerRate;

        word                    NumDigi,DigiLeft,DigiPage;
        word                    *DigiList;
        word                    DigiLastStart,DigiLastEnd;
        boolean                 DigiPlaying;
static  boolean                 DigiMissed,DigiLastSegment;
static  memptr                  DigiNextAddr;
static  word                    DigiNextLen;

//      SoundBlaster variables
static  boolean                 sbNoCheck,sbNoProCheck;
static  volatile boolean        sbSamplePlaying;
static  byte                    sbOldIntMask = 0xff;
static  volatile byte           *sbNextSegPtr;
static  byte                    sbDMA = 1,
                                sbDMAa1 = 0x83,sbDMAa2 = 2,sbDMAa3 = 3;
static  byte                    sba1Vals[] = {0x87,0x83,0,0x82};
static  byte                    sba2Vals[] = {0,2,0,6};
static  byte                    sba3Vals[] = {1,3,0,7};
static  int                     sbLocation = -1,sbInterrupt = 7,sbIntVec = 0xf;
static  int                     sbIntVectors[] = {-1,-1,0xa,0xb,-1,0xd,-1,0xf,-1,-1,-1};
static  volatile longword       sbNextSegLen;
//static  void (__interrupt *sbOldIntHand)(void);
static  byte                    sbpOldFMMix,sbpOldVOCMix;

//      SoundSource variables
        boolean                 ssNoCheck;
        boolean                 ssActive;
        word                    ssControl,ssStatus,ssData;
        byte                    ssOn,ssOff;
        volatile byte           *ssSample;
        volatile longword       ssLengthLeft;

//      PC Sound variables
        volatile byte   pcLastSample;
        volatile byte *pcSound;
        longword                pcLengthLeft;

//      AdLib variables
        boolean                 alNoCheck;
        byte                    *alSound;
        byte                    alBlock;
        longword                alLengthLeft;
        longword                alTimeCount;
        Instrument              alZeroInst;
        boolean                 alNoIRQ;

// This table maps channel numbers to carrier and modulator op cells
static  byte                    carriers[9] =  { 3, 4, 5,11,12,13,19,20,21},
                                modifiers[9] = { 0, 1, 2, 8, 9,10,16,17,18};

//      Sequencer variables
        boolean                 sqActive;
static  byte                    alFXReg;
        word                    *sqHack,*sqHackPtr;
        word                    sqHackLen,sqHackSeqLen;
        long                    sqHackTime;

//      Internal routines
        void                    SDL_DigitizedDoneInIRQ();

#define DMABUFFERSIZE 4096

int DMABufferDescriptor=0;
int DMABufferIndex=0;
byte *DMABuffer;

int count_time=0;
int count_fx=0;
int extreme=0;
volatile boolean pcindicate;

//volatile boolean deactivateSoundHandler=false;

boolean isSBSamplePlaying() { return sbSamplePlaying; }
byte *getSBNextSegPtr() { return (byte *) sbNextSegPtr; }


#ifdef NOTYET

int DPMI_GetDOSMemory(void **ptr, int *descriptor, unsigned length);
#pragma aux DPMI_GetDOSMemory = \
        "mov    eax,0100h" \
        "add    ebx,15" \
        "shr    ebx,4" \
        "int    31h" \
        "jc     DPMI_Exit" \
        "movzx  eax,ax" \
        "shl    eax,4" \
        "mov    [esi],eax" \
        "mov    [edi],edx" \
        "sub    eax,eax" \
        "DPMI_Exit:" \
        parm [esi] [edi] [ebx] \
        modify exact [eax ebx edx]

int DPMI_FreeDOSMemory(int descriptor);
#pragma aux DPMI_FreeDOSMemory = \
        "mov    eax,0101h" \
        "int    31h" \
        "jc     DPMI_Exit" \
        "sub    eax,eax" \
        "DPMI_Exit:" \
        parm [edx] \
        modify exact [eax]

void SDL_turnOnPCSpeaker(word timerval);
#pragma aux SDL_turnOnPCSpeaker = \
        "mov    al,0b6h" \
        "out    43h,al" \
        "mov    al,bl" \
        "out    42h,al" \
        "mov    al,bh" \
        "out    42h,al" \
        "in     al,61h" \
        "or     al,3"   \
        "out    61h,al" \
        parm [bx] \
        modify exact [al]

void SDL_turnOffPCSpeaker();
#pragma aux SDL_turnOffPCSpeaker = \
        "in     al,61h" \
        "and    al,0fch" \
        "out    61h,al" \
        modify exact [al]

void SDL_setPCSpeaker(byte val);
#pragma aux SDL_setPCSpeaker = \
        "in     al,61h" \
        "and    al,0fch" \
        "or     al,ah" \
        "out    61h,al" \
        parm [ah] \
        modify exact [al]

void inline SDL_DoFX()
{
        if(pcSound)
        {
                if(*pcSound!=pcLastSample)
                {
                        pcLastSample=*pcSound;

                        if(pcLastSample)
                                SDL_turnOnPCSpeaker(pcLastSample*60);
                        else
                                SDL_turnOffPCSpeaker();
                }
                pcSound++;
                pcLengthLeft--;
                if(!pcLengthLeft)
                {
                        pcSound=0;
                        SoundNumber=(soundnames)0;
                        SoundPriority=0;
                        SDL_turnOffPCSpeaker();
                }
        }

        if(alSound && !alNoIRQ)
        {
                if(*alSound)
                {
                        alOutInIRQ(alFreqL,*alSound);
                        alOutInIRQ(alFreqH,alBlock);
                }
                else alOutInIRQ(alFreqH,0);
                alSound++;
                alLengthLeft--;
                if(!alLengthLeft)
                {
                        alSound=0;
                        SoundNumber=(soundnames)0;
                        SoundPriority=0;
                        alOutInIRQ(alFreqH,0);
                }
        }
}

void inline SDL_DoFast()
{
        count_fx++;
        if(count_fx>=5)
        {
                count_fx=0;

                SDL_DoFX();

                count_time++;
                if(count_time>=2)
                {
                        TimeCount++;
                        count_time=0;
                }
        }

        if(sqActive && !alNoIRQ)
        {
                if(sqHackLen)
                {
                        do
                        {
                                if(sqHackTime>alTimeCount) break;
                                sqHackTime=alTimeCount+*(sqHackPtr+1);
                                alOutInIRQ(*(byte *)sqHackPtr,*(((byte *)sqHackPtr)+1));
                                sqHackPtr+=2;
                                sqHackLen-=4;
                        }
                        while(sqHackLen);
                }
                alTimeCount++;
                if(!sqHackLen)
                {
                        sqHackPtr=sqHack;
                        sqHackLen=sqHackSeqLen;
                        alTimeCount=0;
                        sqHackTime=0;
                }
        }

        if(ssSample)
        {
                if(!(inp(ssStatus)&0x40))
                {
                        outp(ssData,*ssSample++);
                        outp(ssControl,ssOff);
                        _asm push eax
                        _asm pop eax
                        outp(ssControl,ssOn);
                        _asm push eax
                        _asm pop eax
                        ssLengthLeft--;
                        if(!ssLengthLeft)
                        {
                                ssSample=0;
                                SDL_DigitizedDoneInIRQ();
                        }
                }
        }

        TimerCount+=TimerDivisor;
        if(*((word *)&TimerCount+1))
        {
                *((word *)&TimerCount+1)=0;
                t0OldService();
        }
        else
        {
                outp(0x20,0x20);
        }
}

// Timer 0 ISR for 7000Hz interrupts
void __interrupt SDL_t0ExtremeAsmService(void)
{
        if(pcindicate)
        {
                if(pcSound)
                {
                        SDL_setPCSpeaker(((*pcSound++)&0x80)>>6);
                        pcLengthLeft--;
                        if(!pcLengthLeft)
                        {
                                pcSound=0;
                                SDL_turnOffPCSpeaker();
                                SDL_DigitizedDoneInIRQ();
                        }
                }
        }
        extreme++;
        if(extreme>=10)
        {
                extreme=0;
                SDL_DoFast();
        }
        else
                outp(0x20,0x20);
}

// Timer 0 ISR for 700Hz interrupts
void __interrupt SDL_t0FastAsmService(void)
{
        SDL_DoFast();
}

// Timer 0 ISR for 140Hz interrupts
void __interrupt SDL_t0SlowAsmService(void)
{
        count_time++;
        if(count_time>=2)
        {
                TimeCount++;
                count_time=0;
        }

        SDL_DoFX();

        TimerCount+=TimerDivisor;
        if(*((word *)&TimerCount+1))
        {
                *((word *)&TimerCount+1)=0;
                t0OldService();
        }
        else
                outp(0x20,0x20);
}

void SDL_IndicatePC(boolean ind)
{
        pcindicate=ind;
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_SetTimer0() - Sets system timer 0 to the specified speed
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_SetTimer0(word speed)
{
#ifndef TPROF   // If using Borland's profiling, don't screw with the timer
//      _asm pushfd
        _asm cli

        outp(0x43,0x36);                                // Change timer 0
        outp(0x40,(byte)speed);
        outp(0x40,speed >> 8);
        // Kludge to handle special case for digitized PC sounds
        if (TimerDivisor == (1192030 / (TickBase * 100)))
                TimerDivisor = (1192030 / (TickBase * 10));
        else
                TimerDivisor = speed;

//      _asm popfd
        _asm    sti
#else
        TimerDivisor = 0x10000;
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_SetIntsPerSec() - Uses SDL_SetTimer0() to set the number of
//              interrupts generated by system timer 0 per second
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_SetIntsPerSec(word ints)
{
        TimerRate = ints;
        SDL_SetTimer0(1192030 / ints);
}

static void
SDL_SetTimerSpeed(void)
{
        word    rate;
        void (_interrupt *isr)(void);

        if ((DigiMode == sds_PC) && DigiPlaying)
        {
                rate = TickBase * 100;
                isr = SDL_t0ExtremeAsmService;
        }
        else if ((MusicMode == smm_AdLib) || ((DigiMode == sds_SoundSource) && DigiPlaying)     )
        {
                rate = TickBase * 10;
                isr = SDL_t0FastAsmService;
        }
        else
        {
                rate = TickBase * 2;
                isr = SDL_t0SlowAsmService;
        }

        if (rate != TimerRate)
        {
                _dos_setvect(8,isr);
                SDL_SetIntsPerSec(rate);
                TimerRate = rate;
        }
}

//
//      SoundBlaster code
//

///////////////////////////////////////////////////////////////////////////
//
//      SDL_SBStopSample() - Stops any active sampled sound and causes DMA
//              requests from the SoundBlaster to cease
//
///////////////////////////////////////////////////////////////////////////
static void SDL_SBStopSampleInIRQ(void)
{
        byte    is;

        if (sbSamplePlaying)
        {
                sbSamplePlaying = false;

                sbWriteDelay();
//              sbOut(sbWriteCmd,0xd0); // Turn off DSP DMA
                sbOut(sbWriteCmd,0xda); // exit autoinitialise (stop dma transfer in vdmsound)

                is = inp(0x21); // Restore interrupt mask bit
                if (sbOldIntMask & (1 << sbInterrupt))
                        is |= (1 << sbInterrupt);
                else
                        is &= ~(1 << sbInterrupt);
                outp(0x21,is);
        }
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_SBPlaySeg() - Plays a chunk of sampled sound on the SoundBlaster
//      Insures that the chunk doesn't cross a bank boundary, programs the DMA
//       controller, and tells the SB to start doing DMA requests for DAC
//
///////////////////////////////////////////////////////////////////////////

// data must not overlap a 64k boundary and must be in main memory

static longword SDL_SBPlaySegInIRQ(volatile byte *data,longword length)
{
//      int datapage;
        int dataofs;
        int uselen;

/*      uselen = length;
        if(uselen>4096) uselen=4096;

#ifdef BUFFERDMA
        datapage=((int)data>>16)&255;
        dataofs=(int)data;
#else
        memcpy(DMABuffer[0],(byte *) data,uselen);
        datapage=((int)DMABuffer[0]>>16)&255;
//      dataofs=(int)DMABuffer[0];
#endif*/

        dataofs=inp(sbDMAa2);
        dataofs|=inp(sbDMAa2)<<8;
//      dataofs=inp(sbDMAa2)|(inp(sbDMAa2)<<8);
        int bufoffs=dataofs-(word)DMABuffer;
        int lenleft=8192-bufoffs;
        if(length>lenleft) uselen=lenleft;
        else uselen=length;
//      if(uselen>2048) uselen=2048;

        memcpy(DMABuffer+bufoffs,(byte *) data,uselen);

        uselen--;

/*      // Program the DMA controller
        outp(0x0a,sbDMA | 4);                                   // Mask off DMA on channel sbDMA
        outp(0x0c,0);                                                   // Clear byte ptr flip-flop to lower byte
        outp(0x0b,0x48 | sbDMA);                                // Set transfer mode for D/A conv
        outp(sbDMAa2,(byte)dataofs);                    // Give LSB of address
        outp(sbDMAa2,(byte)(dataofs >> 8));             // Give MSB of address
        outp(sbDMAa1,(byte)datapage);                   // Give page of address
        outp(sbDMAa3,(byte)uselen);                             // Give LSB of length
        outp(sbDMAa3,(byte)(uselen >> 8));              // Give MSB of length
        outp(0x0a,sbDMA);                                               // Re-enable DMA on channel sbDMA
*/

        // Start playing the thing

/*      sbWriteDelay();
        sbOut(sbWriteCmd,0x48);
        sbWriteDelay();
        sbOut(sbWriteData,(byte)uselen);
        sbWriteDelay();
        sbOut(sbWriteData,(byte)(uselen >> 8));
        sbWriteDelay();
        sbOut(sbWriteCmd,0x1c);*/

        sbWriteDelay();
        sbOut(sbWriteCmd,0x14);
        sbWriteDelay();
        sbOut(sbWriteData,(byte)uselen);
        sbWriteDelay();
        sbOut(sbWriteData,(byte)(uselen >> 8));
        return(uselen + 1);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_SBService() - Services the SoundBlaster DMA interrupt
//
///////////////////////////////////////////////////////////////////////////
static void __interrupt
SDL_SBService(void)
{
        longword        used;

        sbIn(sbDataAvail);      // Ack interrupt to SB

        if (sbNextSegPtr)
        {
                used = SDL_SBPlaySegInIRQ(sbNextSegPtr,sbNextSegLen);
                if (sbNextSegLen <= used)
                        sbNextSegPtr = nil;
                else
                {
                        sbNextSegPtr += used;
                        sbNextSegLen -= used;
                }
        }
        else
        {
                SDL_SBStopSampleInIRQ();
                SDL_DigitizedDoneInIRQ();
        }

        outp(0x20,0x20);        // Ack interrupt
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_SBPlaySample() - Plays a sampled sound on the SoundBlaster. Sets up
//              DMA to play the sound
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_SBPlaySample(byte *data,longword len,boolean inIRQ)
{
        longword        used;

        if(!inIRQ)
        {
//              _asm    pushfd
                _asm    cli
        }

        SDL_SBStopSampleInIRQ();

        used = SDL_SBPlaySegInIRQ(data,len);    // interrupt flag already disabled
        if (len <= used)
                sbNextSegPtr = nil;
        else
        {
                sbNextSegPtr = data + used;
                sbNextSegLen = len - used;
        }

        // Save old interrupt status and unmask ours
        sbOldIntMask = inp(0x21);
        outp(0x21,sbOldIntMask & ~(1 << sbInterrupt));

/*
        sbWriteDelay();
        sbOut(sbWriteCmd,0xd4);                                         // Make sure DSP DMA is enabled
*/

        sbSamplePlaying = true;

        if(!inIRQ)
        {
//              _asm    popfd
                _asm    sti
        }

#ifdef SHOWSDDEBUG
        static int numplayed=0;
        numplayed++;
        VL_Plot(numplayed,1,14);
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PositionSBP() - Sets the attenuation levels for the left and right
//              channels by using the mixer chip on the SB Pro. This hits a hole in
//              the address map for normal SBs.
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_PositionSBP(int leftpos,int rightpos)
{
        byte    v;

        if (!SBProPresent)
                return;

        leftpos = 15 - leftpos;
        rightpos = 15 - rightpos;
        v = ((leftpos & 0x0f) << 4) | (rightpos & 0x0f);

//      _asm    pushfd
        _asm    cli

        sbOut(sbpMixerAddr,sbpmVoiceVol);
        sbOut(sbpMixerData,v);

//      _asm    popfd
        _asm    sti
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_CheckSB() - Checks to see if a SoundBlaster resides at a
//              particular I/O location
//
///////////////////////////////////////////////////////////////////////////
static boolean
SDL_CheckSB(int port)
{
        int     i;

        sbLocation = port << 4;         // Initialize stuff for later use

        sbOut(sbReset,true);            // Reset the SoundBlaster DSP
        _asm {
                mov     edx,0x388                               // Wait >4usec
                in      al, dx
                in      al, dx
                in      al, dx
                in      al, dx
                in      al, dx
                in      al, dx
                in      al, dx
                in      al, dx
                in      al, dx
        }

        sbOut(sbReset,false);           // Turn off sb DSP reset

        _asm {
                mov     edx,0x388                               // Wait >100usec
                mov     ecx,100
usecloop:
                in      al,dx
                loop usecloop
        }

        for (i = 0;i < 100;i++)
        {
                if (sbIn(sbDataAvail) & 0x80)           // If data is available...
                {
                        if (sbIn(sbReadData) == 0xaa)   // If it matches correct value
                                return(true);
                        else
                        {
                                sbLocation = -1;                        // Otherwise not a SoundBlaster
                                return(false);
                        }
                }
        }
        sbLocation = -1;                                                // Retry count exceeded - fail
        return(false);
}

///////////////////////////////////////////////////////////////////////////
//
//      Checks to see if a SoundBlaster is in the system. If the port passed is
//              -1, then it scans through all possible I/O locations. If the port
//              passed is 0, then it uses the default (2). If the port is >0, then
//              it just passes it directly to SDL_CheckSB()
//
///////////////////////////////////////////////////////////////////////////
static boolean
SDL_DetectSoundBlaster(int port)
{
        int     i;

        if (port == 0)                                  // If user specifies default, use 2
                port = 2;
        if (port == -1)
        {
                if (SDL_CheckSB(2))                     // Check default before scanning
                        return(true);

                if (SDL_CheckSB(4))                     // Check other SB Pro location before scan
                        return(true);

                for (i = 1;i <= 6;i++)          // Scan through possible SB locations
                {
                        if ((i == 2) || (i == 4))
                                continue;

                        if (SDL_CheckSB(i))             // If found at this address,
                                return(true);           //      return success
                }
                return(false);                          // All addresses failed, return failure
        }
        else
                return(SDL_CheckSB(port));      // User specified address or default
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_SBSetDMA() - Sets the DMA channel to be used by the SoundBlaster
//              code. Sets up sbDMA, and sbDMAa1-sbDMAa3 (used by SDL_SBPlaySeg()).
//
///////////////////////////////////////////////////////////////////////////
void
SDL_SBSetDMA(byte channel)
{
        if (channel > 3)
                Quit("SDL_SBSetDMA() - invalid SoundBlaster DMA channel");

        sbDMA = channel;
        sbDMAa1 = sba1Vals[channel];
        sbDMAa2 = sba2Vals[channel];
        sbDMAa3 = sba3Vals[channel];
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_StartSB() - Turns on the SoundBlaster
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_StartSB(void)
{
        byte    timevalue,test;

        sbIntVec = sbIntVectors[sbInterrupt];
        if (sbIntVec < 0)
                Quit("SDL_StartSB: Illegal or unsupported interrupt number for SoundBlaster");

        byte *buffer;
        if(DPMI_GetDOSMemory((void **) &buffer, &DMABufferDescriptor,4*DMABUFFERSIZE)) // 2*(2*DMABUFFERSIZE)
                Quit("SDL_StartSB: Unable to allocate DMA buffer");

        if(((unsigned long) buffer & 0xffff) + (2*DMABUFFERSIZE) > 0x10000)
        {
                buffer = (byte *)(((unsigned long) buffer & 0xff0000) + 0x10000);
        }
        DMABuffer=buffer;
        memset(DMABuffer,128,2*DMABUFFERSIZE);

        sbOldIntHand = _dos_getvect(sbIntVec);  // Get old interrupt handler
        _dos_setvect(sbIntVec,SDL_SBService);   // Set mine

        sbWriteDelay();
        sbOut(sbWriteCmd,0xd1);                         // Turn on DSP speaker

        // Set the SoundBlaster DAC time constant for 7KHz
        timevalue = 256 - (1000000 / 7000);
        sbWriteDelay();
        sbOut(sbWriteCmd,0x40);
        sbWriteDelay();
        sbOut(sbWriteData,timevalue);

        // Program the DMA controller
        int datapage=((int)DMABuffer>>16)&255;
        int dataofs=(int)DMABuffer;

        outp(0x0a,sbDMA | 4);                                   // Mask off DMA on channel sbDMA
        outp(0x0c,0);                                                   // Clear byte ptr flip-flop to lower byte
        outp(0x0b,0x58 | sbDMA);                                // Set transfer mode for D/A conv
        outp(sbDMAa2,(byte)dataofs);                    // Give LSB of address
        outp(sbDMAa2,(byte)(dataofs >> 8));             // Give MSB of address
        outp(sbDMAa1,(byte)datapage);                   // Give page of address
        outp(sbDMAa3,(byte)(2*DMABUFFERSIZE-1));                                // Give LSB of length
        outp(sbDMAa3,(byte)((2*DMABUFFERSIZE-1) >> 8));         // Give MSB of length
        outp(0x0a,sbDMA);                                               // Re-enable DMA on channel sbDMA

        SBProPresent = false;
        if (sbNoProCheck)
                return;

        // Check to see if this is a SB Pro
        sbOut(sbpMixerAddr,sbpmFMVol);
        sbpOldFMMix = sbIn(sbpMixerData);
        sbOut(sbpMixerData,0xbb);
        test = sbIn(sbpMixerData);
        if (test == 0xbb)
        {
                // Boost FM output levels to be equivilent with digitized output
                sbOut(sbpMixerData,0xff);
                test = sbIn(sbpMixerData);
                if (test == 0xff)
                {
                        SBProPresent = true;

                        // Save old Voice output levels (SB Pro)
                        sbOut(sbpMixerAddr,sbpmVoiceVol);
                        sbpOldVOCMix = sbIn(sbpMixerData);

                        // Turn SB Pro stereo DAC off
                        sbOut(sbpMixerAddr,sbpmControl);
                        sbOut(sbpMixerData,0);                          // 0=off,2=on
                }
        }
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutSB() - Turns off the SoundBlaster
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ShutSB(void)
{
        _asm cli
        SDL_SBStopSampleInIRQ();
        _asm sti

        if (SBProPresent)
        {
                // Restore FM output levels (SB Pro)
                sbOut(sbpMixerAddr,sbpmFMVol);
                sbOut(sbpMixerData,sbpOldFMMix);

                // Restore Voice output levels (SB Pro)
                sbOut(sbpMixerAddr,sbpmVoiceVol);
                sbOut(sbpMixerData,sbpOldVOCMix);
        }

        _dos_setvect(sbIntVec,sbOldIntHand);            // Set vector back

        DPMI_FreeDOSMemory(DMABufferDescriptor);
}

//      Sound Source Code

///////////////////////////////////////////////////////////////////////////
//
//      SDL_SSStopSample() - Stops a sample playing on the Sound Source
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_SSStopSampleInIRQ(void)
{
        ssSample = 0;
}


///////////////////////////////////////////////////////////////////////////
//
//      SDL_SSPlaySample() - Plays the specified sample on the Sound Source
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_SSPlaySample(byte *data,longword len,boolean inIRQ)
{
        if(!inIRQ)
        {
//              _asm    pushfd
                _asm    cli
        }

        ssLengthLeft = len;
        ssSample = (volatile byte *)data;

        if(!inIRQ)
        {
//              _asm    popfd
                _asm    sti
        }
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_StartSS() - Sets up for and turns on the Sound Source
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_StartSS(void)
{
        if (ssPort == 3)
                ssControl = 0x27a;      // If using LPT3
        else if (ssPort == 2)
                ssControl = 0x37a;      // If using LPT2
        else
                ssControl = 0x3be;      // If using LPT1
        ssStatus = ssControl - 1;
        ssData = ssStatus - 1;

        ssOn = 0x04;
        if (ssIsTandy)
                ssOff = 0x0e;                           // Tandy wierdness
        else
                ssOff = 0x0c;                           // For normal machines

        outp(ssControl,ssOn);           // Enable SS
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutSS() - Turns off the Sound Source
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ShutSS(void)
{
        outp(ssControl,ssOff);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_CheckSS() - Checks to see if a Sound Source is present at the
//              location specified by the sound source variables
//
///////////////////////////////////////////////////////////////////////////
static boolean
SDL_CheckSS(void)
{
        boolean         present = false;
        longword        lasttime;

        // Turn the Sound Source on and wait awhile (4 ticks)
        SDL_StartSS();

        lasttime = TimeCount;
        while (TimeCount < lasttime + 4)
                ;

        if(inp(ssStatus)&0x40) goto checkdone;          // Check to see if FIFO is currently empty

        _asm {
                mov             ecx,32                  // Force FIFO overflow (FIFO is 16 bytes)
outloop:
                mov             dx,[ssData]             // Pump a neutral value out
                mov             al,0x80
                out             dx,al

                mov             dx,[ssControl]  // Pulse printer select
                mov             al,[ssOff]
                out             dx,al
                push            eax
                pop             eax
                mov             al,[ssOn]
                out             dx,al

                push            eax                             // Delay a short while before we do this again
                pop             eax
                push            eax
                pop             eax

                loop    outloop
        }

        if(inp(ssStatus)&0x40) present=true; // Is FIFO overflowed now?

checkdone:

        SDL_ShutSS();
        return(present);
}

static boolean
SDL_DetectSoundSource(void)
{
        for (ssPort = 1;ssPort <= 3;ssPort++)
                if (SDL_CheckSS())
                        return(true);
        return(false);
}

//
//      PC Sound code
//

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCPlaySample() - Plays the specified sample on the PC speaker
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_PCPlaySample(byte *data,longword len,boolean inIRQ)
{
        if(!inIRQ)
        {
//              _asm    pushfd
                _asm    cli
        }

        SDL_IndicatePC(true);

        pcLengthLeft = len;
        pcSound = (volatile byte *)data;

        if(!inIRQ)
        {
//              _asm    popfd
                _asm    sti
        }
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCStopSample() - Stops a sample playing on the PC speaker
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_PCStopSampleInIRQ(void)
{
        pcSound = 0;

        SDL_IndicatePC(false);

        _asm    in      al,0x61                 // Turn the speaker off
        _asm    and     al,0xfd                 // ~2
        _asm    out     0x61,al
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCPlaySound() - Plays the specified sound on the PC speaker
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_PCPlaySound(PCSound *sound)
{
//      _asm    pushfd
        _asm    cli

        pcLastSample = -1;
        pcLengthLeft = sound->common.length;
        pcSound = sound->data;

//      _asm    popfd
        _asm    sti
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCStopSound() - Stops the current sound playing on the PC Speaker
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_PCStopSound(void)
{
//      _asm    pushfd
        _asm    cli

        pcSound = 0;

        _asm    in      al,0x61                 // Turn the speaker off
        _asm    and     al,0xfd                 // ~2
        _asm    out     0x61,al

//      _asm    popfd
        _asm    sti
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutPC() - Turns off the pc speaker
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ShutPC(void)
{
//      _asm    pushfd
        _asm    cli

        pcSound = 0;

        _asm    in      al,0x61                 // Turn the speaker & gate off
        _asm    and     al,0xfc                 // ~3
        _asm    out     0x61,al

//      _asm    popfd
        _asm    sti
}

void
SDL_PlayDigiSegment(memptr addr,word len,boolean inIRQ)
{
        switch (DigiMode)
        {
        case sds_PC:
        SDL_PCPlaySample((byte *) addr,len,inIRQ);
                break;
        case sds_SoundSource:
                SDL_SSPlaySample((byte *) addr,len,inIRQ);
                break;
        case sds_SoundBlaster:
                SDL_SBPlaySample((byte *) addr,len,inIRQ);
                break;
        }
}

#endif

void
SD_StopDigitized(void)
{
    DigiLeft = 0;
    DigiNextAddr = nil;
    DigiNextLen = 0;
    DigiMissed = false;
    DigiPlaying = false;
    DigiNumber = (soundnames) 0;
    DigiPriority = 0;
    SoundPositioned = false;
    if ((DigiMode == sds_PC) && (SoundMode == sdm_PC))
        SDL_SoundFinished();

    switch (DigiMode)
    {
        case sds_PC:
//            SDL_PCStopSampleInIRQ();
            break;
        case sds_SoundSource:
//            SDL_SSStopSampleInIRQ();
            break;
        case sds_SoundBlaster:
//            SDL_SBStopSampleInIRQ();
            Mix_HaltChannel(-1);
            break;
    }

    DigiLastStart = 1;
    DigiLastEnd = 0;
}

void
SD_Poll(void)
{
#ifdef NOTYET
        if (DigiLeft && !DigiNextAddr)
        {
                DigiNextLen = (DigiLeft >= PMPageSize)? PMPageSize : (DigiLeft % PMPageSize);
                DigiLeft -= DigiNextLen;
                if (!DigiLeft)
                        DigiLastSegment = true;
                DigiNextAddr = SDL_LoadDigiSegment(DigiPage++);
#ifdef BUFFERDMA
                if(DigiMode==sds_SoundBlaster)
                {
                        DMABufferIndex=(DMABufferIndex+1)&1;
                        memcpy(DMABuffer[DMABufferIndex],DigiNextAddr,DigiNextLen);
                        DigiNextAddr=DMABuffer[DMABufferIndex];
                }
#endif
        }
        if (DigiMissed && DigiNextAddr)
        {
#ifdef SHOWSDDEBUG
                static int nummissed=0;
                nummissed++;
                VL_Plot(nummissed,0,12);
#endif

                SDL_PlayDigiSegment(DigiNextAddr,DigiNextLen,false);
                DigiNextAddr = nil;
                DigiMissed = false;
                if (DigiLastSegment)
                {
                        DigiPlaying = false;
                        DigiLastSegment = false;
                }
        }
        SDL_SetTimerSpeed();
#endif
}

int SD_GetChannelForDigi(int which)
{
    if(DigiChannel[which] != -1) return DigiChannel[which];

    int channel = Mix_GroupAvailable(1);
    if(channel == -1) channel = Mix_GroupOldest(1);
    if(channel == -1)           // All sounds stopped in the meantime?
        return Mix_GroupAvailable(1);
    return channel;
}

void SD_SetPosition(int channel, int leftpos, int rightpos)
{
    if((leftpos < 0) || (leftpos > 15) || (rightpos < 0) || (rightpos > 15)
            || ((leftpos == 15) && (rightpos == 15)))
        Quit("SD_SetPosition: Illegal position");

    switch (DigiMode)
    {
        case sds_SoundBlaster:
//            SDL_PositionSBP(leftpos,rightpos);
            Mix_SetPanning(channel, ((15 - leftpos) << 4) + 15,
                ((15 - rightpos) << 4) + 15);
            break;
    }
}

Sint16 GetSample(float csample, byte *samples, int size)
{
    float s0=0, s1=0, s2=0;
    int cursample = (int) csample;
    float sf = csample - (float) cursample;

    if(cursample-1 >= 0) s0 = (float) ((char) (samples[cursample-1] - 128));
    s1 = (float) ((char) (samples[cursample] - 128));
    if(cursample+1 < size) s2 = (float) ((char) (samples[cursample+1] - 128));

    float val = s0*sf*(sf-1)/2 - s1*(sf*sf-1) + s2*(sf+1)*sf/2;
    int32_t intval = (int32_t) (val * 256);
    if(intval < -32768) intval = -32768;
    else if(intval > 32767) intval = 32767;
    return (Sint16) intval;
}

void SD_PrepareSound(int which)
{
    int page = DigiList[which * 2];
    int size = DigiList[(which * 2) + 1];

    byte *origsamples = SDL_LoadDigiSegment(page);

    int destsamples = (int) ((float) size * (float) SAMPLERATE
        / (float) ORIGSAMPLERATE);

    byte *wavebuffer = (byte *) malloc(sizeof(headchunk) + sizeof(wavechunk)
        + destsamples * 2);     // dest are 16-bit samples
    if(wavebuffer == NULL)
    {
        printf("Unable to allocate wave buffer for sound %i!\n", which);
        return;
    }
    headchunk head = {{'R','I','F','F'}, 0, {'W','A','V','E'},
        {'f','m','t',' '}, 0x10, 0x0001, 1, SAMPLERATE, SAMPLERATE*2, 2, 16};
    wavechunk dhead = {{'d', 'a', 't', 'a'}, destsamples*2};
    head.filelenminus8 = sizeof(head) + destsamples*2;  // (sizeof(dhead)-8 = 0)
    memcpy(wavebuffer, &head, sizeof(head));
    memcpy(wavebuffer+sizeof(head), &dhead, sizeof(dhead));

    Sint16 *newsamples = (Sint16 *) (wavebuffer + sizeof(headchunk)
        + sizeof(wavechunk));
    float cursample = 0.F;
    float samplestep = (float) ORIGSAMPLERATE / (float) SAMPLERATE;
    for(int i=0; i<destsamples; i++, cursample+=samplestep)
    {
        newsamples[i] = GetSample((float)size * (float)i / (float)destsamples,
            origsamples, size);
    }
    SoundBuffers[which] = wavebuffer;

    SoundChunks[which] = Mix_LoadWAV_RW(SDL_RWFromMem(wavebuffer,
        sizeof(headchunk) + sizeof(wavechunk) + destsamples * 2), 1);
}

int SD_PlayDigitized(word which,int leftpos,int rightpos)
{
    word    len;
    memptr  addr;

    if (!DigiMode)
        return 0;

    //SD_StopDigitized();
    if (which >= NumDigi)
        Quit("SD_PlayDigitized: bad sound number");

    int channel = SD_GetChannelForDigi(which);
    SD_SetPosition(channel, leftpos,rightpos);

    DigiPage = DigiList[(which * 2) + 0];
    DigiLeft = DigiList[(which * 2) + 1];

    DigiLastStart = DigiPage;
    DigiLastEnd = DigiPage + ((DigiLeft + (PMPageSize - 1)) / PMPageSize);

#ifdef NOSEGMENTATION
    len = DigiLeft;
#else
    len = (DigiLeft >= PMPageSize)? PMPageSize : (DigiLeft % PMPageSize);
#endif
    addr = SDL_LoadDigiSegment(DigiPage++);

/*#ifdef BUFFERDMA
    if(DigiMode==sds_SoundBlaster)
    {
        DMABufferIndex=(DMABufferIndex+1)&1;
        memcpy(DMABuffer[DMABufferIndex],addr,len);
        addr=DMABuffer[DMABufferIndex];
    }
#endif*/

    DigiPlaying = true;
    DigiLastSegment = false;

//    SDL_PlayDigiSegment(addr,len,false);

    Mix_Chunk *sample = SoundChunks[which];
    if(sample == NULL)
    {
        printf("SoundChunks[%i] is NULL! (which = %i)\n",
            DigiLastStart-STARTDIGISOUNDS, which);
        return 0;
    }

    if(Mix_PlayChannel(channel, sample, 0) == -1)
    {
        printf("Unable to play sound: %s\n", Mix_GetError());
        return 0;
    }

    DigiLeft -= len;
    if (!DigiLeft)
        DigiLastSegment = true;

//    SD_Poll();
    return channel;
}

void SD_ChannelFinished(int channel)
{
    channelSoundPos[channel].valid = 0;
}

#ifdef NOTYET

void
SDL_DigitizedDoneInIRQ(void)
{
        if (DigiNextAddr)
        {
                SDL_PlayDigiSegment(DigiNextAddr,DigiNextLen,true);
                DigiNextAddr = nil;
                DigiMissed = false;
        }
        else
        {
                if (DigiLastSegment)
                {
                        DigiPlaying = false;
                        DigiLastSegment = false;
                        if ((DigiMode == sds_PC) && (SoundMode == sdm_PC))
                        {
                                SDL_SoundFinished();
                        }
                        else
                        {
                                DigiNumber = (soundnames) 0;
                                DigiPriority = 0;
                        }
                        SoundPositioned = false;
                }
                else
                        DigiMissed = true;
        }
}

#endif

void
SD_SetDigiDevice(SDSMode mode)
{
    boolean devicenotpresent;

    if (mode == DigiMode)
        return;

    SD_StopDigitized();

    devicenotpresent = false;
    switch (mode)
    {
        case sds_SoundBlaster:
            if (!SoundBlasterPresent)
            {
                if (SoundSourcePresent)
                    mode = sds_SoundSource;
                else
                    devicenotpresent = true;
            }
            break;
        case sds_SoundSource:
            if (!SoundSourcePresent)
                devicenotpresent = true;
            break;
    }

    if (!devicenotpresent)
    {
#ifdef NOTYET
        if (DigiMode == sds_SoundSource)
            SDL_ShutSS();
#endif

        DigiMode = mode;

#ifdef NOTYET
        if (mode == sds_SoundSource)
            SDL_StartSS();

        SDL_SetTimerSpeed();
#endif
    }
}

void
SDL_SetupDigi(void)
{
    memptr  list;
    word    *p;
    word pg;
    int             i;

    list=malloc(PMPageSize);
    p=(word *)(Pages+((ChunksInFile-1)<<12));
    memcpy(list,p,PMPageSize);

    pg = PMSoundStart;
    for (i = 0;i < PMPageSize / (sizeof(word) * 2);i++,p += 2)
    {
        if (pg >= ChunksInFile - 1)
            break;
        pg += (p[1] + (PMPageSize - 1)) / PMPageSize;
    }

    DigiList=(word *) malloc(i*sizeof(word)*2);
    memcpy(DigiList,list,i*sizeof(word)*2);
    free(list);

    NumDigi = i;

    for (i = 0;i < LASTSOUND;i++)
    {
        DigiMap[i] = -1;
        DigiChannel[i] = -1;
    }
}

//      AdLib Code

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ALStopSound() - Turns off any sound effects playing through the
//              AdLib card
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_ALStopSound(void)
{
    alSound = 0;
    alOut(alFreqH + 0, 0);
}

static void
SDL_AlSetFXInst(Instrument *inst)
{
    byte c,m;

    m = modifiers[0];
    c = carriers[0];
    alOut(m + alChar,inst->mChar);
    alOut(m + alScale,inst->mScale);
    alOut(m + alAttack,inst->mAttack);
    alOut(m + alSus,inst->mSus);
    alOut(m + alWave,inst->mWave);
    alOut(c + alChar,inst->cChar);
    alOut(c + alScale,inst->cScale);
    alOut(c + alAttack,inst->cAttack);
    alOut(c + alSus,inst->cSus);
    alOut(c + alWave,inst->cWave);

    // Note: Switch commenting on these lines for old MUSE compatibility
//    alOutInIRQ(alFeedCon,inst->nConn);
    alOut(alFeedCon,0);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ALPlaySound() - Plays the specified sound on the AdLib card
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_ALPlaySound(AdLibSound *sound)
{
    Instrument      *inst;
    byte            *data;

    SDL_ALStopSound();

    alLengthLeft = sound->common.length;
    data = sound->data;
    alBlock = ((sound->block & 7) << 2) | 0x20;
    inst = &sound->inst;

    if (!(inst->mSus | inst->cSus))
    {
        Quit("SDL_ALPlaySound() - Bad instrument");
    }

    SDL_AlSetFXInst(inst);
    alSound = (byte *)data;
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutAL() - Shuts down the AdLib card for sound effects
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ShutAL(void)
{
    alOut(alEffects,0);
    alOut(alFreqH + 0,0);
    SDL_AlSetFXInst(&alZeroInst);
    alSound = 0;
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_CleanAL() - Totally shuts down the AdLib card
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_CleanAL(void)
{
    int     i;

    alOut(alEffects,0);
    for (i = 1; i < 0xf5; i++)
        alOut(i, 0);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_StartAL() - Starts up the AdLib card for sound effects
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_StartAL(void)
{
    alFXReg = 0;
    alOut(alEffects, alFXReg);
    SDL_AlSetFXInst(&alZeroInst);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_DetectAdLib() - Determines if there's an AdLib (or SoundBlaster
//              emulating an AdLib) present
//
///////////////////////////////////////////////////////////////////////////
static boolean
SDL_DetectAdLib(void)
{
    for (int i = 1; i <= 0xf5; i++)       // Zero all the registers
        alOut(i, 0);

    alOut(1, 0x20);             // Set WSE=1
//    alOut(8, 0);                // Set CSM=0 & SEL=0

    return true;
}

////////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutDevice() - turns off whatever device was being used for sound fx
//
////////////////////////////////////////////////////////////////////////////
static void
SDL_ShutDevice(void)
{
    switch (SoundMode)
    {
        case sdm_PC:
//            SDL_ShutPC();
            break;
        case sdm_AdLib:
            SDL_ShutAL();
            break;
    }
    SoundMode = sdm_Off;
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_CleanDevice() - totally shuts down all sound devices
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_CleanDevice(void)
{
    if ((SoundMode == sdm_AdLib) || (MusicMode == smm_AdLib))
        SDL_CleanAL();
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_StartDevice() - turns on whatever device is to be used for sound fx
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_StartDevice(void)
{
    switch (SoundMode)
    {
        case sdm_AdLib:
            SDL_StartAL();
            break;
    }
    SoundNumber = (soundnames) 0;
    SoundPriority = 0;
}

//      Public routines

///////////////////////////////////////////////////////////////////////////
//
//      SD_SetSoundMode() - Sets which sound hardware to use for sound effects
//
///////////////////////////////////////////////////////////////////////////
boolean
SD_SetSoundMode(SDMode mode)
{
    boolean result = false;
    word    tableoffset = STARTADLIBSOUNDS; // some standard value to avoid crashing...

    SD_StopSound();

#ifndef _MUSE_
    if ((mode == sdm_AdLib) && !AdLibPresent)
        mode = sdm_PC;

    switch (mode)
    {
        case sdm_Off:
            NeedsDigitized = false;
            result = true;
            break;
        case sdm_PC:
            tableoffset = STARTPCSOUNDS;
            NeedsDigitized = false;
            result = true;
            break;
        case sdm_AdLib:
            if (AdLibPresent)
            {
                tableoffset = STARTADLIBSOUNDS;
                NeedsDigitized = false;
                result = true;
            }
            break;
    }
#else
    result = true;
#endif

    if (result && (mode != SoundMode))
    {
        SDL_ShutDevice();
        SoundMode = mode;
#ifndef _MUSE_
        SoundTable = &audiosegs[tableoffset];
#endif
        SDL_StartDevice();
    }

//    SDL_SetTimerSpeed();

    return(result);
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_SetMusicMode() - sets the device to use for background music
//
///////////////////////////////////////////////////////////////////////////
boolean
SD_SetMusicMode(SMMode mode)
{
    boolean result = false;

    SD_FadeOutMusic();
    while (SD_MusicPlaying())
        ;

    switch (mode)
    {
        case smm_Off:
            NeedsMusic = false;
            result = true;
            break;
        case smm_AdLib:
            if (AdLibPresent)
            {
                NeedsMusic = true;
                result = true;
            }
            break;
    }

    if (result)
        MusicMode = mode;

//    SDL_SetTimerSpeed();

    return(result);
}

int numreadysamples = 0;
byte *curAlSound = 0;
byte *curAlSoundPtr = 0;
longword curAlLengthLeft = 0;
int soundTimeCounter = 5;

void myMusicPlayer(void *udata, Uint8 *stream, int len)
{
    int stereolen = len>>1;
    int sampleslen = stereolen>>1;
    INT16 *stream16 = (INT16 *) stream;

    while(1)
    {
        if(numreadysamples)
        {
            if(numreadysamples<sampleslen)
            {
                YM3812UpdateOne(0, stream16, numreadysamples);
                stream16 += numreadysamples*2;
                sampleslen -= numreadysamples;
            }
            else
            {
                YM3812UpdateOne(0, stream16, sampleslen);
                numreadysamples -= sampleslen;
                return;
            }
        }
        soundTimeCounter--;
        if(!soundTimeCounter)
        {
            soundTimeCounter = 5;
            if(curAlSound != alSound)
            {
                curAlSound = curAlSoundPtr = alSound;
                curAlLengthLeft = alLengthLeft;
            }
            if(curAlSound)
            {
                if(*curAlSoundPtr)
                {
                    alOut(alFreqL, *curAlSoundPtr);
                    alOut(alFreqH, alBlock);
                }
                else alOut(alFreqH, 0);
                curAlSoundPtr++;
                curAlLengthLeft--;
                if(!curAlLengthLeft)
                {
                    curAlSound = alSound = 0;
                    SoundNumber = (soundnames) 0;
                    SoundPriority = 0;
                    alOut(alFreqH, 0);
                }
            }
        }
        if(sqActive)
        {
            do
            {
                if(sqHackTime > alTimeCount) break;
                sqHackTime = alTimeCount + *(sqHackPtr+1);
                alOut(*(byte *) sqHackPtr, *(((byte *) sqHackPtr)+1));
                sqHackPtr += 2;
                sqHackLen -= 4;
            }
            while(sqHackLen>0);
            alTimeCount++;
            if(!sqHackLen)
            {
                sqHackPtr = sqHack;
                sqHackLen = sqHackSeqLen;
                sqHackTime = 0;
                alTimeCount = 0;
            }
        }
        numreadysamples = 64;
    }
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_Startup() - starts up the Sound Mgr
//              Detects all additional sound hardware and installs my ISR
//
///////////////////////////////////////////////////////////////////////////
void
SD_Startup(void)
{
    int     i;

    if (SD_Started)
        return;

    for(int i = 0; i < NUMSNDCHUNKS; i++)
        SoundChunks[i] = NULL;
    for(int i = 0; i < MIX_CHANNELS; i++)
        channelSoundPos[i].valid = 0;

    if(Mix_OpenAudio(SAMPLERATE, AUDIO_S16, 2, 2048))
    {
        printf("Unable to open audio: %s\n", Mix_GetError());
        return;
    }

    int numtimesopened, frequency, channels;
    Uint16 format;

    numtimesopened = Mix_QuerySpec(&frequency, &format, &channels);
    if(!numtimesopened) {
        printf("Mix_QuerySpec: %s\n", Mix_GetError());
    }
    else {
        const char *format_str = "Unknown";
        switch(format) {
            case AUDIO_U8: format_str = "U8"; break;
            case AUDIO_S8: format_str = "S8"; break;
            case AUDIO_U16LSB: format_str = "U16LSB"; break;
            case AUDIO_S16LSB: format_str = "S16LSB"; break;
            case AUDIO_U16MSB: format_str = "U16MSB"; break;
            case AUDIO_S16MSB: format_str = "S16MSB"; break;
        }
/*        printf("opened=%d times  frequency=%dHz  format=%s  channels=%d\n",
                numtimesopened, frequency, format_str, channels);*/
    }

    Mix_ReserveChannels(2);  // reserve player and boss weapon channels
    Mix_GroupChannels(2, MIX_CHANNELS-1, 1); // group remaining channels

    // Init music

    if(YM3812Init(1,3579545,44100))
    {
        printf("Unable to create virtual OPL!!\n");
    }

    for(i=1;i<0xf6;i++)
        YM3812Write(0,i,0);

    YM3812Write(0,1,0x20); // Set WSE=1
//    YM3812Write(0,8,0); // Set CSM=0 & SEL=0		 // already set in for statement

    Mix_HookMusic(myMusicPlayer, 0);
    Mix_ChannelFinished(SD_ChannelFinished);
    AdLibPresent = true;
    SoundBlasterPresent = true;

#ifdef NOTYET
    LocalTime = TimeCount = alTimeCount = 0;
#else
    LocalTime = alTimeCount = 0;
#endif

    SD_SetSoundMode(sdm_Off);
    SD_SetMusicMode(smm_Off);

    SDL_SetupDigi();

    SD_Started = true;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_Shutdown() - shuts down the Sound Mgr
//              Removes sound ISR and turns off whatever sound hardware was active
//
///////////////////////////////////////////////////////////////////////////
void
SD_Shutdown(void)
{
    if (!SD_Started)
        return;

    SD_MusicOff();
    SD_StopSound();

    for(int i = 0; i < STARTMUSIC - STARTDIGISOUNDS; i++)
    {
        if(SoundChunks[i]) Mix_FreeChunk(SoundChunks[i]);
        if(SoundBuffers[i]) free(SoundBuffers[i]);
    }

    free(DigiList);

    SD_Started = false;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_PositionSound() - Sets up a stereo imaging location for the next
//              sound to be played. Each channel ranges from 0 to 15.
//
///////////////////////////////////////////////////////////////////////////
void
SD_PositionSound(int leftvol,int rightvol)
{
    LeftPosition = leftvol;
    RightPosition = rightvol;
    nextsoundpos = true;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_PlaySound() - plays the specified sound on the appropriate hardware
//
///////////////////////////////////////////////////////////////////////////
boolean
SD_PlaySound(soundnames sound)
{
    boolean         ispos;
    SoundCommon     *s;
    int             lp,rp;


    lp = LeftPosition;
    rp = RightPosition;
    LeftPosition = 0;
    RightPosition = 0;

    ispos = nextsoundpos;
    nextsoundpos = false;

    if (sound == -1 || (DigiMode == sds_Off && SoundMode == sdm_Off))
        return 0;

    s = (SoundCommon *) SoundTable[sound];

    if ((SoundMode != sdm_Off) && !s)
            Quit("SD_PlaySound() - Uncached sound");

    if ((DigiMode != sds_Off) && (DigiMap[sound] != -1))
    {
        if ((DigiMode == sds_PC) && (SoundMode == sdm_PC))
        {
#ifdef NOTYET
            if (s->priority < SoundPriority)
                return 0;

            SDL_PCStopSound();

            SD_PlayDigitized(DigiMap[sound],lp,rp);
            SoundPositioned = ispos;
            SoundNumber = sound;
            SoundPriority = s->priority;
#else
            return 0;
#endif
        }
        else
        {
#ifdef NOTYET
            if (s->priority < DigiPriority)
                return(false);
#endif

            int channel = SD_PlayDigitized(DigiMap[sound], lp, rp);
            SoundPositioned = ispos;
            DigiNumber = sound;
            DigiPriority = s->priority;
            return channel + 1;
        }

        return(true);
    }

    if (SoundMode == sdm_Off)
        return 0;

    if (!s->length)
        Quit("SD_PlaySound() - Zero length sound");
    if (s->priority < SoundPriority)
        return 0;

    switch (SoundMode)
    {
        case sdm_PC:
//            SDL_PCPlaySound((PCSound *)s);
            break;
        case sdm_AdLib:
            SDL_ALPlaySound((AdLibSound *)s);
            break;
    }

    SoundNumber = sound;
    SoundPriority = s->priority;

    return 0;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_SoundPlaying() - returns the sound number that's playing, or 0 if
//              no sound is playing
//
///////////////////////////////////////////////////////////////////////////
word
SD_SoundPlaying(void)
{
    boolean result = false;

    switch (SoundMode)
    {
        case sdm_PC:
            result = pcSound? true : false;
            break;
        case sdm_AdLib:
            result = alSound? true : false;
            break;
    }

    if (result)
        return(SoundNumber);
    else
        return(false);
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_StopSound() - if a sound is playing, stops it
//
///////////////////////////////////////////////////////////////////////////
void
SD_StopSound(void)
{
    if (DigiPlaying)
        SD_StopDigitized();

    switch (SoundMode)
    {
        case sdm_PC:
//            SDL_PCStopSound();
            break;
        case sdm_AdLib:
            SDL_ALStopSound();
            break;
    }

    SoundPositioned = false;

    SDL_SoundFinished();
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_WaitSoundDone() - waits until the current sound is done playing
//
///////////////////////////////////////////////////////////////////////////
void
SD_WaitSoundDone(void)
{
    while (SD_SoundPlaying())
        ;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicOn() - turns on the sequencer
//
///////////////////////////////////////////////////////////////////////////
void
SD_MusicOn(void)
{
    sqActive = true;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicOff() - turns off the sequencer and any playing notes
//      returns the last music offset for music continue
//
///////////////////////////////////////////////////////////////////////////
int
SD_MusicOff(void)
{
    word    i;

    sqActive = false;
    switch (MusicMode)
    {
        case smm_AdLib:
            alFXReg = 0;
            alOut(alEffects, 0);
            for (i = 0;i < sqMaxTracks;i++)
                alOut(alFreqH + i + 1, 0);
            break;
    }

    return sqHackPtr-sqHack;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_StartMusic() - starts playing the music pointed to
//
///////////////////////////////////////////////////////////////////////////
void
SD_StartMusic(MusicGroup *music)
{
    SD_MusicOff();

    if (MusicMode == smm_AdLib)
    {
        sqHackPtr = sqHack = music->values;
        sqHackLen = sqHackSeqLen = music->length;
        sqHackTime = 0;
        alTimeCount = 0;
        SD_MusicOn();
    }
}

void
SD_ContinueMusic(MusicGroup *music, int startoffs)
{
    SD_MusicOff();

    if (MusicMode == smm_AdLib)
    {
        sqHackPtr = sqHack = music->values;
        sqHackLen = sqHackSeqLen = music->length;

        if(startoffs >= sqHackLen)
        {
            Quit("SD_StartMusic: Illegal startoffs provided!");
        }

        // fast forward to correct position
        // (needed to reconstruct the instruments)

        for(int i = 0; i < startoffs; i += 2)
        {
            byte reg = *(byte *)sqHackPtr;
            byte val = *(((byte *)sqHackPtr) + 1);
            if(reg >= 0xb1 && reg <= 0xb8) val &= 0xdf;           // disable play note flag
            else if(reg == 0xbd) val &= 0xe0;                     // disable drum flags

//                      sqHackTime=alTimeCount+*(sqHackPtr+1);
            alOut(reg,val);
            sqHackPtr += 2;
            sqHackLen -= 4;
        }
        sqHackTime = 0;
        alTimeCount = 0;

        SD_MusicOn();
    }
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_FadeOutMusic() - starts fading out the music. Call SD_MusicPlaying()
//              to see if the fadeout is complete
//
///////////////////////////////////////////////////////////////////////////
void
SD_FadeOutMusic(void)
{
    switch (MusicMode)
    {
        case smm_AdLib:
            // DEBUG - quick hack to turn the music off
            SD_MusicOff();
            break;
    }
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicPlaying() - returns true if music is currently playing, false if
//              not
//
///////////////////////////////////////////////////////////////////////////
boolean
SD_MusicPlaying(void)
{
    boolean result;

    switch (MusicMode)
    {
        case smm_AdLib:
            result = false;
            // DEBUG - not written
            break;
        default:
            result = false;
            break;
    }

    return(result);
}
