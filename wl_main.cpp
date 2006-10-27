// WL_MAIN.C

#include "WL_DEF.H"
#pragma hdrstop


/*
=============================================================================

                            WOLFENSTEIN 3-D

                        An Id Software production

                            by John Carmack

=============================================================================
*/

extern byte signon[];
extern byte ASCIINames[];
extern byte ShiftNames[];
/*
=============================================================================

                            LOCAL CONSTANTS

=============================================================================
*/


#define FOCALLENGTH     (0x5700l)               // in global coordinates
#define VIEWGLOBAL      0x10000                 // globals visable flush to wall

#define VIEWWIDTH       256                     // size of view window
#define VIEWHEIGHT      144

/*
=============================================================================

                            GLOBAL VARIABLES

=============================================================================
*/

char str[80], str2[20];
int tedlevelnum;
boolean tedlevel;
boolean nospr;
boolean IsA386;
int dirangle[9] = {0, ANGLES / 8, 2 * ANGLES / 8, 3 * ANGLES / 8,
    4 * ANGLES / 8, 5 * ANGLES / 8, 6 * ANGLES / 8, 7 * ANGLES / 8, ANGLES};

//
// proejection variables
//
fixed focallength;
unsigned screenofs;
int viewscreenx, viewscreeny;
int viewwidth;
int viewheight;
short centerx;
int shootdelta;           // pixels away from centerx a target can be
fixed scale, maxslope;
long heightnumerator;
int minheightdiv;


void Quit (char *error);

boolean startgame, loadedgame, virtualreality;
int mouseadjustment;

char configname[13] = "CONFIG.";


/*
=============================================================================

                            LOCAL VARIABLES

=============================================================================
*/


/*
====================
=
= ReadConfig
=
====================
*/

void ReadConfig(void)
{
    FILE *file;
    SDMode sd;
    SMMode sm;
    SDSMode sds;

    file = fopen(configname, "rb");

    if(file != NULL)
    {
        //
        // valid config file
        //
        word tmp;
        fread(&tmp, sizeof(tmp), 1, file);
        if (tmp != 0xfefc)
        {
            fclose(file);
            goto noconfig;
        }
        fread(Scores, sizeof(HighScore), MaxScores, file);

        fread(&sd, sizeof(sd), 1, file);
        fread(&sm, sizeof(sm), 1, file);
        fread(&sds, sizeof(sds), 1, file);

        fread(&mouseenabled, sizeof(mouseenabled), 1, file);
        fread(&joystickenabled, sizeof(joystickenabled), 1, file);
        fread(&joypadenabled, sizeof(joypadenabled), 1, file);
        fread(&joystickprogressive, sizeof(joystickprogressive), 1, file);
        fread(&joystickport, sizeof(joystickport), 1, file);

        fread(dirscan, sizeof(dirscan), 1, file);
        fread(buttonscan, sizeof(buttonscan), 1, file);
        fread(buttonmouse, sizeof(buttonmouse), 1, file);
        fread(buttonjoy, sizeof(buttonjoy), 1, file);

        fread(&viewsize, sizeof(viewsize), 1, file);
        fread(&mouseadjustment, sizeof(mouseadjustment), 1, file);

        fclose(file);

        if ((sd == sdm_AdLib || sm == smm_AdLib) && !AdLibPresent && !SoundBlasterPresent)
        {
            sd = sdm_PC;
            sm = smm_Off;
        }

        if ((sds == sds_SoundBlaster && !SoundBlasterPresent) ||
                (sds == sds_SoundSource && !SoundSourcePresent))
            sds = sds_Off;

        // make sure values are correct

        if (mouseenabled)
            mouseenabled = true;
        if (joystickenabled)
            joystickenabled = true;
        if (joystickprogressive)
            joystickprogressive = true;
        if (joypadenabled)
            joypadenabled = true;
        if (joystickport)
            joystickport = 1;

        if (!MousePresent)
            mouseenabled = false;
        if (!JoysPresent[joystickport])
            joystickenabled = false;

        if (mouseadjustment < 0)
            mouseadjustment = 0;
        else if (mouseadjustment > 9)
            mouseadjustment = 9;

        if (viewsize < 4)
            viewsize = 4;
        else if (viewsize > screenwidth/16 - 1)
            viewsize = screenwidth/16 - 1;

        MainMenu[6].active = 1;
        MainItems.curpos = 0;
    }
    else
    {
        //
        // no config file, so select by hardware
        //
noconfig:
        if (SoundBlasterPresent || AdLibPresent)
        {
            sd = sdm_AdLib;
            sm = smm_AdLib;
        }
        else
        {
            sd = sdm_PC;
            sm = smm_Off;
        }

        if (SoundBlasterPresent)
            sds = sds_SoundBlaster;
        else if (SoundSourcePresent)
            sds = sds_SoundSource;
        else
            sds = sds_Off;

        if (MousePresent)
            mouseenabled = true;

        joystickenabled = false;
        joypadenabled = false;
        joystickport = 0;
        joystickprogressive = false;

        viewsize = screenwidth/16 - 1;                          // start with a good size
        mouseadjustment = 5;
    }

    SD_SetMusicMode (sm);
    SD_SetSoundMode (sd);
    SD_SetDigiDevice (sds);
}


/*
====================
=
= WriteConfig
=
====================
*/

void WriteConfig(void)
{
    FILE *file;

    file = fopen(configname, "wb");
    if (file != NULL)
    {
        word tmp = 0xfefc;
        fwrite(&tmp, sizeof(tmp), 1, file);
        fwrite(Scores, sizeof(HighScore), MaxScores, file);

        fwrite(&SoundMode, sizeof(SoundMode), 1, file);
        fwrite(&MusicMode, sizeof(MusicMode), 1, file);
        fwrite(&DigiMode, sizeof(DigiMode), 1, file);

        fwrite(&mouseenabled, sizeof(mouseenabled), 1, file);
        fwrite(&joystickenabled, sizeof(joystickenabled), 1, file);
        fwrite(&joypadenabled, sizeof(joypadenabled), 1, file);
        fwrite(&joystickprogressive, sizeof(joystickprogressive), 1, file);
        fwrite(&joystickport, sizeof(joystickport), 1, file);

        fwrite(dirscan, sizeof(dirscan), 1, file);
        fwrite(buttonscan, sizeof(buttonscan), 1, file);
        fwrite(buttonmouse, sizeof(buttonmouse), 1, file);
        fwrite(buttonjoy, sizeof(buttonjoy), 1, file);

        fwrite(&viewsize, sizeof(viewsize), 1, file);
        fwrite(&mouseadjustment, sizeof(mouseadjustment), 1, file);

        fclose(file);
    }
}


//===========================================================================

/*
=====================
=
= NewGame
=
= Set up new game to start from the beginning
=
=====================
*/

void NewGame (int difficulty, int episode)
{
    memset (&gamestate, 0, sizeof(gamestate));
    gamestate.difficulty = difficulty;
    gamestate.weapon = gamestate.bestweapon
                       = gamestate.chosenweapon = wp_pistol;
    gamestate.health = 100;
    gamestate.ammo = STARTAMMO;
    gamestate.lives = 3;
    gamestate.nextextra = EXTRAPOINTS;
    gamestate.episode = episode;

    startgame = true;
}

//===========================================================================

void DiskFlopAnim(int x, int y)
{
    static char which = 0;
    if (!x && !y)
        return ;
    VWBL_DrawPic(x, y, C_DISKLOADING1PIC + which);
    VW_UpdateScreen();
    which ^= 1;
}


long DoChecksum(byte *source, unsigned size, long checksum)
{
    unsigned i;

    for (i = 0; i < size - 1; i++)
        checksum += source[i] ^ source[i + 1];

    return checksum;
}


/*
==================
=
= SaveTheGame
=
==================
*/

extern statetype s_grdstand;
extern statetype s_player;

boolean SaveTheGame(FILE *file, int x, int y)
{
    //struct diskfree_t dfree;
    //long avail, size;
    long checksum;
    objtype *ob;
    objtype nullobj;
    statobj_t nullstat;

/*    if (_dos_getdiskfree(0, &dfree))
        Quit("Error in _dos_getdiskfree call");

    avail = (long)dfree.avail_clusters *
            dfree.bytes_per_sector *
            dfree.sectors_per_cluster;

    size = 0;
    for (ob = player; ob ; ob = ob->next)
        size += sizeof(*ob);
    size += sizeof(nullobj);

    size += sizeof(gamestate) +
            sizeof(LRstruct) * LRpack +
            sizeof(tilemap) +
            sizeof(actorat) +
            sizeof(laststatobj) +
            sizeof(statobjlist) +
            sizeof(doorposition) +
            sizeof(pwallstate) +
            sizeof(pwalltile) +
            sizeof(pwallx) +
            sizeof(pwally) +
            sizeof(pwalldir) +
            sizeof(pwallpos);

    if (avail < size)
    {
        Message(STR_NOSPACE1"\n"STR_NOSPACE2);
        return false;
    }*/

    checksum = 0;

    DiskFlopAnim(x, y);
    fwrite(&gamestate, sizeof(gamestate), 1, file);
    checksum = DoChecksum((byte *) & gamestate, sizeof(gamestate), checksum);

    DiskFlopAnim(x, y);
    fwrite (&LevelRatios[0], sizeof(LRstruct), LRpack, file);
    checksum = DoChecksum((byte *) & LevelRatios[0], sizeof(LRstruct) * LRpack, checksum);

    DiskFlopAnim(x, y);
    fwrite (tilemap, sizeof(tilemap), 1, file);
    checksum = DoChecksum((byte *)tilemap, sizeof(tilemap), checksum);
    DiskFlopAnim(x, y);

    for (int i = 0; i < MAPSIZE; i++)
    {
        for (int j = 0; j < MAPSIZE; j++)
        {
            word actnum;
            objtype *objptr = actorat[i][j];
            if ((long)objptr&0xffff0000)
                actnum = 0x8000 | (word)(objptr - objlist);
            else
                actnum = (word)(Uint32)objptr;
            fwrite(&actnum, sizeof(actnum), 1, file);
            checksum = DoChecksum((byte *) & actnum, sizeof(actnum), checksum);
        }
    }

    fwrite (areaconnect, sizeof(areaconnect), 1, file);
    fwrite (areabyplayer, sizeof(areabyplayer), 1, file);

    // player object needs special treatment as it's in WL_AGENT.CPP and not in
    // WL_ACT2.CPP which could cause problems for the relative addressing

    ob = player;
    DiskFlopAnim(x, y);
    memcpy(&nullobj, ob, sizeof(nullobj));
    nullobj.state = (statetype *) ((long)nullobj.state - (long) & s_player);
    fwrite (&nullobj, sizeof(nullobj), 1, file);
    ob = ob->next;

    DiskFlopAnim(x, y);
    for (; ob; ob = ob->next)
    {
        memcpy(&nullobj, ob, sizeof(nullobj));
        nullobj.state = (statetype *) ((long)nullobj.state - (long) & s_grdstand);
        fwrite (&nullobj, sizeof(nullobj), 1, file);
    }
    nullobj.active = ac_badobject;          // end of file marker
    DiskFlopAnim(x, y);
    fwrite (&nullobj, sizeof(nullobj), 1, file);

    DiskFlopAnim(x, y);
    word laststatobjnum = (word) (laststatobj - statobjlist);
    fwrite (&laststatobjnum, sizeof(laststatobjnum), 1, file);
    checksum = DoChecksum((byte *) & laststatobjnum, sizeof(laststatobjnum), checksum);

    DiskFlopAnim(x, y);
    for (int i = 0; i < MAXSTATS; i++)
    {
        memcpy(&nullstat, statobjlist + i, sizeof(nullstat));
        nullstat.visspot = (byte *) ((long) nullstat.visspot - (long)spotvis);
        fwrite (&nullstat, sizeof(nullstat), 1, file);
        checksum = DoChecksum((byte *) & nullstat, sizeof(nullstat), checksum);
    }

    DiskFlopAnim(x, y);
    fwrite (doorposition, sizeof(doorposition), 1, file);
    checksum = DoChecksum((byte *)doorposition, sizeof(doorposition), checksum);
    DiskFlopAnim(x, y);
    fwrite (doorobjlist, sizeof(doorobjlist), 1, file);
    checksum = DoChecksum((byte *)doorobjlist, sizeof(doorobjlist), checksum);

    DiskFlopAnim(x, y);
    fwrite (&pwallstate, sizeof(pwallstate), 1, file);
    checksum = DoChecksum((byte *) & pwallstate, sizeof(pwallstate), checksum);
    fwrite (&pwalltile, sizeof(pwalltile), 1, file);
    checksum = DoChecksum((byte *) & pwalltile, sizeof(pwalltile), checksum);
    fwrite (&pwallx, sizeof(pwallx), 1, file);
    checksum = DoChecksum((byte *) & pwallx, sizeof(pwallx), checksum);
    fwrite (&pwally, sizeof(pwally), 1, file);
    checksum = DoChecksum((byte *) & pwally, sizeof(pwally), checksum);
    fwrite (&pwalldir, sizeof(pwalldir), 1, file);
    checksum = DoChecksum((byte *) & pwalldir, sizeof(pwalldir), checksum);
    fwrite (&pwallpos, sizeof(pwallpos), 1, file);
    checksum = DoChecksum((byte *) & pwallpos, sizeof(pwallpos), checksum);

    //
    // WRITE OUT CHECKSUM
    //
    fwrite (&checksum, sizeof(checksum), 1, file);

    fwrite (&lastgamemusicoffset, sizeof(lastgamemusicoffset), 1, file);

    return (true);
}

//===========================================================================

/*
==================
=
= LoadTheGame
=
==================
*/

boolean LoadTheGame(FILE *file, int x, int y)
{
    long checksum, oldchecksum;
    objtype nullobj;
    statobj_t nullstat;

    checksum = 0;

    DiskFlopAnim(x, y);
    fread (&gamestate, sizeof(gamestate), 1, file);
    checksum = DoChecksum((byte *) & gamestate, sizeof(gamestate), checksum);

    DiskFlopAnim(x, y);
    fread (&LevelRatios[0], sizeof(LRstruct), LRpack, file);
    checksum = DoChecksum((byte *) & LevelRatios[0], sizeof(LRstruct) * LRpack, checksum);

    DiskFlopAnim(x, y);
    SetupGameLevel ();

    DiskFlopAnim(x, y);
    fread ( tilemap, sizeof(tilemap), 1, file);
    checksum = DoChecksum((byte *)tilemap, sizeof(tilemap), checksum);

    DiskFlopAnim(x, y);

    int actnum = 0;
    for (int i = 0; i < MAPSIZE; i++)
    {
        for (int j = 0; j < MAPSIZE; j++)
        {
            fread (&actnum, sizeof(word), 1, file);
            checksum = DoChecksum((byte *) & actnum, sizeof(word), checksum);
            if (actnum&0x8000)
                actorat[i][j] = objlist + (actnum & 0x7fff);
            else
                actorat[i][j] = (objtype *) actnum;
        }
    }

    fread (areaconnect, sizeof(areaconnect), 1, file);
    fread (areabyplayer, sizeof(areabyplayer), 1, file);

    InitActorList ();
    DiskFlopAnim(x, y);
    fread (player, sizeof(*player), 1, file);
    player->state = (statetype *) ((long)player->state + (long) & s_player);

    while (1)
    {
        DiskFlopAnim(x, y);
        fread (&nullobj, sizeof(nullobj), 1, file);
        if (nullobj.active == ac_badobject)
            break;
        GetNewActor ();
        nullobj.state = (statetype *) ((long)nullobj.state + (long) & s_grdstand);
        // don't copy over the links
        memcpy (newobj, &nullobj, sizeof(nullobj) - 8);
    }

    DiskFlopAnim(x, y);
    word laststatobjnum;
    fread (&laststatobjnum, sizeof(laststatobjnum), 1, file);
    laststatobj = statobjlist + laststatobjnum;
    checksum = DoChecksum((byte *) & laststatobjnum, sizeof(laststatobjnum), checksum);

    DiskFlopAnim(x, y);
    for (int i = 0; i < MAXSTATS; i++)
    {
        fread(&nullstat, sizeof(nullstat), 1, file);
        checksum = DoChecksum((byte *) & nullstat, sizeof(nullstat), checksum);
        nullstat.visspot = (byte *) ((long)nullstat.visspot + (long)spotvis);
        memcpy(statobjlist + i, &nullstat, sizeof(nullstat));
    }

    DiskFlopAnim(x, y);
    fread (doorposition, sizeof(doorposition), 1, file);
    checksum = DoChecksum((byte *)doorposition, sizeof(doorposition), checksum);
    DiskFlopAnim(x, y);
    fread (doorobjlist, sizeof(doorobjlist), 1, file);
    checksum = DoChecksum((byte *)doorobjlist, sizeof(doorobjlist), checksum);

    DiskFlopAnim(x, y);
    fread (&pwallstate, sizeof(pwallstate), 1, file);
    checksum = DoChecksum((byte *) & pwallstate, sizeof(pwallstate), checksum);
    fread (&pwalltile, sizeof(pwalltile), 1, file);
    checksum = DoChecksum((byte *) & pwalltile, sizeof(pwalltile), checksum);
    fread (&pwallx, sizeof(pwallx), 1, file);
    checksum = DoChecksum((byte *) & pwallx, sizeof(pwallx), checksum);
    fread (&pwally, sizeof(pwally), 1, file);
    checksum = DoChecksum((byte *) & pwally, sizeof(pwally), checksum);
    fread (&pwalldir, sizeof(pwalldir), 1, file);
    checksum = DoChecksum((byte *) & pwalldir, sizeof(pwalldir), checksum);
    fread (&pwallpos, sizeof(pwallpos), 1, file);
    checksum = DoChecksum((byte *) & pwallpos, sizeof(pwallpos), checksum);

    if (gamestate.secretcount)      // assign valid floorcodes under moved pushwalls
    {
        word *map, *obj;
        word tile, sprite;
        map = mapsegs[0];
        obj = mapsegs[1];
        for (y = 0; y < mapheight; y++)
            for (x = 0; x < mapwidth; x++)
            {
                tile = *map++;
                sprite = *obj++;
                if (sprite == PUSHABLETILE && !tilemap[x][y]
                        && (tile < AREATILE || tile >= (AREATILE + NUMMAPS)))
                {
                    if (*map >= AREATILE)
                        tile = *map;
                    if (*(map - 1 - mapwidth) >= AREATILE)
                        tile = *(map - 1 - mapwidth);
                    if (*(map - 1 + mapwidth) >= AREATILE)
                        tile = *(map - 1 + mapwidth);
                    if ( *(map - 2) >= AREATILE)
                        tile = *(map - 2);

                    *(map - 1) = tile;
                    *(obj - 1) = 0;
                }
            }
    }

    Thrust(0, 0);    // set player->areanumber to the floortile you're standing on

    fread (&oldchecksum, sizeof(oldchecksum), 1, file);

    fread (&lastgamemusicoffset, sizeof(lastgamemusicoffset), 1, file);
    if (lastgamemusicoffset < 0)
        lastgamemusicoffset = 0;


    if (oldchecksum != checksum)
    {
        Message(STR_SAVECHT1"\n"
                STR_SAVECHT2"\n"
                STR_SAVECHT3"\n"
                STR_SAVECHT4);

        IN_ClearKeysDown();
        IN_Ack();

        gamestate.oldscore = gamestate.score = 0;
        gamestate.lives = 1;
        gamestate.weapon =
            gamestate.chosenweapon =
                gamestate.bestweapon = wp_pistol;
        gamestate.ammo = 8;
    }

    return true;
}

//===========================================================================

/*
==========================
=
= ShutdownId
=
= Shuts down all ID_?? managers
=
==========================
*/

void ShutdownId (void)
{
    US_Shutdown ();         // This line is completely useless...
    SD_Shutdown ();
    PM_Shutdown ();
    IN_Shutdown ();
    VW_Shutdown ();
    CA_Shutdown ();
}


//===========================================================================

/*
==================
=
= BuildTables
=
= Calculates:
=
= scale                 projection constant
= sintable/costable     overlapping fractional tables
=
==================
*/

const float radtoint = (float)FINEANGLES / 2 / PI;

void BuildTables (void)
{
    //
    // calculate fine tangents
    //

    for (int i = 0; i < FINEANGLES / 8; i++)
    {
        double tang = tan((i + 0.5) / radtoint);
        finetangent[i] = (long)(tang * GLOBAL1);
        finetangent[FINEANGLES / 4 - 1 - i] = (long)((1 / tang) * GLOBAL1);
    }

    //
    // costable overlays sintable with a quarter phase shift
    // ANGLES is assumed to be divisable by four
    //

    float angle = 0;
    float anglestep = (float)(PI / 2 / ANGLEQUAD);
    for (int i = 0; i <= ANGLEQUAD; i++)
    {
        fixed value = (long) (GLOBAL1 * sin(angle));
        sintable[i] = sintable[i + ANGLES] = sintable[ANGLES / 2 - i] = value;
        sintable[ANGLES - i] = sintable[ANGLES / 2 + i] = -value;
        angle += anglestep;
    }

    // Initialize pseudo random generator for FizzleFade
    fizzleM = screenwidth * screenheight;

    // screen resolutions are mostly only dividable by the primes 2, 3 and 5

    int sqrtM = (int) sqrt(fizzleM);
    int *primes = (int *) malloc((int)(((double)sqrtM / log(sqrtM))*1.2F) * sizeof(int));
    if(primes == NULL)
    {
        printf("Unable to calculate fizzle fade constants! Will use trivial values...\n");
        fizzleC = 4211;
        fizzleA = 1;
        return;
    }
    int numprimes = 1;
    primes[0] = 2;
    for(int i=3; i<=sqrtM; i+=2)
    {
        int j;
        for(j=0; j<numprimes; j++)
            if(!(i % primes[j])) break;
        if(j==numprimes)
        {
            // found new prime
            primes[numprimes++] = i;
        }
    }
    int adec = 327;
    int illegalprimes = 0;
    for(int i=0; i<numprimes; i++)
    {
        if(!(fizzleM % primes[i]))
        {
            adec *= primes[i];
            primes[i] = 0;
            illegalprimes++;
        }
    }
    if(!(fizzleM & 3)) adec <<= 1;      // dividable by 4 -> adec multiple of 4
    fizzleA = adec+1;

    int c = 1;
    for(int i=numprimes-1; i>=0; i--)
    {
        if(primes[i])
        {
            c *= primes[i];
            if(c&0xfffffff0) break;     // already big enough?
        }
    }
    fizzleC = c;

    printf("fizzleA = %i fizzleC = %i fizzleM = %i\n", fizzleA, fizzleC, fizzleM);
    free(primes);
}

//===========================================================================


/*
====================
=
= CalcProjection
=
= Uses focallength
=
====================
*/

void CalcProjection (long focal)
{
    int i;
    int intang;
    float angle;
    double tang;
    int halfview;
    double facedist;

    focallength = focal;
    facedist = focal + MINDIST;
    halfview = viewwidth / 2;                                 // half view in pixels

    //
    // calculate scale value for vertical height calculations
    // and sprite x calculations
    //
    scale = (fixed) (halfview * facedist / (VIEWGLOBAL / 2));

    //
    // divide heightnumerator by a posts distance to get the posts height for
    // the heightbuffer.  The pixel height is height>>2
    //
    heightnumerator = (TILEGLOBAL * scale) >> 6;
    minheightdiv = heightnumerator / 0x7fff + 1;

    //
    // calculate the angle offset from view angle of each pixel's ray
    //

    for (i = 0; i < halfview; i++)
    {
        // start 1/2 pixel over, so viewangle bisects two middle pixels
        tang = (long)i * VIEWGLOBAL / viewwidth / facedist;
        angle = atan(tang);
        intang = (int) (angle * radtoint);
        pixelangle[halfview - 1 - i] = intang;
        pixelangle[halfview + i] = -intang;
    }

    //
    // if a point's abs(y/x) is greater than maxslope, the point is outside
    // the view area
    //
    maxslope = finetangent[pixelangle[0]];
    maxslope >>= 8;
}



//===========================================================================

/*
===================
=
= SetupWalls
=
= Map tile values to scaled pics
=
===================
*/

void SetupWalls (void)
{
    int i;

    horizwall[0] = 0;
    vertwall[0] = 0;

    for (i = 1; i < MAXWALLTILES; i++)
    {
        horizwall[i] = (i - 1) * 2;
        vertwall[i] = (i - 1) * 2 + 1;
    }
}

//===========================================================================

/*
==========================
=
= SignonScreen
=
==========================
*/

void SignonScreen (void)                        // VGA version
{
    //      unsigned        segstart,seglength;

    VL_SetVGAPlaneMode ();
//    VL_SetPalette (gamepal);

    if (!virtualreality)
    {
/*        VW_SetScreen(0x8000, 0);
        VL_MungePic (signon, 320, 200);
        VL_MemToScreen (signon, 320, 200, 0, 0);
        VW_SetScreen(0, 0);*/
        byte *vidbuf = VL_LockSurface(backgroundSurface);
        for(int y=0; y<200; y++)
        {
            for(int x=0; x<320; x++)
            {
                vidbuf[y*backgroundPitch+x] = signon[y*320+x];
            }
        }
        VL_UnlockSurface(backgroundSurface);
        VH_UpdateScreen();
    }

    // TODO: The signon memory does NOT become recycled anymore!!!

#ifdef ABCAUS
    //
    // reclaim the memory from the linked in signon screen
    //
    segstart = FP_SEG(&introscn);
    seglength = 64000 / 16;
    if (FP_OFF(&introscn))
    {
        segstart++;
        seglength--;
    }
    MML_UseSpace (segstart, seglength);
#endif
}


/*
==========================
=
= FinishSignon
=
==========================
*/

void FinishSignon (void)
{
#ifndef SPEAR
    byte *vidbuf = VL_LockSurface(backgroundSurface);
    byte color = *vidbuf;

    VL_Bar (vidbuf, backgroundPitch, 0, 189, 300, 11, color);
    VL_UnlockSurface(backgroundSurface);
    WindowX = 0;
    WindowW = screenwidth;
    PrintY = screenheight-10;

#ifndef JAPAN

    SETFONTCOLOR(14, 4);

#ifdef SPANISH

    US_CPrint ("Oprima una tecla");
#else

    US_CPrint ("Press a key");
#endif

#endif
    VH_UpdateScreen();

    if (!NoWait)
        IN_Ack ();

#ifndef JAPAN

    VWBL_Bar (0, 189, 300, 11, color);

    PrintY = screenheight-10;
    SETFONTCOLOR(10, 4);

#ifdef SPANISH

    US_CPrint ("pensando...");
#else

    US_CPrint ("Working...");
#endif

#endif
    VH_UpdateScreen();

    SETFONTCOLOR(0, 15);
#else

    if (!NoWait)
        VW_WaitVBL(3*70);
#endif
}

//===========================================================================

/*
=================
=
= MS_CheckParm
=
=================
*/

boolean MS_CheckParm (char *check)
{
    int i;
    char *parm;

    for (i = 1; i < __argc; i++)
    {
        parm = __argv[i];

        while ( !isalpha(*parm) )       // skip - / \ etc.. in front of parm
            if (!*parm++)
                break;                          // hit end of string without an alphanum

        if ( !stricmp(check, parm) )
            return true;
    }

    return false;
}

//===========================================================================

/*
=====================
=
= InitDigiMap
=
=====================
*/

// channel mapping:
//  -1: any non reserved channel
//   0: player weapons
//   1: boss weapons

static int wolfdigimap[] =
    {
        // These first sounds are in the upload version
#ifndef SPEAR
        HALTSND, 0, -1,
        DOGBARKSND, 1, -1,
        CLOSEDOORSND, 2, -1,
        OPENDOORSND, 3, -1,
        ATKMACHINEGUNSND, 4, 0,
        ATKPISTOLSND, 5, 0,
        ATKGATLINGSND, 6, 0,
        SCHUTZADSND, 7, -1,
        GUTENTAGSND, 8, -1,
        MUTTISND, 9, -1,
        BOSSFIRESND, 10, 1,
        SSFIRESND, 11, -1,
        DEATHSCREAM1SND, 12, -1,
        DEATHSCREAM2SND, 13, -1,
        DEATHSCREAM3SND, 13, -1,
        TAKEDAMAGESND, 14, -1,
        PUSHWALLSND, 15, -1,

        LEBENSND, 20, -1,
        NAZIFIRESND, 21, -1,
        SLURPIESND, 22, -1,

        YEAHSND, 32, -1,

#ifndef UPLOAD
        // These are in all other episodes
        DOGDEATHSND, 16, -1,
        AHHHGSND, 17, -1,
        DIESND, 18, -1,
        EVASND, 19, -1,

        TOT_HUNDSND, 23, -1,
        MEINGOTTSND, 24, -1,
        SCHABBSHASND, 25, -1,
        HITLERHASND, 26, -1,
        SPIONSND, 27, -1,
        NEINSOVASSND, 28, -1,
        DOGATTACKSND, 29, -1,
        LEVELDONESND, 30, -1,
        MECHSTEPSND, 31, -1,        // perhaps dedicated channel needed?

        SCHEISTSND, 33, -1,
        DEATHSCREAM4SND, 34, -1,              // AIIEEE
        DEATHSCREAM5SND, 35, -1,             // DEE-DEE
        DONNERSND, 36, -1,             // EPISODE 4 BOSS DIE
        EINESND, 37, -1,             // EPISODE 4 BOSS SIGHTING
        ERLAUBENSND, 38, -1,             // EPISODE 6 BOSS SIGHTING
        DEATHSCREAM6SND, 39, -1,              // FART
        DEATHSCREAM7SND, 40, -1,             // GASP
        DEATHSCREAM8SND, 41, -1,             // GUH-BOY!
        DEATHSCREAM9SND, 42, -1,             // AH GEEZ!
        KEINSND, 43, -1,             // EPISODE 5 BOSS SIGHTING
        MEINSND, 44, -1,             // EPISODE 6 BOSS DIE
        ROSESND, 45, -1,             // EPISODE 5 BOSS DIE

#endif
#else
        //
        // SPEAR OF DESTINY DIGISOUNDS
        //
        HALTSND, 0, -1,
        CLOSEDOORSND, 2, -1,
        OPENDOORSND, 3, -1,
        ATKMACHINEGUNSND, 4, 0,
        ATKPISTOLSND, 5, 0,
        ATKGATLINGSND, 6, 0,
        SCHUTZADSND, 7, -1,
        BOSSFIRESND, 8, 1,
        SSFIRESND, 9, -1,
        DEATHSCREAM1SND, 10, -1,
        DEATHSCREAM2SND, 11, -1,
        TAKEDAMAGESND, 12, -1,
        PUSHWALLSND, 13, -1,
        AHHHGSND, 15, -1,
        LEBENSND, 16, -1,
        NAZIFIRESND, 17, -1,
        SLURPIESND, 18, -1,
        LEVELDONESND, 22, -1,
        DEATHSCREAM4SND, 23, -1,             // AIIEEE
        DEATHSCREAM3SND, 23, -1,             // DOUBLY-MAPPED!!!
        DEATHSCREAM5SND, 24, -1,             // DEE-DEE
        DEATHSCREAM6SND, 25, -1,             // FART
        DEATHSCREAM7SND, 26, -1,             // GASP
        DEATHSCREAM8SND, 27, -1,             // GUH-BOY!
        DEATHSCREAM9SND, 28, -1,             // AH GEEZ!
        GETGATLINGSND, 38, -1,             // Got Gat replacement

#ifndef SPEARDEMO
        DOGBARKSND, 1, -1,
        DOGDEATHSND, 14, -1,
        SPIONSND, 19, -1,
        NEINSOVASSND, 20, -1,
        DOGATTACKSND, 21, -1,
        TRANSSIGHTSND, 29, -1,             // Trans Sight
        TRANSDEATHSND, 30, -1,             // Trans Death
        WILHELMSIGHTSND, 31, -1,             // Wilhelm Sight
        WILHELMDEATHSND, 32, -1,             // Wilhelm Death
        UBERDEATHSND, 33, -1,             // Uber Death
        KNIGHTSIGHTSND, 34, -1,             // Death Knight Sight
        KNIGHTDEATHSND, 35, -1,             // Death Knight Death
        ANGELSIGHTSND, 36, -1,             // Angel Sight
        ANGELDEATHSND, 37, -1,             // Angel Death
        GETSPEARSND, 39, -1,             // Got Spear replacement
#endif
#endif
        LASTSOUND
    };


void InitDigiMap (void)
{
    int *map;

    for (map = wolfdigimap; *map != LASTSOUND; map += 3)
    {
        DigiMap[map[0]] = map[1];
        DigiChannel[map[1]] = map[2];
        SD_PrepareSound(map[1]);
    }
}

#ifndef SPEAR
CP_iteminfo MusicItems = {CTL_X, CTL_Y, 6, 0, 32};
CP_itemtype MusicMenu[] =
    {
        {1, "Get Them!", 0},
        {1, "Searching", 0},
        {1, "P.O.W.", 0},
        {1, "Suspense", 0},
        {1, "War March", 0},
        {1, "Around The Corner!", 0},

        {1, "Nazi Anthem", 0},
        {1, "Lurking...", 0},
        {1, "Going After Hitler", 0},
        {1, "Pounding Headache", 0},
        {1, "Into the Dungeons", 0},
        {1, "Ultimate Conquest", 0},

        {1, "Kill the S.O.B.", 0},
        {1, "The Nazi Rap", 0},
        {1, "Twelfth Hour", 0},
        {1, "Zero Hour", 0},
        {1, "Ultimate Conquest", 0},
        {1, "Wolfpack", 0}
    };
#else
CP_iteminfo MusicItems = {CTL_X, CTL_Y - 20, 9, 0, 32};
CP_itemtype MusicMenu[] =
    {
        {1, "Funky Colonel Bill", 0},
        {1, "Death To The Nazis", 0},
        {1, "Tiptoeing Around", 0},
        {1, "Is This THE END?", 0},
        {1, "Evil Incarnate", 0},
        {1, "Jazzin' Them Nazis", 0},
        {1, "Puttin' It To The Enemy", 0},
        {1, "The SS Gonna Get You", 0},
        {1, "Towering Above", 0}
    };
#endif

#ifndef SPEARDEMO
void DoJukebox(void)
{
    int which, lastsong = -1;
    unsigned start;
    unsigned songs[] =
        {
#ifndef SPEAR
            GETTHEM_MUS,
            SEARCHN_MUS,
            POW_MUS,
            SUSPENSE_MUS,
            WARMARCH_MUS,
            CORNER_MUS,

            NAZI_OMI_MUS,
            PREGNANT_MUS,
            GOINGAFT_MUS,
            HEADACHE_MUS,
            DUNGEON_MUS,
            ULTIMATE_MUS,

            INTROCW3_MUS,
            NAZI_RAP_MUS,
            TWELFTH_MUS,
            ZEROHOUR_MUS,
            ULTIMATE_MUS,
            PACMAN_MUS
#else
            XFUNKIE_MUS,              // 0
            XDEATH_MUS,               // 2
            XTIPTOE_MUS,              // 4
            XTHEEND_MUS,              // 7
            XEVIL_MUS,                // 17
            XJAZNAZI_MUS,             // 18
            XPUTIT_MUS,               // 21
            XGETYOU_MUS,              // 22
            XTOWER2_MUS              // 23
#endif
        };

    IN_ClearKeysDown();
    if (!AdLibPresent && !SoundBlasterPresent)
        return ;

    MenuFadeOut();

#ifndef SPEAR
#ifndef UPLOAD

    start = (SDL_GetTicks() % 3) * 6;
#else

    start = 0;
#endif
#else

    start = 0;
#endif

    CA_CacheGrChunk (STARTFONT + 1);
#ifdef SPEAR

    CacheLump (BACKDROP_LUMP_START, BACKDROP_LUMP_END);
#else

    CacheLump (CONTROLS_LUMP_START, CONTROLS_LUMP_END);
#endif

    CA_LoadAllSounds ();

    fontnumber = 1;
    ClearMScreen ();
    VWBL_DrawPic(112, 184, C_MOUSELBACKPIC);
    DrawStripes (10);
    SETFONTCOLOR (TEXTCOLOR, BKGDCOLOR);

#ifndef SPEAR
    DrawWindow (CTL_X - 2, CTL_Y - 6, 280, 13*7, BKGDCOLOR);
#else
    DrawWindow (CTL_X - 2, CTL_Y - 26, 280, 13*10, BKGDCOLOR);
#endif

    DrawMenu (&MusicItems, &MusicMenu[start]);

    SETFONTCOLOR (READHCOLOR, BKGDCOLOR);
    PrintY = 15;
    WindowX = 0;
    WindowY = 320;
    US_CPrint ("Robert's Jukebox");

    SETFONTCOLOR (TEXTCOLOR, BKGDCOLOR);
    VW_UpdateScreen();
    MenuFadeIn();

    do
    {
        which = HandleMenu(&MusicItems, &MusicMenu[start], NULL);
        if (which >= 0)
        {
            if (lastsong >= 0)
                MusicMenu[start + lastsong].active = 1;

            StartCPMusic(songs[start + which]);
            MusicMenu[start + which].active = 2;
            DrawMenu (&MusicItems, &MusicMenu[start]);
            VW_UpdateScreen();
            lastsong = which;
        }
    }
    while (which >= 0);

    MenuFadeOut();
    IN_ClearKeysDown();
#ifdef SPEAR

    UnCacheLump (BACKDROP_LUMP_START, BACKDROP_LUMP_END);
#else

    UnCacheLump (CONTROLS_LUMP_START, CONTROLS_LUMP_END);
#endif
}
#endif

/*
==========================
=
= InitGame
=
= Load a few things right away
=
==========================
*/

void InitGame (void)
{
#ifndef SPEARDEMO
    boolean didjukebox = false;
#endif

    SignonScreen ();

    IN_Startup ();
    PM_Startup ();
    SD_Startup ();
    CA_Startup ();
#ifdef NOTYET
    US_Startup ();
#endif

    // TODO: Will any memory checking be needed someday??
#ifdef ABCAUS
#ifndef SPEAR

    if (mminfo.mainmem < 235000L)
#else

    if (mminfo.mainmem < 257000L && !MS_CheckParm("debugmode"))
#endif

    {
        byte *screen;

        CA_CacheGrChunk (ERRORSCREEN);
        screen = grsegs[ERRORSCREEN];
        ShutdownId();
        memcpy((byte *)0xb8000, screen + 7 + 7*160, 17*160);
        gotoxy (1, 23);
        exit(1);
    }
#endif


    //
    // build some tables
    //
    InitDigiMap ();

    ReadConfig ();

    //
    // HOLDING DOWN 'M' KEY?
    //
#ifndef SPEARDEMO

    if (Keyboard[sc_M])
    {
        DoJukebox();
        didjukebox = true;
    }
    else
#endif

    //
    // draw intro screen stuff
    //
    IntroScreen ();

    //
    // load in and lock down some basic chunks
    //

    CA_CacheGrChunk(STARTFONT);

    LoadLatchMem ();
    BuildTables ();          // trig tables
    SetupWalls ();

    NewViewSize (viewsize);

    //
    // initialize variables
    //
    InitRedShifts ();
#ifndef SPEARDEMO

    if (!didjukebox)
#endif

        FinishSignon();

//    vdisp = (byte *) (0xa0000 + PAGE1START);
//    vbuf = (byte *) (0xa0000 + PAGE2START);
}

//===========================================================================

/*
==========================
=
= SetViewSize
=
==========================
*/

boolean SetViewSize (unsigned width, unsigned height)
{
    viewwidth = width & ~15;                  // must be divisable by 16
    viewheight = height & ~1;                 // must be even
    centerx = viewwidth / 2 - 1;
    shootdelta = viewwidth / 10;
    viewscreenx = (screenwidth - viewwidth) / 2;
    viewscreeny = (screenheight - STATUSLINES - viewheight) / 2;
//    screenofs = ((screenheight - STATUSLINES - viewheight) / 2 * SCREENWIDTH + (screenwidth - viewwidth) / 8);
    screenofs = viewscreeny * screenpitch + viewscreenx;

    //
    // calculate trace angles and projection constants
    //
    CalcProjection (FOCALLENGTH);

    return true;
}


void ShowViewSize (int width)
{
    int oldwidth, oldheight;

    oldwidth = viewwidth;
    oldheight = viewheight;

    viewwidth = width * 16;
    viewheight = (int) (width * 16 * HEIGHTRATIO);
    DrawPlayBorder ();

    viewheight = oldheight;
    viewwidth = oldwidth;
}


void NewViewSize (int width)
{
    viewsize = width;
    SetViewSize (width * 16, (int) (width * 16 * HEIGHTRATIO));
}



//===========================================================================

/*
==========================
=
= Quit
=
==========================
*/

void Quit (char *error)
{
    byte *screen;

    if (!pictable)  // don't try to display the red box before it's loaded
    {
        ShutdownId();
        if (error && *error)
        {
#ifdef NOTYET
            SetTextCursor(0, 0);
#endif
            puts(error);
#ifdef NOTYET
            SetTextCursor(0, 2);
#endif
            VW_WaitVBL(100);
        }
        exit(1);
    }

    if (!error || !*error)
    {
#ifndef JAPAN
        CA_CacheGrChunk (ORDERSCREEN);
        screen = grsegs[ORDERSCREEN];
#endif

        WriteConfig ();
    }
    else
    {
        CA_CacheGrChunk (ERRORSCREEN);
        screen = grsegs[ERRORSCREEN];
    }

    ShutdownId ();

#ifdef NOTYET
    if (error && *error)
    {
        memcpy((byte *)0xb8000, screen + 7, 7*160);
        SetTextCursor(0, 0);
        puts(" ");      // temp fix for screen pointer assignment
        SetTextCursor(9, 3);
        puts(error);          // printf(error);
        SetTextCursor(0, 7);
        VW_WaitVBL(200);
        exit(1);
    }
    else
        if (!error || !(*error))
        {
#ifndef JAPAN
            memcpy((byte *)0xb8000, screen + 7, 24*160); // 24 for SPEAR/UPLOAD compatibility
#endif

            SetTextCursor(0, 0);
            puts(" ");      // temp fix for screen pointer assignment
            SetTextCursor(0, 23);
        }
#endif

    exit(0);
}

//===========================================================================



/*
=====================
=
= DemoLoop
=
=====================
*/

static char *ParmStrings[] = {"baby", "easy", "normal", "hard", ""};

void DemoLoop (void)
{
    static int LastDemo;
    int i, level;

//    NoWait = true;      // TODO: Remove this!

    //
    // check for launch from ted
    //
    if (tedlevel)
    {
        NoWait = true;
        NewGame(1, 0);

#ifdef NOTYET
        for (i = 1; i < __argc; i++)
        {
            if ( (level = US_CheckParm(__argv[i], ParmStrings)) != -1)
            {
                gamestate.difficulty = level;
                break;
            }
        }
#endif

#ifndef SPEAR
        gamestate.episode = tedlevelnum / 10;
        gamestate.mapon = tedlevelnum % 10;
#else

        gamestate.episode = 0;
        gamestate.mapon = tedlevelnum;
#endif

        GameLoop();
        Quit (NULL);
    }

    //
    // main game cycle
    //

#ifndef DEMOTEST

#ifndef UPLOAD

#ifndef GOODTIMES
 #ifndef SPEAR
 #ifndef JAPAN
    if (!NoWait)
        NonShareware();
#endif
 #else
 #ifndef GOODTIMES
 #ifndef SPEARDEMO

    extern void CopyProtection(void);
    CopyProtection();
#endif
 #endif
 #endif
 #endif
 #endif

    StartCPMusic(INTROSONG);

#ifndef JAPAN
    if (!NoWait)
        PG13 ();
#endif

#endif

    while (1)
    {
        while (!NoWait)
        {
            //
            // title page
            //
#ifndef DEMOTEST

#ifdef SPEAR
            CA_CacheGrChunk (TITLEPALETTE);

            CA_CacheGrChunk (TITLE1PIC);
            VWBL_DrawPic (0, 0, TITLE1PIC);
            UNCACHEGRCHUNK (TITLE1PIC);

            CA_CacheGrChunk (TITLE2PIC);
            VWBL_DrawPic (0, 80, TITLE2PIC);
            UNCACHEGRCHUNK (TITLE2PIC);
            VW_UpdateScreen ();
            VL_FadeIn(0, 255, grsegs[TITLEPALETTE], 30);

            UNCACHEGRCHUNK (TITLEPALETTE);
#else
            CA_CacheScreen (TITLEPIC);
//            VW_UpdateScreen ();
            VW_FadeIn();
#endif

            if (IN_UserInput(TickBase*15))
                break;
            VW_FadeOut();
            //
            // credits page
            //
            CA_CacheScreen (CREDITSPIC);
//            VW_UpdateScreen();
            VW_FadeIn ();
            if (IN_UserInput(TickBase*10))
                break;
            VW_FadeOut ();
            //
            // high scores
            //
            DrawHighScores ();
            VW_UpdateScreen ();
            VW_FadeIn ();

            if (IN_UserInput(TickBase*10))
                break;
#endif
            //
            // demo
            //

#ifndef SPEARDEMO
            PlayDemo (LastDemo++ % 4);
#else
            PlayDemo (0);
#endif

            if (playstate == ex_abort)
                break;
            StartCPMusic(INTROSONG);
        }

        VW_FadeOut ();

#ifdef DEBUGKEYS
#ifndef SPEAR
        if (Keyboard[sc_Tab] && MS_CheckParm("goobers"))
#else
        if (Keyboard[sc_Tab] && MS_CheckParm("debugmode"))
#endif
            RecordDemo ();
        else
            US_ControlPanel (0);
#else
        US_ControlPanel (0);
#endif

        if (startgame || loadedgame)
        {
            GameLoop ();
            VW_FadeOut();
            StartCPMusic(INTROSONG);
        }
    }
}


//===========================================================================

int ToggleFullscreen(int)
{
    fullscreen = !fullscreen;

    // save current screen
    SDL_BlitSurface(screen, NULL, fizzleSurface, NULL);

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    screen = SDL_SetVideoMode(screenwidth, screenheight, 8,
        SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_HWPALETTE
        | (fullscreen ? SDL_FULLSCREEN : 0));
    if(screen == NULL)
    {
        printf("Failed to toggle fullscreen!\n");
        fullscreen = !fullscreen;
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        ProcessEvents();
        SDL_InitSubSystem(SDL_INIT_VIDEO);
        screen = SDL_SetVideoMode(screenwidth, screenheight, 8,
            SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_HWPALETTE
            | (fullscreen ? SDL_FULLSCREEN : 0));
        if(screen == NULL)
        {
            printf("Unable to restore previous video mode!\n");
            Quit(NULL);
        }
    }
    screenpitch = screen->pitch;
    screenofs = viewscreeny * screenpitch + viewscreenx;
    SDL_ShowCursor(SDL_DISABLE);
    SDL_SetColors(screen, gamepal, 0, 256);
    SDL_SetColors(fizzleSurface, gamepal, 0, 256);

    // restore current screen
    SDL_BlitSurface(fizzleSurface, NULL, screen, NULL);
    if(fizzlein)
        InitFizzleFade();

    IN_ClearKeysDown();
    return 0;
}

void ProcessEvents()
{
    SDL_Event event;
    SDLMod mod;
    while (SDL_PollEvent(&event))
    {
        // check for messages
        switch (event.type)
        {
            // exit if the window is closed
            case SDL_QUIT:
                Quit(NULL);
            // check for keypresses
            case SDL_KEYDOWN:
                LastScan = event.key.keysym.sym;
                mod = SDL_GetModState();
//                if((mod & KMOD_ALT))
                if(Keyboard[sc_Alt])
                {
                    if(event.key.keysym.sym==SDLK_F4)
                        Quit(NULL);
                    if(event.key.keysym.sym==SDLK_RETURN)
                    {
                        ToggleFullscreen(0);
                        return;
                    }
                }
                int sym = event.key.keysym.sym;
                if(sym >= 'a' && sym <= 'z')
                    sym -= 32;  // convert to uppercase

                if(mod & (KMOD_SHIFT | KMOD_CAPS))
                {
                    if(ShiftNames[sym])
                        LastASCII = ShiftNames[sym];
                }
                else
                {
                    if(ASCIINames[sym])
                        LastASCII = ASCIINames[sym];
                }
                if(LastScan<SDLK_LAST)
                    Keyboard[LastScan] = 1;
                if(LastScan == SDLK_PAUSE)
                    Paused = true;
                break;
            case SDL_KEYUP:
                if(event.key.keysym.sym<SDLK_LAST)
                    Keyboard[event.key.keysym.sym] = 0;
                break;
        }
    }

//    Keyboard = SDL_GetKeyState(NULL);
}


/*
==========================
=
= main
=
==========================
*/

int main ( int argc, char** argv )
{
	_fmode=O_BINARY;		// DON'T create save games in text mode!!
	setvbuf(stdout,NULL,_IONBF,0);	// disable stdout buffer, so printf immediately prints
    putenv("SDL_VIDEODRIVER=directx");

    // initialize SDL video
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) < 0 )
    {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
        return 1;
    }

    // make sure SDL cleans up before exit
    atexit(SDL_Quit);

/*    // create a new window
    screen = SDL_SetVideoMode(320, 200, 8,
            SDL_HWSURFACE | SDL_DOUBLEBUF); // | SDL_FULLSCREEN);
    if ( !screen )
    {
        printf("Unable to set 640x480 video: %s\n", SDL_GetError());
        return 1;
    }*/

#ifdef BETA
    //
    // THIS IS FOR BETA ONLY!
    //
    struct dosdate_t d;

    _dos_getdate(&d);
    if (d.year > YEAR ||
            (d.month >= MONTH && d.day >= DAY))
    {
        printf("Sorry, BETA-TESTING is over. Thanks for you help.\n");
        exit(1);
    }
#endif

    CheckForEpisodes();

    InitGame ();

    DemoLoop();

    Quit("Demo loop exited???");
    return 0;
}
