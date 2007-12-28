Wolf4SDL by Moritz "Ripper" Kroll (http://www.chaos-software.de.vu)
Original Wolfenstein 3D by id Software (http://www.idsoftware.com)
=============================================================================

Wolf4SDL is an open-source port of Wolfenstein 3D to the cross-plattform
multimedia library "Simple DirectMedia Layer (SDL)" (http://www.libsdl.org).

Supported operating systems should be at least:
 - Windows 2000, Windows XP, Windows Vista
 - Linux
 - BSD variants

This port includes the OPL2 emulator from MAME, so you can not only hear the
Adlib sounds but also music without any Adlib-compatible soundcards!
Digitized sounds are played on 8 channels! So in a fire fight you will always
hear, when a guard opens the door behind you ;)

Higher screen resolutions (multiples of 320x200, default is 640x400) can be
set using the --res parameter (start Wolf4SDL with --help to see usage).

To play Wolfenstein 3D with Wolf4SDL, you just have to copy the original WL6
files into the same directory as the Wolf4SDL executable. If you want to use
Wolf4SDL with the shareware version (tested) or with Spear (not tested), you
can compile Wolf4SDL with other defines set at the beginning of "wl_def.h".

The current version of the source code is available in the svn repository at:
   svn://tron.homeunix.org:3690/wolf3d/trunk


Known bugs:
 - There seem to exist some problems with the palette sometimes. Then some or
   all colors are wrong. This only appears in fullscreen mode, so consider
   using the --windowed option in that case.

Credits:
 - Special thanks to id Software! Without the source code we would still have
   to throw with hex editors and disassemblers after Wolfenstein 3D ;D
 - Many thanks to "Der Tron" for hosting the svn repository, making Wolf4SDL
   FreeBSD compatible, testing and bugfixing!
