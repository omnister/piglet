
extern unsigned int width, height;

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
extern int quit_now; 		/* set to 1 when done using this program. */
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
extern void xwin_dump_graphics();
extern void xwin_draw_point(double x, double y);


