Wolf4SDL by Moritz "Ripper" Kroll (http://www.chaos-software.de.vu)
Original Wolfenstein 3D by id Software (http://www.idsoftware.com)
=============================================================================

Wolf4SDL is an open-source port of Wolfenstein 3D to the cross-plattform
multimedia library "Simple DirectMedia Layer (SDL)" (http://www.libsdl.org).

Supported operating systems are at least:
 - Windows 98, Windows ME, Windows 2000, Windows XP, Windows Vista
 - Linux
 - BSD variants

Only little endian platforms like x86, ARM and SH-4 are supported, yet.

This port includes the OPL2 emulator from MAME, so you can not only hear the
Adlib sounds but also music without any Adlib-compatible soundcards!
Digitized sounds are played on 8 channels! So in a fire fight you will always
hear, when a guard opens the door behind you ;)

Higher screen resolutions (multiples of 320x200, default is 640x400) can be
set using the --res parameter (start Wolf4SDL with --help to see usage).

The following versions of Wolfenstein 3D data files are currently supported
by the source code (choose the version by commenting/uncommenting lines in
version.h as described in that file):

 - Wolfenstein 3D v1.1 full Apogee
 - Wolfenstein 3D v1.4 full Apogee
 - Wolfenstein 3D v1.4 full GT/ID/Activision
 - Wolfenstein 3D v1.0 shareware Apogee
 - Wolfenstein 3D v1.1 shareware Apogee
 - Wolfenstein 3D v1.2 shareware Apogee
 - Wolfenstein 3D v1.4 shareware
 - Spear of Destiny full
 - Spear of Destiny demo

How to play:

To play Wolfenstein 3D with Wolf4SDL, you just have to copy the original WL6
files into the same directory as the Wolf4SDL executable. If you want to use
Wolf4SDL with the shareware version or with Spear, you can compile Wolf4SDL
with other defines in "version.h".

If you play in windowed mode (--windowed parameter), press SCROLLLOCK or F12
to grab the mouse. Press it again to release the mouse.


Compiling from source:

The current version of the source code is available in the svn repository at:
   svn://tron.homeunix.org:3690/wolf3d/trunk

The following ways of compiling the source code are supported:
 - Makefile (for Linux, BSD variants and MinGW/MSYS)
 - Visual C++ 2005 or above (Wolf4SDL.sln and Wolf4SDL.vcproj)
 - Visual C++ 6 (Wolf4SDL.dsw and Wolf4SDL.dsp)
 - Code::Blocks 8.02 (Wolf4SDL.cbp)
 - Dev-C++ 5.0 Beta 9.2 (4.9.9.2) (Wolf4SDL.dev) (see README-devcpp.txt)

To compile the source code you need the development libraries of
 - SDL (http://www.libsdl.org/download-1.2.php) and
 - SDL_mixer (http://www.libsdl.org/projects/SDL_mixer/)
and have to adjust the include and library paths in the projects accordingly.

Please note, that there is no official SDL_mixer development pack for MinGW,
yet, but you can get the needed files from a Dev-C++ package here:
http://sourceforge.net/project/showfiles.php?group_id=94270&package_id=151751
Just rename the file extension from ".devpack" to ".tar.bz2" and unpack it
with for example WinRAR. Then add the directories include/SDL and lib to the
according search paths in your project.


Known bugs:
 - There seem to exist some problems with the palette sometimes. Then some or
   all colors are wrong. This only appears in fullscreen mode. If you restart
   the game one or two times, it should be OK (smells like a race condition).
   You can press ALT+F4 to leave the game faster. If it does not help you,
   consider using the --windowed option.
 - Demos don't work very well...


Credits:
 - Special thanks to id Software! Without the source code we would still have
   to pelt Wolfenstein 3D with hex editors and disassemblers ;D
 - Many thanks to "Der Tron" for hosting the svn repository, making Wolf4SDL
   FreeBSD compatible, testing, bugfixing and cleaning up the code!
 - Thanks to Chris for his improvements on Wolf4GW, on which Wolf4SDL bases.
 - Thanks to Pickle for the GP2X support
 - Thanks to fackue for the Dreamcast support


Licenses:
 - The original source code of Wolfenstein 3D: license-id.txt
 - The OPL2 emulator (fmopl.cpp): license-mame.txt
