/* globals for interacting with db.c */

extern DB_TAB *currep;
extern int quit_now; 		/* set to 1 when done using this program. */
extern int need_redraw;

/* start up first window and the X11 event loop */

extern int initX();
extern int procXevent();

/* globals that should eventually be part of a */
/* window structure to allow multiple viewports */

extern unsigned int g_width, g_height; 

/* xwin routines that should eventually also contain a viewport arg */

extern void V_to_R();
extern void R_to_V(); 
extern void snapxy();
extern void snapxy_major();
void xwin_draw_line(int x1, int y1, int x2, int y2);
void xwin_xor_line(int x1, int y1, int x2, int y2);
void xwin_rubber_line(int x1, int y1, int x2, int y2);
void xwin_fill_poly();
void xwin_set_pen_line_fill(int pen, int line, int fill);
typedef enum {D_ON, D_OFF, D_TOGGLE} DISPLAYSTATE;
void xwin_display_set_state( DISPLAYSTATE state );
int  xwin_display_state();
typedef enum {G_ON, G_OFF, G_TOGGLE} GRIDSTATE;
void xwin_grid_state( GRIDSTATE state );
void xwin_grid_color( int color );
extern void xwin_window_set();
extern void xwin_raise_window();
extern void xwin_window_get();
extern int xwin_dump_graphics();
extern void xwin_draw_text(double x, double y, char *s);
extern void xwin_draw_point(double x, double y);
extern void xwin_draw_origin(double x, double y);
extern void xwin_draw_circle(double x, double y);
void xwin_grid_pts( 
    double xd, double yd,
    double xs, double ys, 
    double xo, double yo
);

extern void xwin_doXevent();

/* to set up rubber band routine */

void xwin_set_rubber_callback();
void xwin_clear_rubber_callback();
void xwin_set_title(char *title);

extern char version[];
extern void init_stipples();
extern int get_stipple_index(int fill, int pen);
extern char *get_stipple_bits(int i);
extern const char * xwin_ps_dashes(int line);
extern const char * get_hpgl_fill(int line);
