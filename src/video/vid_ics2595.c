/* Copyright holders: Sarah Walker
   see COPYING for more details
*/
/*ICS2595 clock chip emulation
  Used by ATI Mach64*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include "../ibm.h"
#include "vid_ics2595.h"


enum
{
        ICS2595_IDLE = 0,
        ICS2595_WRITE,
        ICS2595_READ
};


static int ics2595_div[4] = {8, 4, 2, 1};


void ics2595_write(ics2595_t *ics2595, int strobe, int dat)
{
        if (strobe)
        {
                if ((dat & 8) && !ics2595->oldfs3) /*Data clock*/
                {
                        switch (ics2595->state)
                        {
                                case ICS2595_IDLE:
                                ics2595->state = (dat & 4) ? ICS2595_WRITE : ICS2595_IDLE;
                                ics2595->pos = 0;
                                break;
                                case ICS2595_WRITE:
                                ics2595->dat = (ics2595->dat >> 1);
                                if (dat & 4)
                                        ics2595->dat |= (1 << 19);
                                ics2595->pos++;
                                if (ics2595->pos == 20)
                                {
                                        int d, n, l;
                                        l = (ics2595->dat >> 2) & 0xf;
                                        n = ((ics2595->dat >> 7) & 255) + 257;
                                        d = ics2595_div[(ics2595->dat >> 16) & 3];

                                        ics2595->clocks[l] = (14318181.8 * ((double)n / 46.0)) / (double)d;
                                        ics2595->state = ICS2595_IDLE;
                                }
                                break;                                                
                        }
                }
                        
                ics2595->oldfs2 = dat & 4;
                ics2595->oldfs3 = dat & 8;
        }
        ics2595->output_clock = ics2595->clocks[dat];
}
