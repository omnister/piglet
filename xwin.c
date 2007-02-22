#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include "eventnames.h"
#include <errno.h>
#include <math.h>

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include <readline/readline.h>
#include <readline/history.h>     

#include "db.h"
#include "xwin.h"
#include "token.h"
#include "eprintf.h"
#include "rubber.h"
#include "lex.h"
#include "readmenu.h"
#include "path.h"

extern void do_win();

#define MAX_MENU 256

void load_font();
int init_colors();
void xwin_doXevent();
void getGC();
int draw_grid();
void paint_pane(); 
int dump_window(); 
int xwin_display_state();
extern Pixmap stipple[];

/* globals for interacting with db.c */
DB_TAB *currep = NULL;		/* keep track of current rep */
XFORM  unity_transform;
/*XFORM  screen_transform; */
/*XFORM  *xp = &screen_transform; */

int quit_now; /* when != 0 ,  means the user is done using this program. */

char version[] = "$Id: xwin.c,v 1.46 2007/02/11 17:12:31 walker Exp walker $";

unsigned int top_width, top_height;	/* main window pixel size    */
unsigned int g_width, g_height;		/* graphic window pixel size */
unsigned int menu_width;		/* menu window pixel size    */
unsigned int dpy_width, dpy_height;	/* display pixel size        */

double vp_xmin, vp_ymin, vp_xmax, vp_ymax;
double scale   = 1.0;			/* xform factors from real->viewport */
double xoffset = 0.0;			
double yoffset = 0.0;

/* these grid variable are only used during MAIN opening screen */
/* otherwise currep is the place for the current grid */
double grid_xd = 10.0;	/* grid delta */
double grid_yd = 10.0;
double grid_xs = 2.0;	/* how often to display grid ticks */
double grid_ys = 2.0;
double grid_xo = 0.0; 	/* starting offset */
double grid_yo = 0.0;


GRIDSTATE grid_state = G_ON;		/* ON, OFF */
DISPLAYSTATE display_state = D_ON;	/* ON, OFF */
int grid_color = 1;	/* 1 through 6 for different colors */ 
int grid_notified = 0;

int need_redraw=0;

static double x,y;
static int xa,ya; 	/* remember raw button coords */
static int xb,yb; 	/* remember raw button coords */
static int xr,yr; 	/* remember raw button coords */
static int moved=0;

#define icon_bitmap_width 20
#define icon_bitmap_height 20
static char icon_bitmap_bits[] = {
    0x60, 0x00, 0x01, 0xb0, 0x00, 0x07, 0x0c, 0x03, 0x00, 0x04, 0x04, 0x00,
    0xc2, 0x18, 0x00, 0x03, 0x30, 0x00, 0x01, 0x60, 0x00, 0xf1, 0xdf, 0x00,
    0xc1, 0xf0, 0x01, 0x82, 0x01, 0x00, 0x02, 0x03, 0x00, 0x02, 0x0c, 0x00,
    0x02, 0x38, 0x00, 0x04, 0x60, 0x00, 0x04, 0xe0, 0x00, 0x04, 0x38, 0x00,
    0x84, 0x06, 0x00, 0x14, 0x14, 0x00, 0x9c, 0x34, 0x00, 0x00, 0x00, 0x00};

#define BITMAPDEPTH 1
#define TOO_SMALL 0
#define BIG_ENOUGH 1

/* for select in XEvent loop */
#ifndef FD_SET
#define FD_SET(n, p)    ((p)->fds_bits[0] |= (1 << ((n) % 32)))
#define FD_CLR(n, p)    ((p)->fds_bits[0] &= ~(1 << ((n) % 32)))
#define FD_ISSET(n, p)  ((p)->fds_bits[0] & (1 << ((n) % 32)))
#define FD_ZERO(p)      bzero((char *)(p), sizeof(*(p)))
#endif /* !FD_SET */

extern char *getwd();

Display *dpy; int scr;

extern int errno;

#define BUF_SIZE 128

Window topwin;
Window win;
Window menuwin;
GC gca, gcb;
GC gcx, gcg;

/* Menu definitions */

#define NONE 100
#define BLACK 1
#define WHITE 0

Window inverted_pane = NONE;
int num_menus;
MENUENTRY menutab[MAX_MENU];

XFontStruct *font_info;

#define MAX_COLORS 10
unsigned long colors[MAX_COLORS];    /* will hold pixel values for colors */
#define MAX_LINETYPE 7

int initX()
{
    double x,y;
    unsigned int border_width = 4;
    extern unsigned int dpy_width, dpy_height;
    char *window_name = "PD_Piglet: Personal Interactive Graphic Layout EdiTor";
    char *icon_name = "rigel";	/* rick's interactive graphic editor */
    Pixmap icon_pixmap;
    XSizeHints *size_hints;
    XIconSize *size_list;
    XWMHints *wm_hints;
    XClassHint *class_hints;
    XTextProperty windowName, iconName;
    XSetWindowAttributes attr;
    int icount;
    char *dpy_name = NULL;

    int debug=0;

    /* menu stuff */
    char *string;
    int char_count;
    int direction, ascent, descent;
    int pane_height;
    int menu_height;
    XCharStruct overall;
    int winindex;
    int linewidth;
    int numrows;
    char buf[128];

    if (!(size_hints = XAllocSizeHints())) {
	eprintf("failure allocating SizeHints memory");
    }
    if (!(wm_hints = XAllocWMHints())) {
	eprintf("failure allocating WMHints memory");
        exit (0);
    }
    if (!(class_hints = XAllocClassHint())) {
        eprintf("failure allocating ClassHint memory");
    }

    /* Connect to X server */
    if ( (dpy=XOpenDisplay(dpy_name)) == NULL ) {
        eprintf("cannot connect to X server %s",XDisplayName(dpy_name) );
    }

    /* Get screen size from display structure macro */
    scr = DefaultScreen(dpy);
    dpy_width = DisplayWidth(dpy, scr);
    dpy_height = DisplayHeight(dpy, scr);

    /* eventually we want to set x,y from command line or
       resource database */

    /* suggest that window be placed in upper left corner */
    x=y=2*border_width;

    /* Size window */
    top_width = 3*dpy_width/4, top_height = 3*dpy_height/4;

    /* figure out menu sizes */
    string = "X";
    char_count = strlen(string);

    load_font(&font_info);

    findfile(PATH, "MENUDATA_V", buf, R_OK);
    if (buf[0] == '\0') {
	printf("Could not file MENUDATA.F\n");
	printf("PATH=\"%s\"\n", PATH);
	exit(5);
    } else {
	printf("loading %s from disk\n", buf);
	num_menus=loadmenu(menutab, MAX_MENU, buf, &linewidth, &numrows);
    }

    /* Determine the extent of each menu pane based on font size */
    XTextExtents(font_info, string, char_count, &direction, &ascent,
        &descent, &overall);

    menu_width = overall.width*linewidth + 6;
    pane_height = overall.ascent + overall.descent + 6;
    menu_height = pane_height * numrows;

    /* create top level window for packing graphic and menu windows */
    topwin = XCreateSimpleWindow(dpy, RootWindow(dpy,scr),
        (int) x,(int) y, top_width, top_height, border_width,
	WhitePixel(dpy,scr), BlackPixel(dpy,scr));

    x=y=0.0;
    border_width=1;
    g_width=top_width-menu_width;
    g_height=top_height;

    /* Create opaque window for graphics */
    win = XCreateSimpleWindow(dpy, topwin,
        (int) x,(int) y, g_width, g_height, border_width, 
	WhitePixel(dpy,scr), BlackPixel(dpy,scr));

    /* Create opaque window for menu */
    menuwin = XCreateSimpleWindow(dpy, topwin,
        (g_width)+1, (int) y, menu_width-2, g_height,
	border_width, WhitePixel(dpy,scr), BlackPixel(dpy,scr));

    /* Create the menu boxes */
    for (winindex = 1; winindex <= num_menus; winindex++) {
        menutab[winindex].pane = XCreateSimpleWindow(dpy, menuwin, 
	    overall.width*menutab[winindex].xloc,			/*x*/
	    pane_height*menutab[winindex].yloc, 			/*y*/
	    overall.width*menutab[winindex].width,			/*width*/
	    pane_height,						/*height*/
	    border_width = 1,
	    WhitePixel(dpy,scr),
	    BlackPixel(dpy,scr));
	XSelectInput(dpy, menutab[winindex].pane, ButtonPressMask |
	    ButtonReleaseMask | ExposureMask);
    }

    XMapSubwindows(dpy, menuwin);

    /* Get available icon sizes from window manager */

    if (XGetIconSizes(dpy, RootWindow(dpy, scr), &size_list, &icount) == 0) {
        if (debug) weprintf("WM didn't set icon sizes - using default.");
    } else {
        /* should eventually create a pixmap here */
        ;
    }

    /* Create pixmap of depth 1 (bitmap) for icon */
    icon_pixmap = XCreateBitmapFromData(dpy, win,
        icon_bitmap_bits, icon_bitmap_width, icon_bitmap_height);
    
    size_hints->flags = PPosition | PSize | PMinSize;
    size_hints->min_width =  300;
    size_hints->min_height = 200;

    if (XStringListToTextProperty(&window_name, 1, &windowName) == 0) {
        eprintf("structure allocation for windowName failed.");
    }

    if (XStringListToTextProperty(&icon_name, 1, &iconName) == 0) {
        eprintf("structure allocation for iconName failed.");
    }
    
    wm_hints->initial_state = NormalState;
    wm_hints->input = True;
    wm_hints->icon_pixmap = icon_pixmap;

    wm_hints->flags = StateHint | IconPixmapHint | InputHint;

    class_hints->res_name = estrdup(progname());
    class_hints->res_class = class_hints->res_name;

    XSetWMProperties(dpy, topwin, &windowName, &iconName,
        (char **) NULL, (int) NULL, size_hints, wm_hints, class_hints);

    XFree(windowName.value);
    XFree(iconName.value);
    XFree(size_hints);
    XFree(wm_hints);
    free(class_hints->res_name);
    XFree(class_hints);

    /* Select event types wanted */
    XSelectInput(dpy, topwin, ExposureMask | KeyPressMask |
        ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | 
	Button1MotionMask | PointerMotionMask );
    XSelectInput(dpy, win, ExposureMask | KeyPressMask |
        ButtonPressMask | ButtonReleaseMask | 
	Button1MotionMask | PointerMotionMask );
    XSelectInput(dpy, menuwin, ExposureMask | KeyPressMask |
        ButtonPressMask | ButtonReleaseMask | 
	Button1MotionMask | PointerMotionMask );

    init_colors();

    /* Create GC for text and drawing */
    getGC(win, &gca, font_info);
    getGC(win, &gcb, font_info);	/* gcb for polygon filling */
    getGC(win, &gcg, font_info);	/* gcg for grid */
    getGC(win, &gcx, font_info);	/* gcx for highlighting */

    init_stipples();

    XSetFillStyle(dpy, gcg, FillStippled);	
    XSetFillRule(dpy, gcg, WindingRule);
    XSetFunction(dpy, gcg, GXxor);

    XSetFillStyle(dpy, gcx, FillStippled);
    XSetFillRule(dpy, gcx, WindingRule);
    XSetFunction(dpy, gcx, GXxor);

    XSetStipple(dpy, gcb, stipple[0]);
    XSetFillStyle(dpy, gcb, FillStippled);
    XSetFillRule(dpy, gcb, WindingRule);

    /* dpy windows */
    XMapWindow(dpy, topwin);
    XMapWindow(dpy, win);
    XMapWindow(dpy, menuwin);

    /* turn on backing store */
    attr.backing_store = Always;
    XChangeWindowAttributes(dpy,win,CWBackingStore,&attr);   

    /* initialize various things, should be put into init() function */

    xwin_grid_pts(10.0, 10.0, 2.0, 2.0, 0.0, 0.0);
    xwin_grid_color(1);
    xwin_grid_state(G_ON);
    xwin_display_set_state(G_ON);
    xwin_window_set(-100.0,-100.0, 100.0, 100.0);

    /* initialize unitytransform */
    unity_transform.r11 = 1.0;
    unity_transform.r12 = 0.0;
    unity_transform.r21 = 0.0;
    unity_transform.r22 = 1.0;
    unity_transform.dx  = 0.0;
    unity_transform.dy  = 0.0;

    return(0);
}

void zoom(int dir, double scale)
{
    double xmin,ymin,xmax,ymax;
    extern double x,y;

    if (dir < 0) {
       scale = 1.0/scale;
    }

    if (currep != NULL) {
	xmin = currep->vp_xmin;
	ymin = currep->vp_ymin;
	xmax = currep->vp_xmax;
	ymax = currep->vp_ymax;

	xmin=x-(x-xmin)*scale;
	xmax=x+(xmax-x)*scale;
	ymin=y-(y-ymin)*scale;
	ymax=y+(ymax-y)*scale;

	xwin_window_set(xmin,ymin,xmax,ymax);
    }
}

void winfit() 
{
    double xmin,ymin,xmax,ymax;
    double dx,dy;

    if (currep != NULL ) {
	xmin = currep->minx;
	ymin = currep->miny;
	xmax = currep->maxx;
	ymax = currep->maxy;

	dx=(xmax-xmin);
        dy=(ymax-ymin);
        xmin-=dx/40.0;
        xmax+=dx/40.0;
        ymin-=dy/40.0;
        ymax+=dy/40.0;

	xwin_window_set(xmin,ymin,xmax,ymax);
    } 
}

void pan(double x1, double y1) 
{
    double xmin,ymin,xmax,ymax;
    double dx,dy;

    if (currep != NULL) {
	xmin = currep->vp_xmin;
	ymin = currep->vp_ymin;
	xmax = currep->vp_xmax;
	ymax = currep->vp_ymax;

	dx=(xmax-xmin);
	dy=(ymax-ymin);
	xmin=x1-dx/2.0;
	xmax=x1+dx/2.0;
	ymin=y1-dy/2.0;
	ymax=y1+dy/2.0;

	xwin_window_set(xmin,ymin,xmax,ymax);
    }
}


static int pan_x = 0;
static int pan_y = 0;
void pan_init(int x, int y) {
   pan_x=x;
   pan_y=y;
}

void pan_update(int x, int y) {
   int dx;
   int dy;

   dx = pan_x-x;
   dy = pan_y-y;
   
   if (abs(dx)+abs(dy) > 10) {
       if (dx > dy) {
	  zoom(1,1.1);
       } else {
	  zoom(-1,1.1);
       }
       pan_x=x; pan_y=y;
   }
}

void dosplash() {
    DB_TAB *splashrep;
    extern double scale,xoffset,yoffset;
    double x1,x2,y1,y2;
    double dratio,wratio;
    BOUNDS bb;

    bb.init=0;

    splashrep=db_lookup("piglogo");
    if (splashrep != NULL) {
	x1 = splashrep->vp_xmin; 
	y1 = splashrep->vp_ymin;
	x2 = splashrep->vp_xmax;
	y2 = splashrep->vp_ymax;

	x1 = splashrep->minx; 
	y1 = splashrep->miny;
	x2 = splashrep->maxx;
	y2 = splashrep->maxy;
	
        grid_xd = splashrep->grid_xd;
        grid_yd = splashrep->grid_yd;
        grid_xs = splashrep->grid_xs;
        grid_ys = splashrep->grid_ys;
        grid_xo = splashrep->grid_xo;
        grid_yo = splashrep->grid_yo;

	dratio = (double) g_height/ (double)g_width;
	wratio = (y2-y1)/(x2-x1);

	if (wratio > dratio) {	/* world is taller,skinnier than display */
	    scale=0.9*((double) g_height)/(y2-y1);
	} else {			/* world is shorter,fatter than display */
	    scale=0.9*((double) g_width)/(x2-x1);
	}

	xoffset = (((double) g_width)/2.0) -  ((x1+x2)/2.0)*scale;
	yoffset = (((double) g_height)/2.0) + ((y1+y2)/2.0)*scale;
	db_render(splashrep,0,&bb,0); 
    }

}


int procXevent()
{
    /* readline select stuff */
    int nf, nfds, cn, in; 
    struct timeval *timer = (struct timeval *) 0;
    fd_set rset, tset;
    static char *s = NULL;
    BOUNDS bb;

    /* courtesy g_plt_X11.c code from gnuplot */
    cn = ConnectionNumber(dpy);
    in = fileno(stdin);

    FD_ZERO(&rset);
    FD_SET(cn, &rset);
    FD_SET(in, &rset);

    nfds = (cn > in) ? cn + 1 : in + 1;

    while (1) {

	if (need_redraw) {
	    need_redraw = 0;
	    XClearArea(dpy, win, 0, 0, 0, 0, False);
	    if (currep != NULL ) {
		bb.init=0;
		db_render(currep,0,&bb,0);  /* render the current rep */
		draw_grid(win, gcg, currep->grid_xd, currep->grid_yd,
		    currep->grid_xs, currep->grid_ys, 
		    currep->grid_xo, currep->grid_yo);
		if (xwin_display_state() == D_ON) {
		    rubber_draw(x, y, 1); 
		}
		XFlush(dpy);
	    } else {
		dosplash();
		draw_grid(win, gcg, grid_xd, grid_yd,
		    grid_xs, grid_ys, grid_xo, grid_yo);
	    }
	}

	/* some event resulted in text in buffer 's' */
	/* so feed it back to readline routine */

	if (s != NULL) {
	    if (*s!='\0') {
		return((int) *(s++));
	    } else {
		s = NULL;
	    }
	}

	tset = rset;
	nf = select(nfds, &tset, (fd_set *) 0, (fd_set *) 0, timer);
	if (nf < 0) {
	    if (errno == EINTR)
		continue;
	    eprintf("select failed. errno:%d", errno);
	}

	if (nf > 0) {
	    XFlush(dpy);
	}

	if (FD_ISSET(cn, &tset)) { 		/* pending X Event */
	    xwin_doXevent(&s);
	}

	if (FD_ISSET(in, &tset)) {	/* pending stdin */
	    return(getc(stdin));
	}
    }
}

void xwin_doXevent(s)
char **s;
{
    static int button_down=0;
    XEvent xe;
    int j;
    int debug=0;

    static char buf[BUF_SIZE];
    static char pickbuf[BUF_SIZE];
    static char tmp[BUF_SIZE];
    static double xold,yold;
    unsigned long all = 0xffffffff;

    static int charcount;
    static char keybuf[10];
    static int keybufsize=10;
    KeySym keysym;
    XComposeStatus *compose;
    BOUNDS bb;

    if (need_redraw) {
	need_redraw = 0;
	XClearArea(dpy, win, 0, 0, 0, 0, False);
	if (currep != NULL ) {
	    bb.init=0;
	    db_render(currep,0,&bb,0); /* render the current rep */
	    draw_grid(win, gcg, currep->grid_xd, currep->grid_yd,
		currep->grid_xs, currep->grid_ys,
		currep->grid_xo, currep->grid_yo);
	    if (xwin_display_state() == D_ON) {
		rubber_draw(x, y, 1);
	    }
	    XFlush(dpy);
	} else {
	    dosplash();
	    draw_grid(win, gcg, grid_xd, grid_yd,
		grid_xs, grid_ys, grid_xo, grid_yo);
	}
    }

    while (XCheckMaskEvent(dpy, all, &xe)) { /* pending X Event */
	switch (xe.type) {
	case MotionNotify:
	    if (debug) printf("EVENT LOOP: got Motion\n");

	    if (!button_down) {
		x = (double) xe.xmotion.x;
		y = (double) xe.xmotion.y;
		V_to_R(&x,&y);
		snapxy(&x,&y);
		sprintf(buf,"(%-8g,%-8g)", x, y);
		if (xwin_display_state() == D_ON && xe.xmotion.window == win) {
		    XDrawImageString(dpy,win,gcg,20, 20, buf, strlen(buf));
		}
	    } else {
		xr = xe.xmotion.x;
		yr = xe.xmotion.y;
		pan_update(xr, yr);
		moved=1;
	    }

	    if (xold != x || yold != y) {
		if (xwin_display_state() == D_ON && xe.xmotion.window == win) {  /* RCW */
		    rubber_draw(x, y, 0);
		}
		xold=x;
		yold=y;
	    } 

	    break;
	case Expose:
	    if (debug) printf("EVENT LOOP: got Expose\n");

	    if (currep != NULL) {
		xwin_window_set(
		currep->vp_xmin, 
		currep->vp_ymin,
		currep->vp_xmax,
		currep->vp_ymax );
	    } else {
		xwin_window_set( vp_xmin, vp_ymin, vp_xmax, vp_ymax );
	    }

	    if (xe.xexpose.count != 0)
		break;
	    if (xe.xexpose.window == win) {
		if (currep != NULL ) {
		    draw_grid(win, gcg, currep->grid_xd, currep->grid_yd,
			currep->grid_xs, currep->grid_ys, currep->grid_xo, currep->grid_yo);
		} else {
		    draw_grid(win, gcg, grid_xd, grid_yd,
			grid_xs, grid_ys, grid_xo, grid_yo);
		}
	    }  
	
	    for (j=1; j<=num_menus; j++) {
		 if (menutab[j].pane == xe.xexpose.window) {
		     paint_pane(xe.xexpose.window, menutab, gca, gcb, BLACK);
		 }
	    }

	    break;

	case ConfigureNotify:
	    if (debug) printf("EVENT LOOP: got Config Notify\n");
	    top_width = xe.xconfigure.width;
	    g_height=top_height = xe.xconfigure.height;
	    g_width=top_width-menu_width;
	    XMoveWindow(dpy, menuwin, top_width-menu_width, 0);
	    XResizeWindow(dpy, menuwin, menu_width, g_height);
	    XResizeWindow(dpy, win, top_width-menu_width, g_height);
	    if (currep != NULL) {
		xwin_window_set( currep->vp_xmin, 
				 currep->vp_ymin,
				 currep->vp_xmax, 
				 currep->vp_ymax );
	    } else {
		xwin_window_set( vp_xmin, vp_ymin, vp_xmax, vp_ymax );
	    }
	    break;
	case ButtonRelease:
	    button_down=0;
	    if (debug) printf("EVENT LOOP: got ButtonRelease\n");

	    if (xe.xexpose.window == win) {
		switch (xe.xbutton.button) {
		    case 2:	/* left button */
			if (abs(xa-xb) < 3 && abs(ya-yb) < 3) {
			   winfit();
			   xa = xb = ya = yb = -1; 	/* set to impossible values */
			} else if (!moved || (abs(xb-xr) < 3 && abs(yb-yr) < 3)) {
			   pan(x,y);
			}
		        break;
		    default:
		        break;
		}
	    }
	    moved=0;
	    break;
	case ButtonPress:
	    if (xe.xexpose.window == win) {
		switch (xe.xbutton.button) {
		    case 1:	/* left button */
			x = (double) xe.xmotion.x;
			y = (double) xe.xmotion.y;
		        V_to_R(&x,&y);

			/* FIXME: turn off snapping for fine picking */
			/*
			if (snap==0) {
			    snap=1;
			} else {
			    snapxy(&x,&y);
			}
			*/

			snapxy(&x,&y);

			/* returning mouse loc as a string */
			sprintf(pickbuf," %f, %f\n", x, y);
			*s = pickbuf;

			if (debug) printf("EVENT LOOP: got ButtonPress\n");
			break;
		    case 2:	/* middle button */
			button_down=1;
			xa = xb; ya = yb;	
			xr = xb = xe.xmotion.x;
			yr = yb = xe.xmotion.y;
			pan_init(xb,yb);
			break;
		    case 3: /* right button */
			/* RIGHT button returns EOC */
			sprintf(pickbuf," ;\n");
			*s = pickbuf;
			break;
		    case 4: /* mouse scroll button (up/out) */
		    	zoom(1,1.2);
			break;
		    case 5: /* mouse scroll button (in/down) */
		    	zoom(-1,1.1);
			break;
			/* RIGHT button returns EOC */
		    default:
			/* just ignore unexpected events... */
			/* my laptop touchpad mouse makes random */
			/* button #7 events all the time (?) */
			/* printf("a: unexpected button event: %d",
			    xe.xbutton.button); */
			break;
		}
	    } else {
		for (j=1; j<=num_menus; j++) {
		    if (menutab[j].pane == xe.xbutton.window) {
			switch (xe.xbutton.button) {
			case 1:
			   *s=menutab[j].text;
			   break;
			case 2:	
			   break;
			case 3:
			   /* RIGHT button returns EOC */
			   sprintf(pickbuf," ;\n");
			   *s = pickbuf;
			   break;
			default:
			   /* printf("b: unexpected button event: %d",
				xe.xbutton.button); */
			   break;
			}
		    }
		}
	    }
	    break;
	case KeyPress:
	    if (debug) printf("EVENT LOOP: got KeyPress\n");
	    charcount = XLookupString((XKeyEvent * ) &xe, keybuf, 
			    keybufsize, &keysym, compose);

	    if (charcount >= 1) {
		*s = keybuf;
	    } else {
		if (compose == NULL) {
		    tmp[0]=(char) ((char)keysym - 64);
		    tmp[1]='\0';
		    *s = tmp;
	        }
	    }
	    
	    break;
	case MapNotify:
	case UnmapNotify:
	case ReparentNotify:
	    break;
	default:
	    eprintf("got unexpected default event: %s",
		event_names[xe.type]);
	    break;
	}
    }
}

void getGC(win, gc, font_info)
Window win;
GC *gc;
XFontStruct *font_info;
{
    unsigned long valuemask = 0;

    XGCValues values;
    unsigned int line_width = 0;
    int line_style = LineSolid;
    int cap_style = CapButt;
    int join_style = JoinMiter;
    int dash_offset = 0;
    static char dash_list[] = {1,2};
    int list_length = 2;

    /* Create default Graphics Context */
    *gc = XCreateGC(dpy, win, valuemask, &values);

    /* Specify font */
    XSetFont(dpy, *gc, font_info->fid);

    /* Specify black foreground since :efault window background
    is white and default foreground is undefined */

    XSetBackground(dpy, *gc, BlackPixel(dpy, scr));
    XSetForeground(dpy, *gc, WhitePixel(dpy, scr));

    /* set line attributes */
    XSetLineAttributes(dpy, *gc, line_width, line_style,
        cap_style, join_style);

    /* set dashes */
    XSetDashes(dpy, *gc, dash_offset, dash_list, list_length); 
}

void load_font(font_info)
XFontStruct **font_info;
{
    /* char *fontname = "9x15"; */
    /* char *fontname = "12x24"; */
    char *fontname = "10x20"; 

    /* Load font and get font information structure */
    if ((*font_info = XLoadQueryFont(dpy, fontname)) == NULL) {
        eprintf("can't open %s font.", fontname);
    }
}

/* this routine is used for doing the rubber band drawing during interactive */
/* point selection.  It is identical to xwin_draw_line() but uses an xor style */

/* used for grid, color changes with GRID :C<grid_color> */
void xwin_xor_line(x1, y1, x2, y2)	
int x1,y1,x2,y2;
{
    if (xwin_display_state() == D_ON) {
	XDrawLine(dpy, win, gcg, x1, y1, x2, y2);
    }
}

/* always use white for XOR, otherwise goes black on some color */
void xwin_rubber_line(x1, y1, x2, y2)	
int x1,y1,x2,y2;
{
    if (xwin_display_state() == D_ON) {
	XDrawLine(dpy, win, gcx, x1, y1, x2, y2);
    }
}


void xwin_draw_line(x1, y1, x2, y2)
int x1,y1,x2,y2;
{
    /* XSetFunction(dpy, gca, GXor); */
    if (xwin_display_state() == D_ON) {
	XDrawLine(dpy, win, gca, x1, y1, x2, y2);
    }
}

void xwin_fill_poly(poly, n) /* call XFillPolygon with saved points */
XPoint *poly;
int n;
{
    if (xwin_display_state() == D_ON) {
	XFillPolygon(dpy, win, gcb, poly, n, Complex, CoordModeOrigin);
    }
}

void xwin_set_pen_line_fill(int pen, int line, int fill) 
{
    int dash_n;
    int dash_offset;
    int line_style;
    char dash_list[5];

    /* FIXME: should avoid accessing out of bounds of colors[]
     * also should cache different pen colors to avoid having to keep
     * sending messages to server.  Currently this is enforced because
     * only db::set_pen() calls here, and uses pen%5 as the argument 
     */

    /* optimize out unnecessary Xserver calls */
    static int oldpen=(-9999);
    if (pen == oldpen) return;

    if (pen > MAX_COLORS-1) {
	pen = MAX_COLORS-1;
    } else if (pen < 0) {
	pen = 0;
    }

    XSetForeground(dpy, gca, colors[pen]);	/* for lines */
    XSetForeground(dpy, gcb, colors[pen]);	/* for polygon fill */

    /* FIXME:should cache different pen colors to avoid having to keep
     * sending messages to server.  
     */

    /* optimize out unnecessary Xserver calls */
    static int oldlinetype=(-9999);
    if (line == oldlinetype) return;

    line = abs(line)%MAX_LINETYPE;

    /* there are two switches here because even if you XSetDashes(),
     * the drawn line will still be solid *unless* you also change
     * the line style to LineOnOffDash or LineDoubleDash.  So we set
     * line zero to be LineSolid and all the rest to LineOnOffDash
     * and then the XSetDashes will take effect.  (45 minutes to debug!)
     * - with much appreciation to Ken Poulton for an example of this in 
     * autoplot(1) code.
     */

    switch (line) {
       case 0:     line_style = LineSolid;     break;	/* solid */
       case 1:     line_style = LineOnOffDash; break;	/* dotted */
       case 2:     line_style = LineOnOffDash; break;   /* broken */
       case 3:     line_style = LineOnOffDash; break;   /* dot center */
       case 4:     line_style = LineOnOffDash; break;   /* dash center */
       case 5:     line_style = LineOnOffDash; break;   /* long dash */
       case 6:     line_style = LineOnOffDash; break;   /* long dotted */
       default:
	   eprintf("line type %d out of range.", line);
    }        
    
    switch (line) {
       case 0:				
       case 1:     dash_list[0]=2; dash_list[1]=2; dash_n=2; break;

       case 2:     dash_list[0]=7; dash_list[1]=5; dash_n=2; break;

       case 3:     dash_list[0]=7; dash_list[1]=2;
                   dash_list[2]=1; dash_list[3]=2; dash_n=4; break;

       case 4:     dash_list[0]=7; dash_list[1]=2;
                   dash_list[2]=3; dash_list[3]=2; dash_n=4; break;

       case 5:     dash_list[0]=9; dash_list[1]=5; dash_n=2; break;

       case 6:     dash_list[0]=4; dash_list[1]=4; dash_n=2; break;
    }

    dash_offset=0;
    XSetLineAttributes(dpy, gca, 0, line_style, CapButt, JoinRound); 
    XSetDashes(dpy, gca, dash_offset, dash_list, dash_n);

    /* optimize out unnecessary Xserver calls */
    static int oldfill=(-9999);
    if (fill == oldfill) return;

    if (fill <= 1) {
        XSetFillStyle(dpy, gcb, FillSolid);
    } else {
        XSetFillStyle(dpy, gcb, FillStippled);
	XSetStipple(dpy, gcb, stipple[(fill-2)*10+pen]);	/* use gcb for polygon filling */
    }
}

int draw_grid(win, gc, dx, dy, sx, sy, xorig, yorig)
Window win;
GC gc;
double dx,dy;		/* grid delta */
double sx,sy;		/* grid skip number */
double xorig,yorig;	/* grid origin */
{
    extern unsigned int g_width, g_height;
    extern GRIDSTATE grid_state;
    extern int grid_color;
    double delta;
    double x,y, xd,yd;
    int i,j;
    int debug=0;
    extern int grid_notified;

    double xstart, ystart, xend, yend;

    if (debug) printf("draw_grid called with: %f %f %f %f %f %f, color %d\n", 
    	dx, dy, sx, sy, xorig, yorig, grid_color);

    /* colored grid */
    if (currep != NULL) {
	XSetForeground(dpy, gc, colors[currep->grid_color]);
    } else {
	XSetForeground(dpy, gc, colors[grid_color]); 
    }

    XSetFillStyle(dpy, gc, FillSolid);

    xstart=(double) 0;
    ystart=(double) g_height;
	if (debug) printf("starting with %f %f\n", xstart, ystart); 
    V_to_R(&xstart, &ystart);
	if (debug) printf("V_to_R %f %f\n", xstart, ystart); 

    snapxy_major(&xstart,&ystart);
	if (debug) printf("snap %f %f\n", xstart, ystart); 

    xend=(double) g_width;
    yend=(double) 0;
	if (debug) printf("starting with %f %f\n", xend, yend); 
    V_to_R(&xend, &yend);
	if (debug) printf("V_to_R %f %f\n", xend, yend); 
    snapxy_major(&xend,&yend);
	if (debug) printf("snap %f %f\n", xend, yend); 

    if ( sx <= 0.0 || sy <= 0.0) {
	printf("grid x,y step must be a positive integer\n");
	return(1);
    }


    if (dx > 0.0 && dy >0.0 && sx > 0.0 && sy > 0.0) {

	/* require at least 5 pixels between grid ticks */

	if ( ((xend-xstart)/(dx*sx) >= (double) g_width/5) ||
	     ((yend-ystart)/(dy*sx) >= (double) g_height/5) ) {

	    if (!grid_notified) {
		printf("grid suppressed, too many points\n");
		grid_notified++;
	    }

	} else if ( ((xend-xstart)/(dx) >= (double) g_width/5) ||
	     ((yend-ystart)/(dy) >= (double) g_height/5) ) {

	    /* this is the old style grid - no fine ticks */

	    for (x=xstart-dx*sx; x<=xend; x+=dx*sx) {
	       for (y=ystart-dy*sy; y<=yend; y+=dy*sy) {
		    xd=x;
		    yd=y;
		    R_to_V(&xd,&yd);
		    if (grid_state == G_ON && xwin_display_state() == D_ON) {
			XDrawPoint(dpy, win, gc, (int)xd, (int)yd);
		    }
		}
	    }

	} else { 

	    /* draw grid with fine intermediate ticks */

	    /* subtract dx*sx, dy*sy from starting coords to make sure */
	    /* that we draw grid all the way to left margin even after */
	    /* calling snap_major above */

	    if (grid_state == G_ON && xwin_display_state() == D_ON) {
		for (x=xstart-dx*sx; x<=xend; x+=dx*sx) {
		    for (y=ystart-dy*sy; y<=yend; y+=dy*sy) {
			for (i=0; i<sx; i++) {
			    for (j=0; j<sy; j++) {
				if (i==0 || j==0) {
				    xd=x + (double) i*dx;
				    yd=y + (double) j*dy;
				    R_to_V(&xd,&yd);
				    if (xwin_display_state() == D_ON) {
				        XDrawPoint(dpy, win, gc, (int)xd, (int)yd);
				    }
				}
			    }
			}
		    }
		}
	    }
	}

	/* now draw crosshair at 1/50 of largest display dimension */

	if (g_width > g_height) {
	    delta = (double) dpy_width/100;
	} else {
	    delta = (double) dpy_height/100;
	}

	/* conveniently reusing these variable names */
	x=(double)0; y=0; R_to_V(&x,&y);

	if (grid_state == G_ON && xwin_display_state() == D_ON) {
	    XDrawLine(dpy, win, gc, 
		(int)x, (int)(y-delta), (int)x, (int)(y+delta));
	    XDrawLine(dpy, win, gc, 
		(int)(x-delta), (int)y, (int)(x+delta), (int)(y)); 
	    XFlush(dpy);
	}

    } else {
	printf("grid arguments out of range\n");
    }
    return(0);
}

void xwin_draw_circle(x,y) 
double x, y;
{
    double delta;
    extern unsigned int g_width, g_height;

    if (xwin_display_state() == D_ON) {

	if (g_width > g_height) {
	    delta = (double) dpy_width/300;
	} else {
	    delta = (double) dpy_height/300;
	}

	R_to_V(&x,&y);

	XDrawLine(dpy, win, gcx, 
	    (int)x+delta, (int)(y), (int)(x+delta*0.7), (int)(y+delta*0.7));
	XDrawLine(dpy, win, gcx, 
	    (int)(x+delta*0.7), (int)(y+delta*0.7), 
	    (int)x, (int)(y+delta));
	XDrawLine(dpy, win, gcx, 
	    (int)x-delta, (int)(y), (int)(x-delta*0.7), (int)(y+delta*0.7));
	XDrawLine(dpy, win, gcx, 
	    (int)(x-delta*0.7), (int)(y+delta*0.7), (int)x,
	    (int)(y+delta));
	XDrawLine(dpy, win, gcx, 
	    (int)x-delta, (int)(y), (int)(x-delta*0.7), (int)(y-delta*0.7));
	XDrawLine(dpy, win, gcx, 
	    (int)(x-delta*0.7), (int)(y-delta*0.7), (int)x,
	    (int)(y-delta));
	XDrawLine(dpy, win, gcx, 
	    (int)x+delta, (int)(y), (int)(x+delta*0.7), (int)(y-delta*0.7));
	XDrawLine(dpy, win, gcx, 
	    (int)(x+delta*0.7), (int)(y-delta*0.7), (int)x,
	    (int)(y-delta));

	XFlush(dpy);

    }
}

void xwin_draw_text(x,y,s) 
double x, y;
char *s;
{
    R_to_V(&x,&y);

    XDrawString(dpy,win,gcx, x, y, s, strlen(s));
    XFlush(dpy);
}

#define RES 6

void xwin_draw_point(x,y) 
double x, y;
{
    double delta, xx, yy;
    extern unsigned int g_width, g_height;
    char buf[BUF_SIZE];

    xx = x-fmod(x,1.0/pow(10.0,RES));
    yy = y-fmod(y,1.0/pow(10.0,RES));

    sprintf(buf,"%g,%g", xx, yy);

    if (g_width > g_height) {
	delta = (double) dpy_width/100;
    } else {
	delta = (double) dpy_height/100;
    }

    R_to_V(&x,&y);

    XDrawLine(dpy, win, gcx, 
	    (int)x, (int)(y-delta), (int)x, (int)(y+delta));
    XDrawLine(dpy, win, gcx, 
	    (int)(x-delta), (int)y, (int)(x+delta), (int)(y)); 

    XDrawString(dpy,win,gcx, x+10, y-10, buf, strlen(buf));

    XFlush(dpy);
}

void xwin_draw_origin(x,y) 
double x, y;
{
    double delta, xx, yy;
    extern unsigned int g_width, g_height;

    xx = x-fmod(x,1.0/pow(10.0,RES));
    yy = y-fmod(y,1.0/pow(10.0,RES));

    if (g_width > g_height) {
	delta = (double) dpy_width/100;
    } else {
	delta = (double) dpy_height/100;
    }

    R_to_V(&x,&y);

    XDrawLine(dpy, win, gcx, 
	    (int)x, (int)(y-delta), (int)x, (int)(y+delta));
    XDrawLine(dpy, win, gcx, 
	    (int)(x-delta), (int)y, (int)(x+delta), (int)(y)); 

    XFlush(dpy);
}

void snapxy(x,y)	/* snap world coordinates to grid ticks */
double *x, *y;
{
    extern double grid_xo, grid_xd;
    extern double grid_yo, grid_yd;
    double xo, yo, xd, yd;
    double xx, yy;
    int debug=0;

    if (currep != NULL) {
    	xo = currep->grid_xo;
    	yo = currep->grid_yo;
    	xd = currep->grid_xd;
    	yd = currep->grid_yd;
    } else {
    	xo = grid_xo;
    	yo = grid_yo;
    	xd = grid_xd;
    	yd = grid_yd;
    }

    if (debug) printf("snap called with %f %f, and xo %f %f xd %f %f\n", 
    	*x, *y,xo, yo, xd, yd);


    /* adapted from Graphic Gems, v1 p630 (round to nearest int fxn) */

    xx = (double) ((*x)>0 ? 
	xd*( (int) ((((*x)-xo)/xd)+0.5))+xo :
	xd*(-(int) (0.5-(((*x)-xo)/xd)))+xo);
    yy = (double) ((*y)>0 ? 
	yd*( (int) ((((*y)-yo)/yd)+0.5))+yo : 
	yd*(-(int) (0.5-(((*y)-yo)/yd)))+yo);

    *x = xx-fmod(xx,1.0/pow(10.0,RES));
    *y = yy-fmod(yy,1.0/pow(10.0,RES));

}

void snapxy_major(x,y)	/* snap to grid ticks multiplied by ds,dy */
double *x, *y;
{
    extern double grid_xo, grid_xd, grid_xs;
    extern double grid_yo, grid_yd, grid_ys;
    double xo, yo, xd, yd, xs, ys;
    double xdel, ydel;

    if (currep != NULL) {
    	xo = currep->grid_xo;
    	yo = currep->grid_yo;
    	xd = currep->grid_xd;
    	yd = currep->grid_yd;
    	xs = currep->grid_xs;
    	ys = currep->grid_ys;
    } else {
    	xo = grid_xo;
    	yo = grid_yo;
    	xd = grid_xd;
    	yd = grid_yd;
    	xs = grid_xs;
    	ys = grid_ys;
    }


    xdel = xd*xs;
    ydel = yd*ys;

    /* adapted from Graphic Gems, v1 p630 (round to nearest int fxn) */

    *x = (double) ((*x)>0 ? 
	xdel*( (int) ((((*x)-xo)/xdel)+0.5))+xo :
	xdel*(-(int) (0.5-(((*x)-xo)/xdel)))+xo);
    *y = (double) ((*y)>0 ? 
	ydel*( (int) ((((*y)-yo)/ydel)+0.5))+yo : 
	ydel*(-(int) (0.5-(((*y)-yo)/ydel)))+yo);
}

void xwin_grid_color(color) 
int color;
{
    extern int grid_color;
    int debug=0;

    if (debug) {
    	printf("xwin_grid_color: old %d, new %d\n", grid_color, color);
    }

    if (currep != NULL) {
	if (currep->grid_color != color) {
	    currep->grid_color = color;
	    need_redraw++;
	}
    } else {
	if (grid_color != color) {
	    grid_color = color;
	    need_redraw++;
	}
    }

}

int xwin_display_state()
{
    extern DISPLAYSTATE display_state;

    if (currep != NULL) {
    	return (currep->display_state);
    } else {
    	return (display_state);
    }
}

void xwin_display_set_state(state)
DISPLAYSTATE state;
{
    extern DISPLAYSTATE display_state;
    int debug=0;

    if (currep != NULL) {
    	display_state = currep->display_state;
    }

    switch (state) {
    	case D_TOGGLE:
	    if (debug) printf("toggling display state\n");
	    if (display_state==D_OFF) {
	    	display_state=D_ON;
		need_redraw++;
	    } else {
	    	display_state=D_OFF;
		need_redraw++;
	    }
	    break;
	case D_ON:
	    if (debug) printf("display on\n");
	    if (state != display_state) {
		display_state=D_ON;
		need_redraw++;
	    }
	    break;
	case D_OFF:
	    if (debug) printf("display off\n");
	    if (state != display_state) {
		display_state=D_OFF;
		need_redraw++;
	    }
	    break;
	default:
	    weprintf("bad state in xwin_display_set_state\n");
	    break;
    }
    if (currep != NULL) {
    	currep->display_state = display_state;
    }
}

void xwin_grid_state(state) 
GRIDSTATE state;
{
    extern GRIDSTATE grid_state;
    int debug=0;
 
    /* FIXME: grid state should be kept in currep */
    switch (state) {
    	case G_TOGGLE:
	    if (debug) printf("toggling grid state\n");
	    if (grid_state==G_OFF) {
	    	grid_state=G_ON;
		need_redraw++;
	    } else {
	    	grid_state=G_OFF;
		need_redraw++;
	    }
	    break;
	case G_ON:
	    if (debug) printf("grid on\n");
	    if (state != grid_state) {
		grid_state=G_ON;
		need_redraw++;
	    }
	    break;
	case G_OFF:
	    if (debug) printf("grid off\n");
	    if (state != grid_state) {
		grid_state=G_OFF;
		need_redraw++;
	    }
	    break;
	default:
	    weprintf("bad state in xwin_grid\n");
	    break;
    }
}

void xwin_grid_pts( xd, yd, xs, ys, xo, yo)
double xd;
double yd;
double xs;
double ys;
double xo; 
double yo;
{
    int debug = 0;
    extern double grid_xd;
    extern double grid_yd;
    extern double grid_xs;
    extern double grid_ys;
    extern double grid_xo;
    extern double grid_yo;

    if (currep != NULL) {
    	if (currep->grid_xd != xd ||
	    currep->grid_yd != yd ||
	    currep->grid_xs != xs ||
	    currep->grid_ys != ys ||
	    currep->grid_xo != xo ||
	    currep->grid_yo != yo ) { /* only redraw if something has changed */

	    currep->grid_xd=xd;
	    currep->grid_yd=yd;
	    currep->grid_xs=xs;
	    currep->grid_ys=ys;
	    currep->grid_xo=xo;
	    currep->grid_yo=yo;
	    /* currep->modified++; */
	    need_redraw++;
	}
    } else {
    	if (grid_xd != xd ||
	    grid_yd != yd ||
	    grid_xs != xs ||
	    grid_ys != ys ||
	    grid_xo != xo ||
	    grid_yo != yo ) { /* only redraw if something has changed */

	    grid_xd=xd;
	    grid_yd=yd;
	    grid_xs=xs;
	    grid_ys=ys;
	    grid_xo=xo;
	    grid_yo=yo;
	    /* currep->modified++; */
	    need_redraw++;
	}
    }
   
    if (debug) printf("setting grid: xydelta=%f,%f xyskip=%f,%f xyoffset=%f,%f\n",
	xd, yd, xs, ys, xo, yo);
}

/*
WIN [:X[scale]] [:Z[power]] [:G{ON|OFF}] [:O{ON|OFF}] [:F] [:Nn] [xy1 [xy2]]
    scale is absolute
    power is magnification factor, positive scales up, negative scales down
    xy1 recenters the display on new point
    xy1,xy2 zooms in on selected region
*/

void xwin_window_set(x1,y1,x2,y2) /* change the current window parameters */
double x1,y1,x2,y2;
{
    extern double scale,xoffset,yoffset;
    extern int grid_notified;
    double dratio,wratio;
    double tmp;
    int debug=0;

    grid_notified=0;	/* never yet evaluated the grid visibility */ 

    if (debug) printf("xwin_window_set called with %f %f %f %f\n", x1,y1,x2,y2); 

    if (x2 < x1) {		/* canonicalize the selection rectangle */
	tmp = x2; x2 = x1; x1 = tmp;
    }
    if (y2 < y1) {
	tmp = y2; y2 = y1; y1 = tmp;
    }

    if (currep != NULL) {
	currep->vp_xmin=x1;
	currep->vp_ymin=y1;
	currep->vp_xmax=x2;
	currep->vp_ymax=y2;
	/* currep->modified++; */
    } else {
	vp_xmin=x1;
	vp_ymin=y1;
	vp_xmax=x2;
	vp_ymax=y2;
    }

    dratio = (double) g_height/ (double)g_width;
    wratio = (y2-y1)/(x2-x1);

    if (wratio > dratio) {	/* world is taller,skinnier than display */
	if (currep != NULL) {
	    currep->scale=((double) g_height)/(y2-y1);
	} else {
	    scale=((double) g_height)/(y2-y1);
	}
    } else {			/* world is shorter,fatter than display */
	if (currep != NULL) {
	    currep->scale=((double) g_width)/(x2-x1);
	} else {
	    scale=((double) g_width)/(x2-x1);
	}
    }

    if (currep != NULL) {
	currep->xoffset = (((double) g_width)/2.0) -  ((x1+x2)/2.0)*currep->scale;
	currep->yoffset = (((double) g_height)/2.0) + ((y1+y2)/2.0)*currep->scale;
    } else {
	xoffset = (((double) g_width)/2.0) -  ((x1+x2)/2.0)*scale;
	yoffset = (((double) g_height)/2.0) + ((y1+y2)/2.0)*scale;
    }

    if (debug) printf("screen width = %d, height=%d\n",g_width, g_height);
    if (debug)  printf("scale = %f, xoffset=%f, yoffset=%f\n",
    	scale, xoffset, yoffset);

    need_redraw++; 
}

/* convert viewport coordinates to real-world coords */
void V_to_R(x,y) 
double *x;
double *y;
{
    extern double xoffset, yoffset, scale;
    if (currep != NULL) {
	*x = (*x - currep->xoffset)/currep->scale;
	*y = (currep->yoffset - *y)/currep->scale;
    } else {
	*x = (*x - xoffset)/scale;
	*y = (yoffset - *y)/scale;
    }
}

/* convert real-world coordinates to viewport coords */
void R_to_V(x,y) 
double *x;
double *y;
{
    extern double xoffset, yoffset, scale;
    if (currep != NULL) {
	*x = currep->xoffset + *x * currep->scale;
	*y = currep->yoffset - *y * currep->scale;
    } else {
	*x = xoffset + *x * scale;
	*y = yoffset - *y * scale;
    }
}

static char *visual_class[] = {
    "StaticGray",
    "GrayScale",
    "StaticColor",
    "PseudoColor",
    "TrueColor",
    "DirectColor"
};

int init_colors() 
{
    int default_depth;
    Visual *default_visual;
    static char *name[] = {
	"white",        /* FIXME: eventually should be black */
	"red",
	"green",
	"#7272ff",	/* brighten up our blues a la Ken.P. :-)*/
	"cyan",
	"magenta",
	"yellow",
	"white",
	"#b0b0b0",
	"#505050"
    };

    XColor exact_def;
    Colormap default_cmap;
    int ncolors = 0;
    int i;
    XVisualInfo visual_info;
    int debug=0;

    /* try to allocate colors for PseudoColor, TrueColor,
     * DirectColor, and StaticColor; use black and white
     * for StaticGray and GrayScale */

    default_depth = DefaultDepth(dpy, scr);
    default_visual = DefaultVisual(dpy, scr);
    default_cmap = DefaultColormap(dpy, scr);
    if (default_depth == 1) {
	/* Must be StaticGray, use black and white */
	colors[0] = BlackPixel(dpy, scr);
	for (i=1; i<MAX_COLORS; i++) {
	    colors[i] = WhitePixel(dpy, scr);
	};
	return(0);
    }

    i=5;
    while (!XMatchVisualInfo(dpy, scr, default_depth,
	/* visual class */ i--, &visual_info)) 
	;
    if (debug) weprintf("found a %s class visual at default depth.", visual_class[++i]);

    if ( i < StaticColor ) {	/* Color visual classes are 2 to 5 */
	/* No color visual available at default depth;
	 * some applications might call XMatchVisualInfo
	 * here to try for a GrayScale visual if they can
	 * use gray to advantage, before giving up and
	 * using black and white */

	colors[0] = BlackPixel(dpy, scr);
	for (i=1; i<MAX_COLORS; i++) {
	    colors[i] = WhitePixel(dpy, scr);
	};

	return(0);
    }

    /* Otherwise, got a color visual at default depth */

    /* The visual we found is not necessarily the default
     * visual, and therefore it is not necessarily the one we used to
     * creat our window; however, we now know for sure that color is
     * supported, so the following code will work (or fail in a
     * controlled way) */

     for (i=0; i < MAX_COLORS; i++) {
	/* printf("allocating %s\n", name[i]); */
	if (!XParseColor(dpy, default_cmap, name[i], &exact_def)) {
	    eprintf("color name %s not in database", name[i]);
	}
	/* printf("The RGB values from the database are %d, %d, %d\n",
	    exact_def.red, exact_def.green, exact_def.blue); */
	if (!XAllocColor(dpy, default_cmap, &exact_def)) {
	    eprintf("no matching colors cells can be allocated");
	    exit(0);
	}
	/* printf("The RGB values actually allocated are %d, %d, %d\n",
	    exact_def.red, exact_def.green, exact_def.blue); */
	colors[i] = exact_def.pixel;
	ncolors++;
    }
    /* printf("%s: allocated %d read-only color cells\n", progname, ncolors); */
    return(1);
}


/* following routine originally from Xlib Programming Manual Vol I., */
/* O'Reilly & associates, page 533.  */

void paint_pane(window, menutab, ngc, rgc, mode) 
Window window;
MENUENTRY menutab[];
GC ngc, rgc;
int mode;
{
    int win;
    int x = 2, y;
    GC gc;

    if (mode == BLACK) {
    	XSetWindowBackground(dpy, window, 
	    BlackPixel(dpy, scr));
	gc = ngc;
    } else {
    	XSetWindowBackground(dpy, window, 
	    WhitePixel(dpy, scr));
	gc = rgc;
    }

    /* Clearing repaints the background */
    XClearWindow(dpy, window);

    /* Find out winndex of window for label text */
    for (win=0; window != menutab[win].pane; win++) {
    	;
    }

    y = font_info->max_bounds.ascent;

    /* The string length is necessary because strings 
     * for XDrawString may not be null terminated */
    XSetForeground(dpy, gc, colors[menutab[win].color]);
    XDrawString(dpy, window, gc, x, y, menutab[win].text,
        strlen(menutab[win].text)); 
}

void xwin_raise_window()
{
   XRaiseWindow(dpy, topwin);
   // XMapRaised(dpy, topwin);
   XFlush(dpy);
   XSync(dpy, False);
   XFlush(dpy);
   XSync(dpy, False);
   XFlush(dpy);
   XSync(dpy, False);
}

/* given a shiftmask, return how many bits to shift */
/* it into position... look at most max bits */

int shift_from_mask(int mask, int max) {

    int bit, tmp, shift;

    tmp = mask;
    bit = tmp & 1;
    shift=0;
    while(bit != 1 && shift<max) {
       tmp = tmp>>1;	/* shift down to 1 */
       bit = tmp & 1;
       shift++;
    }
    bit = tmp & 128;
    while(bit != 128 && shift>1) {
       tmp = tmp<<1;	/* shift up to 128 */
       bit = tmp & 128;
       shift--;
    }
    return (shift);
}

//  typical values...
// width  = 612
// height = 576
// byte_order = LSBFirst
// bitmap unit=32
// bitmap_bit_order = LSBFirst
// bitmap pad = 32
// depth = 24
// bytes_per_line = 2448
// bit_per_pixel = 32
// red mask= 16711680
// green mask= 65280
// blue mask= 255


int dump_window(window, gc, width, height) 
Window window;
GC gc;
unsigned int width, height;
{
    XImage *xi;
    int x, y;
    unsigned long pixel;
    char buf[128];
    int R,G,B;
    int fd;
    int i;
    int debug=1;
    int rshift, gshift, bshift;

    XSync(dpy,0);

    if (currep == NULL) {
    	printf("nothing to dump, not editing any cell...\n");
	return(0);
    }

    xi = XGetImage(dpy, window, 0,0, width, height, AllPlanes, ZPixmap);

    if (debug) {

	printf("width  = %d\n", xi->width );
	printf("height = %d\n", xi->height);

	if (xi->byte_order == LSBFirst) {
	    printf("byte_order = LSBFirst\n");
	} else if (xi->byte_order == MSBFirst) {
	    printf("byte_order = MSBFirst\n");
	} else {
	    printf("unknown byte_order: %d\n", xi->byte_order);
	}

	printf("bitmap unit=%d\n", xi->bitmap_unit);

	if (xi->bitmap_bit_order == LSBFirst) {
	    printf("bitmap_bit_order = LSBFirst\n");
	} else if (xi->byte_order == MSBFirst) {
	    printf("bitmap_bit_order = MSBFirst\n");
	} else {
	    printf("unknown bitmap_bit_order: %d\n", xi->bitmap_bit_order);
	}

	printf("bitmap pad = %d\n", xi->bitmap_pad);
	printf("depth = %d\n", xi->depth);
	printf("bytes_per_line = %d\n", xi->bytes_per_line);
	printf("bit_per_pixel = %d\n", xi->bits_per_pixel);
	printf("red mask= %d\n", (int) xi->red_mask);
	printf("green mask= %d\n", (int) xi->green_mask);
	printf("blue mask= %d\n", (int) xi->blue_mask);
    }

    /* compute bit shift values */

    rshift=shift_from_mask(xi->red_mask, 8*xi->bitmap_unit);
    gshift=shift_from_mask(xi->green_mask, 8*xi->bitmap_unit);
    bshift=shift_from_mask(xi->blue_mask, 8*xi->bitmap_unit);
    printf("rs: %d, gs: %d, bs: %d\n",rshift, gshift, bshift);

    sprintf(buf, "%s.ppm", currep->name);

    printf("dumping to %s", buf);
    fflush(stdout);

    fd = open(buf, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO );
    sprintf(buf, "P6\n%d\n%d\n%d\n", width, height, 255);
    write(fd, buf, strlen(buf));

    i=0;
    for (y=0; y<=height; y++) {
	for (x=0; x<width; x++) {
	   if (++i==10000) {
	       i=0;
	       printf(".");
	       fflush(stdout);
	   }
	   pixel = XGetPixel(xi, x, y);	

	   R=pixel & xi->red_mask;
	   R = R>>rshift;
	   buf[0] = (unsigned char) R;

	   G=pixel & xi->green_mask;
	   G = G>>gshift;
	   buf[1] = (unsigned char) G;

	   B=pixel & xi->blue_mask;
	   B = B>>bshift;
	   buf[2] = (unsigned char) B;

	   write(fd, buf, 3);
	}
    }
    printf("\n");
    fflush(stdout);

    close(fd);
    XDestroyImage(xi);
    return(1);
}

void xwin_dump_graphics() {
    dump_window(win, gca, g_width, g_height);
}
