
extern void V_to_R();
extern void R_to_V(); 
extern void snapxy();
extern void snapxy_major();

extern int initX();
extern int procXevent();
extern int need_redraw;
void xwin_draw_line(int x1, int y1, int x2, int y2);
void xwin_xor_line(int x1, int y1, int x2, int y2);
void xwin_set_pen(int pen);

/* to set up rubber band routine */

void xwin_set_rubber_callback();
void xwin_clear_rubber_callback();


/* 
 * rubber band function receives current location back as arguments 
 *
 *  void 
 *  xwin_rubber_callback(Window win, GC gcx, int x1, int y1, int x2, int y2);
 */

/* globals for interacting with db.c */
extern DB_TAB *currep;
extern DB_TAB *newrep;			/* scratch pointer for new rep */
extern XFORM  screen_transform;
extern XFORM  *xp;

/* When non-zero, this global means the user is done using this program. */
extern int quit_now;

extern unsigned int width, height;	/* window pixel size */

extern double vp_xmin;     		/* world coordinates of viewport */ 
extern double vp_ymin;
extern double vp_xmax;
extern double vp_ymax;	
extern double scale;			/* xform factors from real->viewport */
extern double xoffset;
extern double yoffset;

extern int grid_xd;			/* grid delta */
extern int grid_yd;
extern int grid_xs;			/* how often to display grid ticks */
extern int grid_ys;
extern int grid_xo; 			/* starting offset */
extern int grid_yo;

extern int modified;
extern int need_redraw;

extern char version[];

typedef enum {D_ON, D_OFF, D_TOGGLE} DISPLAYSTATE;
void xwin_display_set_state( DISPLAYSTATE state );

typedef enum {G_ON, G_OFF, G_TOGGLE} GRIDSTATE;
void xwin_grid_state( GRIDSTATE state );

void xwin_grid_color( int color );

void xwin_grid_pts( 
	double xd,
	double yd,
	double xs,
	double ys, 
	double xo,
	double yo
);

extern void xwin_window_set();
extern void xwin_window_get();
