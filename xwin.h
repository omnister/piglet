extern int initX();
extern int procXevent();
extern int need_redraw;
void xwin_draw_line(int x1, int y1, int x2, int y2);
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
extern OPTS   *opts;
extern XFORM  transform;
extern XFORM  *xp;

/* When non-zero, this global means the user is done using this program. */
extern int done;

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
extern int xwin_grid();
extern int xwin_window();
