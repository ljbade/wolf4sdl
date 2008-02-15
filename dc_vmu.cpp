/*
	dc_vmu.cpp
	This file is part of the Wolf4SDL\DC project.
	It has the LCD drawing and soon saving functions.

	LCD drawing code found in Sam Steele's DreamZZT.

	Cyle Terry <cyle.terry@gmail.com>
*/

#ifdef _arch_dreamcast

#include <kos.h>
#include "wl_def.h"
#include "dc_vmu.h"

maple_device_t *lcds[8];
uint8 bitmap[48*32/8];

void StatusDrawLCD(int idxLCD) {
    const char *c;
    int x, y, xi, xb, i;
    maple_device_t *dev;

    memset(bitmap, 0, sizeof(bitmap));

    c = BJFacesLCD[idxLCD-FACE1APIC];

    if(c) {
        for(y = 0; y < 32; y++)	{
            for(x = 0; x < 48; x++)	{
                xi = x / 8;
                xb = 0x80 >> (x % 8);
                if(c[(31 - y) * 48 + (47 - x)] == '.')
                    bitmap[y * (48 / 8) + xi] |= xb;
            }
        }
    }

    i = 0;
    while( (dev = maple_enum_type(i++, MAPLE_FUNC_LCD)) ) {
        vmu_draw_lcd(dev, bitmap);
    }

    vmu_shutdown();
}

#endif // _arch_dreamcast
