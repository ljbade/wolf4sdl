#ifndef _VERSION_H_
#define _VERSION_H_

/* Defines used for different versions */

//#define SPEAR
//#define SPEARDEMO
//#define UPLOAD
#define GOODTIMES

/*
    Wolf3d Full v1.4 GT/ID/Activision         - define GOODTIMES
    Wolf3d Shareware v1.4                     - define UPLOAD
    Wolf3d Full v1.4 Apogee (with ReadThis)   - define none
    Spear of Destiny Full                     - define SPEAR (and GOODTIMES for no FormGen quiz)
    Spear of Destiny Demo                     - define SPEAR and SPEARDEMO

    Wolf3d Full v1.1 and Shareware v1.0-1.1   - can be added by the user
*/

//#define USE_FEATUREFLAGS    // Enables the level feature flags (see bottom of wl_def.h)
//#define USE_FLOORCEILINGTEX // Enables floor and ceiling textures stored in the third mapplane (see wl_floorceiling.cpp)
//#define USE_HIRES           // Enables high resolution textures/sprites (128x128)
//#define USE_PARALLAX 16     // Enables parallax sky with 16 textures per sky (see wl_parallax.cpp)
//#define USE_CLOUDSKY        // Enables cloud sky support (see wl_cloudsky.cpp)
//#define USE_STARSKY         // Enables star sky support (see wl_atmos.cpp)
//#define USE_RAIN            // Enables rain support (see wl_atmos.cpp)
//#define USE_SNOW            // Enables snow support (see wl_atmos.cpp)

#define DEBUGKEYS           // Comment this out to compile without the Tab debug keys
#define ARTSEXTERN
#define DEMOSEXTERN
#define CARMACIZED

#endif
