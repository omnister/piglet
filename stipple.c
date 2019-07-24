#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include "db.h"
#include "xwin.h"

#define STIPW 8
#define STIPH 8 
#define MAXSTIP 100

extern Display *dpy; 
extern int scr;
extern Window win;

Pixmap stipple[MAXSTIP];

unsigned char stip_src[MAXSTIP][STIPW] = {

    /* EQU :F1 solid fill */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* solid fill */
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* solid fill */
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* solid fill */
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* solid fill */
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* solid fill */
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* solid fill */
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* solid fill */
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* solid fill */
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* solid fill */
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* solid fill */

    /* (EQU :F2) family of ten 45 degree stipples, each offset */
    /* by one pixel.  last two are doubled */
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01}, /* 8x+0  45d */
    {0x01, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02}, /* 8x+1  45d */
    {0x02, 0x01, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04}, /* 8x+2  45d */
    {0x04, 0x02, 0x01, 0x80, 0x40, 0x20, 0x10, 0x08}, /* 8x+3  45d */
    {0x08, 0x04, 0x02, 0x01, 0x80, 0x40, 0x20, 0x10}, /* 8x+4  45d */
    {0x10, 0x08, 0x04, 0x02, 0x01, 0x80, 0x40, 0x20}, /* 8x+5  45d */
    {0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x80, 0x40}, /* 8x+6  45d */
    {0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x80}, /* 8x+7  45d */
    {0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x80, 0x40}, /* 8x+6  45d */
    {0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x80}, /* 8x+7  45d */

    /* (EQU :F3) family of ten 135 degree stipples, each offset */
    /* by one pixel.  last two are doubled */

    {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80}, /* 8x+0 135d */
    {0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40}, /* 8x+1 135d */
    {0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20}, /* 8x+2 135d */
    {0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10}, /* 8x+3 135d */
    {0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08}, /* 8x+4 135d */
    {0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04}, /* 8x+5 135d */
    {0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02}, /* 8x+6 135d */
    {0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01}, /* 8x+7 135d */
    {0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02}, /* 8x+6 135d */
    {0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01}, /* 8x+7 135d */

    /* (EQU :F4) family of ten 4x4 single point stipples */

    {0x11, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00}, /* 4x4,1  */
    {0x44, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00}, /* 4x4,2  */
    {0x88, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00}, /* 4x4,3  */
    {0x00, 0x11, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00}, /* 4x4,4  */
    {0x00, 0x22, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00}, /* 4x4,5  */
    {0x00, 0x44, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00}, /* 4x4,6  */
    {0x00, 0x88, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00}, /* 4x4,7  */
    {0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x44, 0x00}, /* 4x4,8  */
    {0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x88, 0x00}, /* 4x4,9  */
    {0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x11, 0x00}, /* 4x4,10 */

    /* (EQU :F5) family of ten horizontal period 8 stipples */

    {0x11, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00}, /* 4x4,1  */
    {0x44, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00}, /* 4x4,2  */
    {0x88, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00}, /* 4x4,3  */
    {0x00, 0x11, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00}, /* 4x4,4  */
    {0x00, 0x22, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00}, /* 4x4,5  */
    {0x00, 0x44, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00}, /* 4x4,6  */
    {0x00, 0x88, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00}, /* 4x4,7  */
    {0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x11, 0x00}, /* 4x4,8  */
    {0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x22, 0x00}, /* 4x4,9  */
    {0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x44, 0x00}, /* 4x4,10 */


    /* (EQU :F6) family of ten horizontal period 8 stipples */
    {0xf0, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00},
    {0x00, 0xf0, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00},
    {0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x0f, 0x00},
    {0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x0f},
    {0x0f, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00},
    {0x00, 0x0f, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00},
    {0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0xf0, 0x00},
    {0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0xf0},
    {0xf0, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00},
    {0x00, 0xf0, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00}
};

void init_stipples() {
    int i,j;
    unsigned char a;
    for (i=0; i<50; i++) {	/* deterministic stipples */
	stipple[i] = XCreateBitmapFromData(dpy, win, (const char *)stip_src[i], 
	    (unsigned int) STIPW, (unsigned int) STIPH);
    }
    for (; i<MAXSTIP; i++) {	/* make random stipples */
    	for (j=0; j<STIPW; j++) {
	    a = (unsigned char) (drand48()*255.0);
	    a &= (unsigned char) (drand48()*255.0);
	    a &= (unsigned char) (drand48()*255.0);
	    a &= (unsigned char) (drand48()*255.0);
	    /* a &= (unsigned char) (drand48()*255.0); */
	    stip_src[i][j] = (unsigned char) a;
	}
	stipple[i] = XCreateBitmapFromData(dpy, win, (const char *)stip_src[i],
	    (unsigned int) STIPW, (unsigned int) STIPH);
    }
}

int get_stipple_index(int fill, int pen) {
   int debug=0;
   if (debug) printf("fill %d, pen %d, index %d\n", fill, pen, (fill-1)*10+pen);
   // fill 1, pen 0, index 0
   if (fill==0) {
      return(0);
   } else {
       return((fill-1)*10+pen);
   }
}

unsigned char *get_stipple_bits(int i) {

   if (i >= MAXSTIP-1) {
      return NULL;
   } else {
       return(&stip_src[i][0]);
   }
}

const char * get_hpgl_fill(int fill)
{
    switch (fill % 8) {
    case 0:
	return ("FT10,0;");
	break;
    case 1:
	return ("FT10,50;");
	break;
    case 2:
	return ("FT3,3,45;");
	break;
    case 3:
	return ("FT3,4,135;");
	break;
    case 4:
	return ("FT3,5,0;");
	break;
    case 5:
	return ("FT3,4,90;");
	break;
    case 6:
	return ("FT4,3,0;");
	break;
    case 7:
	return ("FT4,4,45;");
	break;
    }
    return NULL;
}

