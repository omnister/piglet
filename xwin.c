#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include "eventnames.h"
#include <errno.h>

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

/* globals for interacting with db.c */
DB_TAB *currep = NULL;		/* keep track of current rep */
XFORM  unity_transform;
XFORM  screen_transform;
XFORM  *xp = &screen_transform;

int quit_now; /* when != 0 ,  means the user is done using this program. */

char version[] = "$Id: xwin.c,v 1.26 2004/11/10 23:24:49 walker Exp $";

unsigned int top_width, top_height;	/* main window pixel size */
unsigned int width, height;		/* graphic window pixel size */
unsigned int menu_width;		/* menu window pixel size */
unsigned int dpy_width, dpy_height;	/* disply pixel size */

double vp_xmin=-1000.0;     	/* world coordinates of viewport */ 
double vp_ymin=-1000.0;
double vp_xmax=1000.0;
double vp_ymax=1000.0;	
double scale = 1.0;		/* xform factors from real->viewport */
double xoffset = 0.0;
double yoffset = 0.0;

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

Display *dpy; int scr;

extern int errno;

#define BUF_SIZE 128

Window topwin;
Window win;
Window menuwin;
GC gca, gcb;
GC gcx;

/* Menu definitions */

#define NONE 100
#define BLACK 1
#define WHITE 0

static char *menu_label[] = {
   "EDI  ",
   "SHO#E;\n",
   "WIN:F",
   "ADD  ",
   "MOV  ",
   "COPY ",
   "DEL  ",
   "DUMP ",
   "PLOT ",
   "DIST ",
   "IDEN ",
   "GRID ",
   "LOCK ",
   "SAVE ",
   "EXIT;"
};
#define MAX_CHOICE 15

Window inverted_pane = NONE;
Window panes[MAX_CHOICE];

XFontStruct *font_info;

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
    char *dpy_name = NULL;
    int window_size = 0;    /* either BIG_ENOUGH or
                    TOO_SMALL to display contents */

    int debug=0;
    int i;
    int c;
    char *s;

    /* menu stuff */
    char *string;
    int char_count;
    int direction, ascent, descent;
    int pane_height;
    int menu_height;
    XCharStruct overall;
    int winindex;


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
    string = menu_label[1];
    char_count = strlen(string);

    load_font(&font_info);

    /* Determine the extent of each menu pane based on font size */
    XTextExtents(font_info, string, char_count, &direction, &ascent,
        &descent, &overall);

    menu_width = overall.width + 4;
    pane_height = overall.ascent + overall.descent + 4;
    menu_height = pane_height * MAX_CHOICE;

    /* create top level window for packing graphic and menu windows */
    topwin = XCreateSimpleWindow(dpy, RootWindow(dpy,scr),
        (int) x,(int) y, top_width, top_height, border_width,
	WhitePixel(dpy,scr), BlackPixel(dpy,scr));

    x=y=0.0;
    border_width=1;
    width=top_width-menu_width;
    height=top_height;

    /* Create opaque window for graphics */
    win = XCreateSimpleWindow(dpy, topwin,
        (int) x,(int) y, width, height, border_width, 
	WhitePixel(dpy,scr), BlackPixel(dpy,scr));

    width=top_width-menu_width;

    /* Create opaque window for menu */
    menuwin = XCreateSimpleWindow(dpy, topwin,
        (width)+1, (int) y, menu_width-2, height,
	border_width, WhitePixel(dpy,scr), BlackPixel(dpy,scr));

    /* Create the menu boxes */
    for (winindex = 0; winindex < MAX_CHOICE; winindex++) {
        panes[winindex] = XCreateSimpleWindow(dpy, menuwin, 0,
	    menu_height/MAX_CHOICE*winindex, menu_width,
	    pane_height, border_width = 1,
	    WhitePixel(dpy,scr),
	    BlackPixel(dpy,scr));
	XSelectInput(dpy, panes[winindex], ButtonPressMask |
	    ButtonReleaseMask | ExposureMask);
    }

    XMapSubwindows(dpy, menuwin);

    /* Get available icon sizes from window manager */

    if (XGetIconSizes(dpy, RootWindow(dpy, scr), &size_list, &icount) == 0)
        if (debug) weprintf("WM didn't set icon sizes - using default.");
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

    XSetWMProperties(dpy, topwin, &windowName, &iconName,
        (char **) NULL, (int) NULL, size_hints, wm_hints, class_hints);

        /* argv, argc, size_hints, wm_hints, class_hints); */

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
    getGC(win, &gcb, font_info);
    getGC(win, &gcx, font_info);

    stipple = XCreateBitmapFromData(dpy, win, stip_bits, 
	(unsigned int) STIPW, (unsigned int) STIPH);
    XSetStipple(dpy, gcx, stipple);
    XSetFillStyle(dpy, gcx, FillStippled);
    XSetFunction(dpy, gcx, GXxor);

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
    sprintf(buf,"");

    /* initialize unitytransform */
    unity_transform.r11 = 1.0;
    unity_transform.r12 = 0.0;
    unity_transform.r21 = 0.0;
    unity_transform.r22 = 1.0;
    unity_transform.dx  = 0.0;
    unity_transform.dy  = 0.0;

    return(0);
}

int procXevent(fp)
FILE *fp;
{

    static int button_down=0;
    XEvent xe;
    static int i = 0;
    int j;
    int debug=0;
    BOUNDS bb;

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
		db_render(currep,0,&bb,0); /* render the current rep */
		grid_xd = currep->grid_xd;
		grid_yd = currep->grid_yd;
		grid_xs = currep->grid_xs;
		grid_ys = currep->grid_ys;
		grid_xo = currep->grid_xo;
		grid_yo = currep->grid_yo;
	    }
	    draw_grid(win, gcx, grid_xd, grid_yd,
		grid_xs, grid_ys, grid_xo, grid_yo);
	    /* DRAW_MENU */
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
		    if (debug) printf("EVENT LOOP: got Motion\n");

		    x = (double) xe.xmotion.x;
		    y = (double) xe.xmotion.y;
		    V_to_R(&x,&y);
		    snapxy(&x,&y);
		    sprintf(buf,"(%-8g,%-8g)", x, y);
		    if (display_state == D_ON) {
			XDrawImageString(dpy,win,gcx,20, 20, buf, strlen(buf));
		    }

		    if (xold != x || yold != y) {
	    		if (display_state==D_ON) {
			    rubber_draw(x, y);
			}
			xold=x;
			yold=y;
		    } 

		    break;
		case Expose:
		    if (debug) printf("EVENT LOOP: got Expose\n");

		    xwin_window_set(vp_xmin, vp_ymin, vp_xmax, vp_ymax);

		    if (xe.xexpose.count != 0)
			break;
		    if (xe.xexpose.window == win) {
			draw_grid(win, gcx, grid_xd, grid_yd,
			    grid_xs, grid_ys, grid_xo, grid_yo);
		    }  
		
		    for (j=0; j<MAX_CHOICE; j++) {
		         if (panes[j] == xe.xexpose.window) {
			     paint_pane(xe.xexpose.window, panes, gca, gcb, BLACK);
			 }
		    }

		    break;

		case ConfigureNotify:
		    if (debug) printf("EVENT LOOP: got Config Notify\n");
		    top_width = xe.xconfigure.width;
		    height=top_height = xe.xconfigure.height;
		    width=top_width-menu_width;
		    XMoveWindow(dpy, menuwin, top_width-menu_width, 0);
		    XResizeWindow(dpy, menuwin, menu_width, height);
		    XResizeWindow(dpy, win, top_width-menu_width, height);
		    xwin_window_set(vp_xmin, vp_ymin, vp_xmax, vp_ymax);
		    break;
		case ButtonRelease:
		    button_down=0;
		    if (debug) printf("EVENT LOOP: got ButtonRelease\n");
		    break;
		case ButtonPress:
		    if (xe.xexpose.window == win) {
			switch (xe.xbutton.button) {
			    case 1:	/* left button */
				button_down=1;
				x = (double) xe.xmotion.x;
				y = (double) xe.xmotion.y;
				V_to_R(&x,&y);

				/* FIXME: put in facility to turn off snapping for fine picking */
				/*
				if (snap==0) {
				    snap=1;
				} else {
				    snapxy(&x,&y);
				}
				*/

				snapxy(&x,&y);

				/* returning mouse loc as a string */
				sprintf(buf," %g, %g\n", x, y);
				s = buf;

				if (debug) printf("EVENT LOOP: got ButtonPress\n");
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
		    } else {
			for (j=0; j<MAX_CHOICE; j++) {
			    if (panes[j] == xe.xbutton.window) {
				switch (xe.xbutton.button) {
				case 1:
				   s=menu_label[j];
				   break;
				case 2:	
				   break;
				case 3:
				   /* RIGHT button returns EOC */
				   sprintf(buf," ;\n");
				   s = buf;
				   break;
				default:
				   eprintf("unexpected button event: %d",
					xe.xbutton.button);
				   break;
			    	}
			    }
			}
		    }
		    break;
		case KeyPress:
		    if (debug) printf("EVENT LOOP: got KeyPress\n");
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

    extern DISPLAYSTATE display_state;

    if (display_state == D_ON) {
	XDrawLine(dpy, win, gcx, x1, y1, x2, y2);
    }
}

void xwin_draw_line(x1, y1, x2, y2)
int x1,y1,x2,y2;
{
    /* XSetFunction(dpy, gca, GXcopy);  */
    if (display_state == D_ON) {
	XDrawLine(dpy, win, gca, x1, y1, x2, y2);
    }
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

    /* FIXME: line types are turned off for now .... */
    line = 0;	/* for now turn off dashed line types */

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
    } else if (line < 0) {
	line = 0;
    }

    /* there are two switches here because even if you XSetDashes(),
     * the drawn line will still be solid *unless* you also change
     * the line style to LineOnOffDash or LineDoubleDash.  So we set
     * line zero to be LineSolid and all the rest to LineOnOffDash
     * and then the XSetDashes will take effect.  (45 minutes to debug!)
     * - with much appreciation to Ken P. for an example of this in 
     * autoplot(1) code.
     */

    switch (line) {
       case 0:     line_style = LineSolid;     break;
       case 1:     line_style = LineOnOffDash; break;
       case 2:     line_style = LineOnOffDash; break;
       case 3:     line_style = LineOnOffDash; break;
       case 4:     line_style = LineOnOffDash; break;
       default:
	   eprintf("line type %d out of range.", line);
    }        
    
    switch (line) {
       case 0:
       case 1:     dash_list[0]=7; dash_list[1]=2;
                   dash_list[2]=1; dash_list[3]=2; dash_n=4; break;
       case 2:     dash_list[0]=7; dash_list[1]=5; dash_n=2; break;
       case 3:     dash_list[0]=2; dash_list[1]=2; dash_n=2; break;
       case 4:     dash_list[0]=7; dash_list[1]=2;
                   dash_list[2]=3; dash_list[3]=2; dash_n=4; break;
    }

    dash_offset=0;
    XSetLineAttributes(dpy, gca, 0, line_style, CapButt, JoinRound); 
    XSetDashes(dpy, gca, dash_offset, dash_list, dash_n);
}    


draw_grid(win, gc, dx, dy, sx, sy, xorig, yorig)
Window win;
GC gc;
double dx,dy;		/* grid delta */
double sx,sy;		/* grid skip number */
double xorig,yorig;	/* grid origin */
{
    extern unsigned int width, height;
    extern GRIDSTATE grid_state;
    extern int grid_color;
    double delta;
    double x,y, xd,yd;
    int i,j;
    int debug=0;
    extern int grid_notified;

    double xstart, ystart, xend, yend;

    if (debug) printf("draw_grid called with: %g %g %g %g %g %g, color %d\n", 
    	dx, dy, sx, sy, xorig, yorig, grid_color);

    /* colored grid */
    XSetForeground(dpy, gc, colors[grid_color]); /* RCW */

    XSetFillStyle(dpy, gc, FillSolid);

    xstart=(double) 0;
    ystart=(double) height;
	if (debug) printf("starting with %g %g\n", xstart, ystart); 
    V_to_R(&xstart, &ystart);
	if (debug) printf("V_to_R %g %g\n", xstart, ystart); 

    snapxy_major(&xstart,&ystart);
	if (debug) printf("snap %g %g\n", xstart, ystart); 

    xend=(double) width;
    yend=(double) 0;
	if (debug) printf("starting with %g %g\n", xend, yend); 
    V_to_R(&xend, &yend);
	if (debug) printf("V_to_R %g %g\n", xend, yend); 
    snapxy_major(&xend,&yend);
	if (debug) printf("snap %g %g\n", xend, yend); 

    if ( sx <= 0.0 || sy <= 0.0) {
	printf("grid x,y step must be a positive integer\n");
	return(1);
    }


    if (dx > 0.0 && dy >0.0 && sx > 0.0 && sy > 0.0) {

	/* require at least 5 pixels between grid ticks */

	if ( ((xend-xstart)/(dx*sx) >= (double) width/5) ||
	     ((yend-ystart)/(dy*sx) >= (double) height/5) ) {

	    if (!grid_notified) {
		printf("grid suppressed, too many points\n");
		grid_notified++;
	    }

	} else if ( ((xend-xstart)/(dx) >= (double) width/5) ||
	     ((yend-ystart)/(dy) >= (double) height/5) ) {

	    /* this is the old style grid - no fine ticks */

	    for (x=xstart-dx*sx; x<=xend; x+=dx*sx) {
	       for (y=ystart-dy*sy; y<=yend; y+=dy*sy) {
		    xd=x;
		    yd=y;
		    R_to_V(&xd,&yd);
		    if (grid_state == G_ON && display_state == D_ON) {
			XDrawPoint(dpy, win, gc, (int)xd, (int)yd);
		    }
		}
	    }

	} else { 

	    /* draw grid with fine intermediate ticks */

	    /* subtract dx*sx, dy*sy from starting coords to make sure */
	    /* that we draw grid all the way to left margin even after */
	    /* calling snap_major above */

	    if (grid_state == G_ON && display_state == D_ON) {
		for (x=xstart-dx*sx; x<=xend; x+=dx*sx) {
		    for (y=ystart-dy*sy; y<=yend; y+=dy*sy) {
			for (i=0; i<sx; i++) {
			    for (j=0; j<sy; j++) {
				if (i==0 || j==0) {
				    xd=x + (double) i*dx;
				    yd=y + (double) j*dy;
				    R_to_V(&xd,&yd);
				    if (display_state == D_ON) {
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

	if (width > height) {
	    delta = (double) dpy_width/100;
	} else {
	    delta = (double) dpy_height/100;
	}

	/* conveniently reusing these variable names */
	x=(double)0; y=0; R_to_V(&x,&y);

	if (grid_state == G_ON && display_state == D_ON) {
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

void xwin_draw_point(x,y) 
double x, y;
{
    double delta;
    extern unsigned int width, height;
    char buf[BUF_SIZE];

    sprintf(buf,"%d,%d", (int) x, (int) y);

    if (width > height) {
	delta = (double) dpy_width/100;
    } else {
	delta = (double) dpy_height/100;
    }

    R_to_V(&x,&y);

    XDrawLine(dpy, win, gcx, 
	    (int)x, (int)(y-delta), (int)x, (int)(y+delta));
    XDrawLine(dpy, win, gcx, 
	    (int)(x-delta), (int)y, (int)(x+delta), (int)(y)); 

    XDrawImageString(dpy,win,gcx, x+10, y-10, buf, strlen(buf));

    XFlush(dpy);
}

void snapxy(x,y)	/* snap to grid tick */
double *x, *y;
{
    extern double grid_xo, grid_xd;
    extern double grid_yo, grid_yd;
    int debug=0;

    if (debug) printf("snap called with %g %g, and xo %g %g xd %g %g\n", 
    	*x, *y,grid_xo, grid_yo, grid_xd, grid_yd);


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
    extern double grid_xo, grid_xd, grid_xs;
    extern double grid_yo, grid_yd, grid_ys;

    double xd, yd;

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

    if (currep != NULL) {
    	currep->grid_color = color;
    }
}

void xwin_display_set_state(state)
DISPLAYSTATE state;
{
    extern DISPLAYSTATE display_state;
    int debug=0;

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
}

void xwin_grid_state(state) 
GRIDSTATE state;
{
    extern GRIDSTATE grid_state;
    int debug=0;

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
    extern double grid_xd;
    extern double grid_yd;
    extern double grid_xs;
    extern double grid_ys;
    extern double grid_xo;
    extern double grid_yo;
    int debug = 0;
   
    printf("setting grid: xydelta=%g,%g xyskip=%g,%g xyoffset=%g,%g\n",
	xd, yd, xs, ys, xo, yo);

    /* only redraw if something has changed */

    if (grid_xd !=  xd ||
        grid_yd !=  yd ||
	grid_xs !=  xs ||
	grid_ys !=  ys ||
	grid_xo !=  xo ||  
	grid_yo !=  yo) {

	grid_xd= xd; 
	grid_yd= yd; 
	grid_xs= xs; 
	grid_ys= ys; 
	grid_xo= xo; 
	grid_yo= yo; 
	need_redraw++;

	if (currep != NULL) {
	    currep->grid_xd=xd;
	    currep->grid_yd=yd;
	    currep->grid_xs=xs;
	    currep->grid_ys=ys;
	    currep->grid_xo=xo;
	    currep->grid_yo=yo;
	    currep->grid_color=grid_color;
	    /* currep->modified++; */
	}
    }
}

/*
WIN [:X[scale]] [:Z[power]] [:G{ON|OFF}] [:O{ON|OFF}] [:F] [:Nn] [xy1 [xy2]]
    scale is absolute
    power is magnification factor, positive scales up, negative scales down
    xy1 recenters the display on new point
    xy1,xy2 zooms in on selected region
*/

void xwin_window_get(x1, y1, x2, y2) 
double *x1,*y1,*x2,*y2;
{
    extern double vp_xmin, vp_ymin, vp_xmax, vp_ymax;
    *x1 = vp_xmin;
    *y1 = vp_ymin;
    *x2 = vp_xmax;
    *y2 = vp_ymax;
}

void xwin_window_set(x1,y1,x2,y2) /* change the current window parameters */
double x1,y1,x2,y2;
{
    extern XFORM *xp;
    extern double vp_xmin, vp_ymin, vp_xmax, vp_ymax;
    extern double scale,xoffset,yoffset;
    extern int grid_notified;
    double dratio,wratio;
    double tmp;
    int debug=0;

    grid_notified=0;	/* never yet evaluated the grid visibility */ 

    if (debug) printf("xwin_window_set called with %f %f %f %f\n",
    	x1,y1,x2,y2); 

    if (x2 < x1) {		/* canonicalize the selection rectangle */
	tmp = x2; x2 = x1; x1 = tmp;
    }
    if (y2 < y1) {
	tmp = y2; y2 = y1; y1 = tmp;
    }

    if (currep != NULL) {
	currep->minx=x1;
	currep->miny=y1;
	currep->maxx=x2;
	currep->maxy=y2;
	/* currep->modified++; */
    }

    if (debug) printf("setting world window bounds to %g,%g %g,%g\n",
    	x1,y1,x2,y2);
    vp_xmin=x1;
    vp_ymin=y1;
    vp_xmax=x2;
    vp_ymax=y2;

    dratio = (double) height/ (double)width;
    wratio = (vp_ymax-vp_ymin)/(vp_xmax-vp_xmin);

    if (wratio > dratio) {	/* world is taller,skinnier than display */
	scale=((double) height)/(vp_ymax-vp_ymin);
    } else {			/* world is shorter,fatter than display */
	scale=((double) width)/(vp_xmax-vp_xmin);
    }

    xoffset = (((double) width)/2.0) -  ((vp_xmin+vp_xmax)/2.0)*scale;
    yoffset = (((double) height)/2.0) + ((vp_ymin+vp_ymax)/2.0)*scale;

    if (debug) printf("screen width = %d, height=%d\n",width, height);
    if (debug)  printf("scale = %g, xoffset=%g, yoffset=%g\n",
    	scale, xoffset, yoffset);

    /*
    if (debug)  printf("\n");
    if (debug)  printf("dx = xoffset + rx*scale;\n");
    if (debug)  printf("dy = yoffset - ry*scale;\n");
    if (debug)  printf("rx = (dx - xoffset)/scale;\n");
    if (debug)  printf("ry = (yoffset - dy)/scale;\n");
    if (debug)  printf("xmin -> %d\n", (int) (xoffset + vp_xmin*scale));
    if (debug)  printf("ymin -> %d\n", (int) (yoffset - vp_ymin*scale));
    if (debug)  printf("xmax -> %d\n", (int) (xoffset + vp_xmax*scale));
    if (debug)  printf("ymax -> %d\n", (int) (yoffset - vp_ymax*scale));
    */
    
    xp->r11 = scale;
    xp->r12 = 0.0;
    xp->r21 = 0.0;
    xp->r22 = -scale;
    xp->dx  = xoffset;
    xp->dy  = yoffset;

    need_redraw++;
}

/* convert viewport coordinates to real-world coords */
void V_to_R(x,y) 
double *x;
double *y;
{
    extern double xoffset, yoffset, scale;
    *x = (*x - xoffset)/scale;
    *y = (yoffset - *y)/scale;
}

/* convert real-world coordinates to viewport coords */
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
	"white",        /* FIXME: eventually should be black */
	"white",
	"red",
	"green",
	"#7272ff",	/* brighten up our blues a la Ken.P. :-)*/
	"cyan",
	"magenta",
	"yellow"
    };

    XColor exact_def;
    Colormap default_cmap;
    int ncolors = 0;
    int i;
    XVisualInfo visual_info;
    int debug=0;

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

paint_pane(window, panes, ngc, rgc, mode) 
Window window;
Window panes[];
GC ngc, rgc;
int mode;
{
    int win;
    int x = 2, y;
    GC gc;

    if (mode == BLACK) {
    	XSetWindowBackground(dpy, window, 
	    BlackPixel(dpy, scr));
	gc = rgc;
    } else {
    	XSetWindowBackground(dpy, window, 
	    WhitePixel(dpy, scr));
	gc = ngc;
    }

    /* Clearing repaints the background */
    XClearWindow(dpy, window);

    /* Find out winndex of window for label text */
    for (win=0; window != panes[win]; win++) {
    	;
    }

    y = font_info->max_bounds.ascent;

    /* The string length is necessary because strings 
     * for XDrawString may not be null terminated */
    XSetForeground(dpy, gc, colors[2]);
    XDrawString(dpy, window, gc, x, y, menu_label[win],
        strlen(menu_label[win])); 
}


dump_window(window, gc, width, height) 
Window window;
GC gc;
unsigned int width, height;
{
    XImage *xi;
    unsigned long pixelvalue1, pixelvalue2;
    int x, y;
    unsigned long pixel;
    char buf[128];
    int R,G,B;
    int fd;
    int i;

    xi = XGetImage(dpy, win, 0,0, width, height, AllPlanes, ZPixmap);

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
    printf("red mask= %d\n", xi->red_mask);
    printf("green mask= %d\n", xi->green_mask);
    printf("blue mask= %d\n", xi->blue_mask);

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
	   R = R>>10;
	   buf[0] = (unsigned char) 255*R/64;

	   G=pixel & xi->green_mask;
	   G = G>>5;
	   buf[1] = (unsigned char) 255*G/64;

	   B=pixel & xi->blue_mask;
	   buf[2] = (unsigned char) 255*B/32;

	   write(fd, buf, 3);
	}
    }
    printf("\n");
    fflush(stdout);

    close(fd);
    XDestroyImage(xi);
}

void xwin_dump_graphics() {
    dump_window(win, gca, width, height);
}
