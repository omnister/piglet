#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include "eventnames.h"
#include <errno.h>

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/unistd.h>

#include <readline/readline.h>
#include <readline/history.h>     

#include "db.h"
#include "xwin.h"
#include "eprintf.h"
#include "rubber.h"

/* globals for interacting with db.c */
DB_TAB *currep = NULL;		/* keep track of current rep */
DB_TAB *newrep = NULL;		/* scratch pointer for new rep */
OPTS   *opts;
XFORM  screen_transform;
XFORM  *xp = &screen_transform;

int done; /* When non-zero, this global means the user is done using this program. */

char version[] = "$Header: /home/walker/piglet/piglet/date/foo/RCS/xwin.c,v 1.4 2003/12/28 23:32:02 walker Exp $"; 

unsigned int width, height;		/* window pixel size */
unsigned int dpy_width, dpy_height;	/* disply pixel size */

double vp_xmin=-1000.0;     	/* world coordinates of viewport */ 
double vp_ymin=-1000.0;
double vp_xmax=1000.0;
double vp_ymax=1000.0;	
double scale = 1.0;		/* xform factors from real->viewport */
double xoffset = 0.0;
double yoffset = 0.0;

int grid_xd = 10;	/* grid delta */
int grid_yd = 10;
int grid_xs = 2;	/* how often to display grid ticks */
int grid_ys = 2;
int grid_xo = 0; 	/* starting offset */
int grid_yo = 0;
GRIDSTATE grid_state = ON;	/* ON, OFF */
int grid_color = 1;	/* 1 through 6 for different colors */ 

int modified=0;
int need_redraw=0;

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

/* stipple definitions */

#define STIPW 8
#define STIPH 8 

static char stip_bits1[] = {	/* 8x negative slope diagonal */
    0x80, 0x40, 0x20, 0x10,
    0x08, 0x04, 0x02, 0x01
};

static char stip_bits[] = {	/* 8x diagonal crosshatch */
    0x82, 0x44, 0x28, 0x10,
    0x28, 0x44, 0x82, 0x01
};

/* for select in XEvent loop */
#ifndef FD_SET
#define FD_SET(n, p)    ((p)->fds_bits[0] |= (1 << ((n) % 32)))
#define FD_CLR(n, p)    ((p)->fds_bits[0] &= ~(1 << ((n) % 32)))
#define FD_ISSET(n, p)  ((p)->fds_bits[0] & (1 << ((n) % 32)))
#define FD_ZERO(p)      bzero((char *)(p), sizeof(*(p)))
#endif /* !FD_SET */

extern char *getwd();
extern char *xmalloc();

void V_to_R();
void R_to_V(); 
void snapxy();
void snapxy_major();


Display *dpy; int scr;

extern int errno;

#define BUF_SIZE 128

Window win;
GC gca, gcb;
GC gcx;

#define MAX_COLORS 8
unsigned long colors[MAX_COLORS];    /* will hold pixel values for colors */
#define MAX_LINETYPE 5

int initX()
{
    double x,y;
    double xu,yu;
    unsigned int border_width = 4;
    extern unsigned int dpy_width, dpy_height;
    unsigned int icon_width, icon_height;
    char *window_name = "HOE: Hierarchical Object Editor";
    char *icon_name = "rigel";
    char buf[BUF_SIZE];
    Pixmap icon_pixmap;
    Pixmap stipple;
    XSizeHints *size_hints;
    XIconSize *size_list;
    XWMHints *wm_hints;
    XClassHint *class_hints;
    XTextProperty windowName, iconName;
    XSetWindowAttributes attr;
    int icount;
    XFontStruct *font_info;
    char *dpy_name = NULL;
    int window_size = 0;    /* either BIG_ENOUGH or
                    TOO_SMALL to display contents */

    int i;
    int c;
    char *s;

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
    width = dpy_width/2, height = dpy_height/2;

    /* Create opaque window */
    win = XCreateSimpleWindow(dpy, RootWindow(dpy,scr),
        (int) x,(int) y,width, height, border_width, WhitePixel(dpy,scr),
        BlackPixel(dpy,scr));

    /* Get available icon sizes from window manager */

    if (XGetIconSizes(dpy, RootWindow(dpy, scr), &size_list, &icount) == 0)
        weprintf("Window manager didn't set icon sizes - using default.");
    else {
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

    XSetWMProperties(dpy, win, &windowName, &iconName,
        (char **) NULL, (int) NULL, size_hints, wm_hints, class_hints);

        /* argv, argc, size_hints, wm_hints, class_hints); */

    /* Select event types wanted */
    XSelectInput(dpy, win, ExposureMask | KeyPressMask |
        ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | 
	Button1MotionMask | PointerMotionMask );

    load_font(&font_info);

    init_colors();

    /* Create GC for text and drawing */
    getGC(win, &gca, font_info);
    getGC(win, &gcb, font_info);
    getGC(win, &gcx, font_info);

    stipple = XCreateBitmapFromData(dpy, win, stip_bits, 
	(unsigned int) STIPW, (unsigned int) STIPH);
    XSetStipple(dpy, gcx, stipple);
    XSetFillStyle(dpy, gcx, FillStippled);
    XSetFunction(dpy, gcx, GXxor);

    /* dpy window */
    XMapWindow(dpy, win);

    /* turn on backing store */
    attr.backing_store = Always;
    XChangeWindowAttributes(dpy,win,CWBackingStore,&attr);   

    /* initialize various things, should be put into init() function */

    xwin_grid_pts(10.0, 10.0, 2.0, 2.0, 0.0, 0.0);
    xwin_grid_color(1);
    xwin_grid_state(ON);
    xwin_window(4, -100.0,-100.0, 100.0, 100.0);
    sprintf(buf,"");

    return(0);
}

int procXevent(fp)
FILE *fp;
{

    static int button_down=0;
    XEvent xe;
    static int i = 0;

    static char buf[BUF_SIZE];
    static char *s = NULL;
    static char *t = NULL;
    static double x,y;
    static double xu,yu;
    static double xold,yold;
    extern double vp_xmax, vp_xmin, vp_ymax, vp_ymin;

    static int charcount;
    static char keybuf[10];
    static int keybufsize=10;
    KeySym keysym;
    XComposeStatus compose;

    /* readline select stuff */
    int nf, nfds, cn, in; 
    struct timeval *timer = (struct timeval *) 0;
    fd_set rset, tset;
    unsigned long all = 0xffffffff;
    static int dbug=0;

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
		db_render(currep,xp,0,0); /* render the current rep */
	    }
	    draw_grid(win, gcx, grid_xd, grid_yd,
		grid_xs, grid_ys, grid_xo, grid_yo);
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
	nf > 0 && XFlush(dpy);
	if (FD_ISSET(cn, &tset)) { 		/* pending X Event */
	    while (XCheckMaskEvent(dpy, all, &xe)) {
		switch (xe.type) {
		case MotionNotify:
		    debug("got Motion",dbug);
		    /* if(button_down) { */
		    if(1) {

			x = (double) xe.xmotion.x;
			y = (double) xe.xmotion.y;
			V_to_R(&x,&y);
			snapxy(&x,&y);
			sprintf(buf,"(%5d,%5d)         ", (int) x, (int) y);
			XDrawImageString(dpy,win,gcx,20, 20, buf, strlen(buf));

			if (xold != x || yold != y) {
			    rubber_draw(x, y);
			    xold=x;
			    yold=y;
			} 

		    } else {		/* must be button up */

			xu = (double) xe.xmotion.x;
			yu = (double) xe.xmotion.y;
			V_to_R(&xu,&yu);
			snapxy(&xu,&yu);
			sprintf(buf,"(%5d,%5d)         ", (int) x, (int) y);
			XDrawImageString(dpy,win,gcx,20, 20, buf, strlen(buf));
			
			rubber_draw(xu, yu);
		    }
		    break;
		case Expose:
		    debug("got Expose",dbug);

		    xwin_window(4, vp_xmin, vp_ymin, vp_xmax, vp_ymax);

		    if (xe.xexpose.count != 0)
			break;
		    if (xe.xexpose.window == win) {
			draw_grid(win, gcx, grid_xd, grid_yd,
			    grid_xs, grid_ys, grid_xo, grid_yo);
		    } 
		    break;

		case ConfigureNotify:
		    debug("got Configure Notify",dbug);
		    width = xe.xconfigure.width;
		    height = xe.xconfigure.height;
		    xwin_window(4, vp_xmin, vp_ymin, vp_xmax, vp_ymax);
		    break;
		case ButtonRelease:
		    button_down=0;
		    /* x = (double) xe.xmotion.x;
		     * y = (double) xe.xmotion.y;
		     * V_to_R(&x,&y);
		     * snapxy(&x,&y);
		     */
		    debug("got ButtonRelease",dbug);
		    break;
		case ButtonPress:
		    switch (xe.xbutton.button) {
			case 1:	/* left button */
			    button_down=1;
			    x = (double) xe.xmotion.x;
			    y = (double) xe.xmotion.y;
			    V_to_R(&x,&y);
			    snapxy(&x,&y);

			    /* returning mouse loc as a string */
			    sprintf(buf," %d, %d\n", (int) x, (int) y);
			    s = buf;

			    debug("got ButtonPress",dbug);
			    break;
			case 2:	/* middle button */
			    break;
			case 3: /* right button */
			    /* RIGHT button returns EOC */
			    sprintf(buf," ;\n");
			    s = buf;
			    break;
			default:
			    eprintf("unexpected button event: %d",
				xe.xbutton.button);
			    break;
		    }
		    break;
		case KeyPress:
		    debug("got KeyPress",dbug);
	            charcount = XLookupString((XKeyEvent * ) &xe, keybuf, 
		                    keybufsize, &keysym, &compose);

		    if (charcount >= 1) {
			s = keybuf;
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
	} else if (FD_ISSET(in, &tset)) { 		/* pending stdin */
	    return(getc(stdin));
	}
    }
}

getGC(win, gc, font_info)
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

    /* Specify black foreground since default window background
    is white and default foreground is undefined */

    XSetBackground(dpy, *gc, BlackPixel(dpy, scr));
    XSetForeground(dpy, *gc, WhitePixel(dpy, scr));

    /* set line attributes */
    XSetLineAttributes(dpy, *gc, line_width, line_style,
        cap_style, join_style);

    /* set dashes */
    XSetDashes(dpy, *gc, dash_offset, dash_list, list_length); 
}

load_font(font_info)
XFontStruct **font_info;
{
    char *fontname = "9x15";

    /* Load font and get font information structure */
    if ((*font_info = XLoadQueryFont(dpy, fontname)) == NULL) {
        eprintf("can't open %s font.", fontname);
    }
}

/* this routine is used for doing the rubber band drawing during interactive */
/* point selection.  It is identical to xwin_draw_line() but uses an xor style */

void xwin_xor_line(x1, y1, x2, y2)
int x1,y1,x2,y2;
{
	XDrawLine(dpy, win, gcx, x1, y1, x2, y2);
}

void xwin_draw_line(x1, y1, x2, y2)
int x1,y1,x2,y2;
{
    /* XSetFunction(dpy, gca, GXcopy);  */
    XDrawLine(dpy, win, gca, x1, y1, x2, y2);
}

void xwin_set_pen(pen)
int pen;
{
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

    XSetForeground(dpy, gca, colors[pen]);
}

void xwin_set_line(line)
int line;
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
    static int oldlinetype=(-9999);
    if (line == oldlinetype) return;

    if (line > MAX_LINETYPE) {
	line = MAX_LINETYPE;
    } else if (line < 1) {
	line = 1;
    }

    /* there are two switches here because even if you XSetDashes(),
     * the drawn line will still be solid *unless* you also change
     * the line style to LineOnOffDash or LineDoubleDash.  So we set
     * line one to be LineSolid and all the rest to LineOnOffDash
     * and then the XSetDashes will take effect.  (45 minutes to debug!)
     * - with much appreciation to Ken P. for an example of this in 
     * autoplot(1) code.
     */

    switch (line) {
       case 1:     line_style = LineSolid; break;
       case 2:
       case 3:
       case 4:
       case 5:     line_style = LineOnOffDash; break;
       default:
	   eprintf("line type %d out of range.", line);
    }        
    
    switch (line) {
       case 1:
       case 2:     dash_list[0]=7; dash_list[1]=5; dash_n=2; break;
       case 3:     dash_list[0]=2; dash_list[1]=2; dash_n=2; break;
       case 4:     dash_list[0]=7; dash_list[1]=2;
                   dash_list[2]=3; dash_list[3]=2; dash_n=4; break;
       case 5:     dash_list[0]=7; dash_list[1]=2;
                   dash_list[2]=1; dash_list[3]=2; dash_n=4; break;
    }

    dash_offset=0;
    XSetLineAttributes(dpy, gca, 0, line_style, CapButt, JoinRound); 
    XSetDashes(dpy, gca, dash_offset, dash_list, dash_n);
}    


draw_grid(win, gc, dx, dy, sx, sy, xorig, yorig)
Window win;
GC gc;
int dx,dy;		/* grid delta */
int sx,sy;		/* grid skip number */
int xorig,yorig;	/* grid origin */
{
    extern unsigned int width, height;
    extern GRIDSTATE grid_state;
    extern int grid_color;
    double delta;
    double x,y, xd,yd;
    int i,j;
    int debug=0;

    double xstart, ystart, xend, yend;

    if (debug) printf("draw_grid called with: %d %d %d %d %d %d, color %d\n", 
    	dx, dy, sx, sy, xorig, yorig, grid_color);

    /* colored grid */
    XSetForeground(dpy, gc, colors[grid_color]); /* RCW */

    XSetFillStyle(dpy, gc, FillSolid);

    xstart=(double) 0;
    ystart=(double) height;
	/* printf("starting with %g %g\n", xstart, ystart); */
    V_to_R(&xstart, &ystart);
	/* printf("V_to_R %g %g\n", xstart, ystart); */

    snapxy_major(&xstart,&ystart);
	/* printf("snap %g %g\n", xstart, ystart); */

    xend=(double) width;
    yend=(double) 0;
	/* printf("starting with %g %g\n", xend, yend); */
    V_to_R(&xend, &yend);
	/* printf("V_to_R %g %g\n", xend, yend); */
    snapxy_major(&xend,&yend);
	/* printf("snap %g %g\n", xend, yend); */

    if ( sx == 0 || sy == 0) {
	printf("grid x,y step must be a positive integer\n");
	return(1);
    }


    if (dx > 0 && dy >0 && sx > 0 && sy > 0) {

	/* require at least 5 pixels between grid ticks */

	if ( ((xend-xstart)/(double)(dx) >= (double) width/5) ||
	     ((yend-ystart)/(double)(dy) >= (double) height/5) ) {

	    /* suppress grid */
	    printf("grid suppressed, too many points\n");

	} else { 

	    /* draw grid */

	    /* subtract dx*sx, dy*sy from starting coords to make sure */
	    /* that we draw grid all the way to left margin even after */
	    /* calling snap_major above */

	    if (grid_state == ON) {
		for (x=xstart-(double) dx*sx; x<=xend; x+=(double) dx*sx) {
		    for (y=ystart-(double) dy*sy; y<=yend; y+=(double) dy*sy) {
			for (i=0; i<sx; i++) {
			    for (j=0; j<sy; j++) {
				if (i==0 || j==0) {
				    xd=x + (double) i*dx;
				    yd=y + (double) j*dy;
				    R_to_V(&xd,&yd);
				    XDrawPoint(dpy, win, gc, (int)xd, (int)yd);
				}
			    }
			}
		    }
		}
	    }

	    /* this is the old style grid - no fine ticks */

	    /*
	     *	for (x=xstart; x<=xend; x+=(double) dx*sx) {
	     *	   for (y=ystart; y<=yend; y+=(double) dy*sy) {
	     *		xd=x;
	     *		yd=y;
	     *		R_to_V(&xd,&yd);
	     *		XDrawPoint(dpy, win, gc, (int)xd, (int)yd);
	     *	    }
	     *	}
	     */
	}

	/* now draw crosshair at 1/50 of largest display dimension */

	if (width > height) {
	    delta = (double) dpy_width/100;
	} else {
	    delta = (double) dpy_height/100;
	}

	/* conveniently reusing these variable names */
	x=(double)0; y=0; R_to_V(&x,&y);

	XDrawLine(dpy, win, gc, 
	    (int)x, (int)(y-delta), (int)x, (int)(y+delta));
	XDrawLine(dpy, win, gc, 
	    (int)(x-delta), (int)y, (int)(x+delta), (int)(y));

	XFlush(dpy);

    } else {
	printf("grid arguments out of range\n");
    }
    return(0);
}

void snapxy(x,y)	/* snap to grid tick */
double *x, *y;
{
    extern int grid_xo, grid_xd;
    extern int grid_yo, grid_yd;

    /* adapted from Graphic Gems, v1 p630 (round to nearest int fxn) */

    *x = (double) ((*x)>0 ? 
	grid_xd*( (int) ((((*x)-grid_xo)/grid_xd)+0.5))+grid_xo :
	grid_xd*(-(int) (0.5-(((*x)-grid_xo)/grid_xd)))+grid_xo);
    *y = (double) ((*y)>0 ? 
	grid_yd*( (int) ((((*y)-grid_yo)/grid_yd)+0.5))+grid_yo : 
	grid_yd*(-(int) (0.5-(((*y)-grid_yo)/grid_yd)))+grid_yo);
}

void snapxy_major(x,y)	/* snap to grid ticks multiplied by ds,dy */
double *x, *y;
{
    extern int grid_xo, grid_xd, grid_xs;
    extern int grid_yo, grid_yd, grid_ys;

    int xd, yd;

    xd = grid_xd*grid_xs;
    yd = grid_yd*grid_ys;

    /* adapted from Graphic Gems, v1 p630 (round to nearest int fxn) */

    *x = (double) ((*x)>0 ? 
	xd*( (int) ((((*x)-grid_xo)/xd)+0.5))+grid_xo :
	xd*(-(int) (0.5-(((*x)-grid_xo)/xd)))+grid_xo);
    *y = (double) ((*y)>0 ? 
	yd*( (int) ((((*y)-grid_yo)/yd)+0.5))+grid_yo : 
	yd*(-(int) (0.5-(((*y)-grid_yo)/yd)))+grid_yo);
}

debug(s,dbug)
char *s;
int dbug;
{
    if (dbug) {
	weprintf("%s", s);
    }
}


void xwin_grid_color(color) 
int color;
{
    extern int grid_color;
    int debug=0;

    if (debug) {
    	printf("xwin_grid_color: old %d, new %d\n", grid_color, color);
    }

    if (grid_color != color) {
	grid_color = color;
	need_redraw++;
    }
}

void xwin_grid_state(state) 
GRIDSTATE state;
{
    extern GRIDSTATE grid_state;
    int debug=0;

    switch (state) {
    	case TOGGLE:
	    if (debug) printf("toggling grid state\n");
	    if (grid_state==OFF) {
	    	grid_state=ON;
		need_redraw++;
	    } else {
	    	grid_state=OFF;
		need_redraw++;
	    }
	    break;
	case ON:
	    if (debug) printf("grid on\n");
	    if (state != grid_state) {
		grid_state=ON;
		need_redraw++;
	    }
	    break;
	case OFF:
	    if (debug) printf("grid off\n");
	    if (state != grid_state) {
		grid_state=OFF;
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
    extern int grid_xd;
    extern int grid_yd;
    extern int grid_xs;
    extern int grid_ys;
    extern int grid_xo;
    extern int grid_yo;
    int debug = 0;
   
    if (debug) printf("xwin_grid_pts: setting grid, %g %g %g %g %g %g\n",
	xd, yd, xs, ys, xo, yo);

    /* only redraw if something has changed */

    if (grid_xd != (int) (xd) ||
        grid_yd != (int) (yd) ||
	grid_xs != (int) (xs) ||
	grid_ys != (int) (ys) ||
	grid_xo != (int) (xo) ||  
	grid_yo != (int) (yo)) {

	grid_xd=(int) (xd); 
	grid_yd=(int) (yd); 
	grid_xs=(int) (xs); 
	grid_ys=(int) (ys); 
	grid_xo=(int) (xo); 
	grid_yo=(int) (yo); 
	need_redraw++;
    }

}

/*
WIN [:X[scale]] [:Z[power]] [:G{ON|OFF}] [:O{ON|OFF}] [:F] [:Nn] [xy1 [xy2]]
    scale is absolute
    power is magnification factor, positive scales up, negative scales down
    xy1 recenters the display on new point
    xy1,xy2 zooms in on selected region
*/

xwin_window(n, x1,y1,x2,y2) /* change the current window parameters */
int n;
double x1,y1,x2,y2;
{
    extern XFORM *xp;
    extern double vp_xmin, vp_ymin, vp_xmax, vp_ymax;
    extern double scale,xoffset,yoffset;
    double dratio,wratio;
    double tmp;

    if (n==4) {
	/* printf("xwin_window called with %f %f %f %f\n",x1,y1,x2,y2); */

	if (x2 < x1) {		/* canonicalize the selection rectangle */
	    tmp = x2; x2 = x1; x1 = tmp;
	}
	if (y2 < y1) {
	    tmp = y2; y2 = y1; y1 = tmp;
	}

	/* printf("setting user window to %g,%g %g,%g\n",x1,y1,x2,y2); */
	vp_xmin=x1;
	vp_xmax=x2;
	vp_ymin=y1;
	vp_ymax=y2;
    }

    dratio = (double) height/ (double)width;
    wratio = (vp_ymax-vp_ymin)/(vp_xmax-vp_xmin);

    if (wratio > dratio) {	/* world is taller,skinnier than display */
	scale=((double) height)/(vp_ymax-vp_ymin);
    } else {			/* world is shorter,fatter than display */
	scale=((double) width)/(vp_xmax-vp_xmin);
    }

    xoffset = (((double) width)/2.0) -  ((vp_xmin+vp_xmax)/2.0)*scale;
    yoffset = (((double) height)/2.0) + ((vp_ymin+vp_ymax)/2.0)*scale;

    /* printf("screen width = %d\n",width);
    ** printf("screen height = %d\n",height);
    ** printf("scale = %g, xoffset=%g, yoffset=%g\n", scale, xoffset, yoffset);
    ** printf("\n");
    ** printf("dx = xoffset + rx*scale;\n");
    ** printf("dy = yoffset - ry*scale;\n");
    ** printf("rx = (dx - xoffset)/scale;\n");
    ** printf("ry = (yoffset - dy)/scale;\n");
    ** printf("xmin -> %d\n", (int) (xoffset + vp_xmin*scale));
    ** printf("ymin -> %d\n", (int) (yoffset - vp_ymin*scale));
    ** printf("xmax -> %d\n", (int) (xoffset + vp_xmax*scale));
    ** printf("ymax -> %d\n", (int) (yoffset - vp_ymax*scale));
    */
    
    xp->r11 = scale;
    xp->r12 = 0.0;
    xp->r21 = 0.0;
    xp->r22 = -scale;
    xp->dx  = xoffset;
    xp->dy  = yoffset;

    need_redraw++;

    return (0);
}

void V_to_R(x,y) 
double *x;
double *y;
{
    extern double xoffset, yoffset, scale;
    *x = (*x - xoffset)/scale;
    *y = (yoffset - *y)/scale;
}

void R_to_V(x,y) 
double *x;
double *y;
{
    extern double xoffset, yoffset, scale;
    *x = xoffset + *x * scale;
    *y = yoffset - *y * scale;
}

static char *visual_class[] = {
    "StaticGray",
    "GrayScale",
    "StaticColor",
    "PseudoColor",
    "TrueColor",
    "DirectColor"
};

init_colors() 
{
    int default_depth;
    Visual *default_visual;
    static char *name[] = {
	"black",
	"white",
	"red",
	"green",
	"#7272ff",	/* brighten up our blues */
	"cyan",
	"magenta",
	"yellow"
    };

    XColor exact_def;
    Colormap default_cmap;
    int ncolors = 0;
    int i;
    XVisualInfo visual_info;

    /* try to allocate colors for PseudoColor, TrueColor,
     * DirectCOlor, and StaticColor; use balack and white
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
    weprintf("found a %s class visual at default depth.", visual_class[++i]);

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
     * visual, and therefore it is not necessaritly the one
     * we used to creat our window; however, we now know
     * for sure that color is supported, so the following
     * code will work (or fail in a controlled way) */

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
