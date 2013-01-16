#include <stdlib.h>
#include <stdio.h>
#include "stack.h"
#include "paper.h"

#define RES 5		/* number of decimals to allow */

/* these definitions map bit fields in an int skipping every other */
/* bit allowing circle visibility to be set with x|CIRC, and circle */
/* modifiability with x|(CIRC*2) (eg: the next higher bit */

#define ARC       1
#define CIRC      4
#define INST     16
#define LINE     64
#define NOTE    256
#define OVAL   1024
#define POLY   4096
#define RECT  16384
#define TEXT  65536
#define ALL   87381   /* sum of all bits */

#define ARC_OPTS  "WR"
#define CIRC_OPTS "WRY"
#define LINE_OPTS "BWH"
#define OVAL_OPTS "WR"
#define POLY_OPTS "W"
#define RECT_OPTS "W"
#define NOTE_OPTS "JMNRYZF"
#define TEXT_OPTS "JMNRYZF"
#define INST_OPTS "MRSXYZ"
#define NONAME_OPTS "MRX"

#define MAX_LAYER 1024

extern char *PIG_PATH;

int show[MAX_LAYER];

/*   definition of hierarchical editor data structures 
 *
 *   (this chain from HEAD is the cell definition symbol table)
 *
 *   all insertions are made at the tail to avoid reversing definition order
 *   everytime an archive is made and retrieved (a classic HP Piglet bug
 *   which made it very difficult to run a diff(1) on two versions of an
 *   archive file.
 *
 *   HEAD->[db_tab <inst_name0> ]->[<inst_name1>]->...[<inst_namek>]->NULL
 *                           |                |                  |
 *                           |               ...                ...
 *                           v
 *                       [db_deflist ]->[ ]->[ ]->...[db_deflist]->NULL
 *                                  |    |    |
 *                                  |    |    v
 *                                  |    v  [db_inst] (recursive call)
 *                                  v  [db_line]
 *                                [db_rect]
 *
 *   actually both db_tab and db_deflist chains are doubly-linked,
 *   but this is not shown here for simplicity...  
 *
 */

/*
 *  OK, we use doubly-linked lists all over the place in piglet.
 *  Here's how they work, since I've never seen this sort of
 *  list described anywhere else before.
 *
 *  Each element of the list has pointers to the next element
 *  and pointers to the previous element.  db_deflist
 *  elements are defined like:
 *
 *	typedef struct db_deflist {
 *          ... (various stuff);
 *	    struct db_deflist *next;    // to link to another
 *	    struct db_deflist *prev;    // and to the previous
 *	} DB_DEFLIST;
 *
 * Here is a picture.  "HEAD" is defined as "DB_DEFLIST *HEAD", and is
 * the head pointer.  We have three db_deflist structures linked
 * together here, first (F), second (S) and last (L).  Last->next points
 * at NULL. 
 * 
 * Every cell->next points at the next one in line except for the last
 * one which points at NULL.  Every cell->prev points at the previous
 * one except the first one which points at the last one. 
 *
 * New cells get inserted at HEAD->prev. 
 *
 *             +--prev----------------+
 *             |                      v
 *   HEAD-->[ F ]-next->[ S ]-next->[ L ]--next-->NULL
 *           ^           | ^           |
 *           |           | |           |
 *           +-prev------+ +-prev------+
 *
 * It is an almost completely circular doubly-linked
 * list with HEAD pointing at the first element, and with
 * the last element marked by having its next pointer pointing at NULL.
 *
 * When you look at the code for manipulating these lists (db_tabs,
 * (db_deflists in db.c, coords in coords.c, and stacks in stack.c),
 * it is often helpful to draw out one of these diagrams for both
 * before and after the operation to make sure you've properly 
 * handled all the links.
 *
 * Happy hacking!  RCW 01/12/07
 */

#define MAXFILENAME 128

/* typedef NUM int; */
typedef double NUM;

typedef struct coord_pair {
    NUM x,y;
} PAIR;

typedef struct coord_list {
    PAIR coord;
    struct coord_list *next;
    struct coord_list *prev;
} COORDS;

typedef struct xform {
     double r11;
     double r12;
     double r21;
     double r22;
     double dx;
     double dy;
} XFORM; 

COORDS *coord_new(NUM x, NUM y);
COORDS *coord_copy(XFORM *xp, COORDS *CP);
COORDS *coord_append(COORDS *CP, NUM x, NUM y);
void coord_swap_last(COORDS *CP, NUM x, NUM y);
int coord_get(COORDS *CP, int n, NUM *x, NUM *y);
void coord_drop(COORDS *CP);
void coord_print(COORDS *CP);
int coord_count(COORDS *CP);

typedef struct bounds {
    double xmin;
    double ymin;
    double xmax;
    double ymax;
    int init;	/* 0 = non-initialized */
} BOUNDS;

/********************************************************/

typedef struct db_tab {
    char *name;			/* cell name */
    double minx,miny;		/* for bounding box computation */
    double maxx,maxy;		/* ... */
    int seqflag;

    double vp_xmin, vp_xmax;	/* for storing window parameters */
    double vp_ymin, vp_ymax;
    double old_xmin, old_xmax;	/* for storing old win parms for win :z */
    double old_ymin, old_ymax;
    double scale;
    double xoffset, yoffset;

    int show[MAX_LAYER];	/* each component has its own show set */

    /* grid settings for each cell are stored in this structure  */
    /* the grid in use at time of SAVE is put in the disk */
    /* archive with a GRID command.  When you edit a cell in memory */
    /* the edit automatically re-uses the remembered grid */

    double grid_xd;
    double grid_yd;
    double grid_xs;
    double grid_ys;
    double grid_xo;
    double grid_yo;
    int    grid_color;	

    int logical_level;
    double lock_angle;

    int display_state;		/* turns X11 display on or off (see xwin.c)*/

    int modified;		/* for EXIT/SAVE and bounding box usage */

    int being_edited;		/* flag to prevent recursive edits */
    				/* 0 = not edited, !0 = location in edit stack */

    int flag;			/* bookingkeeping flag for db_def_archive() */
    int is_tmp_rep;		/* used to mark temporary instances created by WRAP */

    struct db_deflist *dbhead;  /* pointer to first cell definition */
    STACK  *undo;		/* place for keeping older versions */
    STACK  *redo;               /* place for keeping newer versions */
    int chksum;			/* checksum of last autosaved copy */

    char *background;		/* cell to display as background */
    int prims;			/* number of prims in the recursive expansion */
    				/* will be valid after first db_render() */

    struct db_tab *next;    	/* to link to next */
    struct db_tab *prev;    	/* link to previous */
} DB_TAB;

/********************************************************/

typedef struct opt_list {
    double font_size;       /* :F<font_size> */
    double height;
    int mirror;             /* :M<x,xy,y>    */
    int font_num;           /* :N<font_number> */
    int justification;      /* :J<justification> */
    int stepflag;	    /* true if rows/cols valid */
    int rows;		    /* :S<rows>,<cols> */
    int cols;		
    double rotation;        /* :R<rotation,resolution> */
    double width;           /* :W<width> */
    double scale; 	    /* :X<scale> */
    double aspect_ratio;    /* :Y<aspect_ratio> */
    double slant;           /* :Z<slant> */
    char *sname;	    /* signal name */
    char *cname;	    /* signal name */
    int bezier;
} OPTS;

extern void    db_set_font_size(double size);
extern double  db_get_font_size();
extern void    db_set_text_slant(double slant);
extern double  db_get_text_slant();

extern OPTS *opt_create();
extern void opt_set_defaults( OPTS *opts  );
extern OPTS *opt_copy( OPTS *opts);
extern void append_opt( char *s );
extern void discard_opts( void );
extern OPTS *opt_alloc( char *s );
extern OPTS *first_opt, *last_opt; 
extern int  is_ortho(OPTS *opts);	

#define MIRROR_OFF 0
#define MIRROR_X   1
#define MIRROR_Y   2
#define MIRROR_XY  3

typedef struct db_arc {
    int layer;
    OPTS *opts;
    NUM x1,y1,x2,y2,x3,y3;
} DB_ARC; 

typedef struct db_circ {
    int layer;
    OPTS *opts;
    NUM x1,y1,x2,y2;
} DB_CIRC; 

typedef struct db_inst {
    char *name; 	    /* was struct db_tab *def; */
    OPTS *opts;
    NUM x,y;
    NUM colx,coly;		// column vector 
    NUM rowx,rowy;		// row vector
} DB_INST; 

typedef struct db_line {
    int layer;
    OPTS *opts;
    COORDS *coords;
} DB_LINE; 

typedef struct db_oval {
    int layer;
    OPTS *opts;
    NUM x1,y1,x2,y2,x3,y3;
} DB_OVAL; 

typedef struct db_poly {
    int layer;
    OPTS *opts;
    COORDS *coords;
} DB_POLY; 

typedef struct db_rect {
    int layer;
    OPTS *opts;
    NUM x1,y1,x2,y2;
} DB_RECT; 

typedef struct db_note {
    int layer;
    OPTS *opts;
    char *text;
    NUM x,y;
} DB_NOTE; 

typedef struct db_text {
    int layer;
    OPTS *opts;
    char *text;
    NUM x,y;
} DB_TEXT; 

/********************************************************/

typedef struct db_deflist {
    int type;
    union {
        DB_ARC  *a;  /* arc definition */
        DB_CIRC *c;  /* circle definition */
        DB_INST *i;  /* instance call */
        DB_LINE *l;  /* line definition */
        DB_NOTE *n;  /* note definition */
        DB_OVAL *o;  /* oval definition */
        DB_POLY *p;  /* polygon definition */
        DB_RECT *r;  /* rectangle definition */
        DB_TEXT *t;  /* text definition */
    } u;
    NUM xmin, ymin, xmax, ymax; /* wrapper bounds for screening extents */
    struct db_deflist *next;    /* to link to another */
    struct db_deflist *prev;    /* and to the previous */
} DB_DEFLIST;
            
extern char *strsave( char *string );

extern DB_TAB *db_lookup( char *cellname );
extern DB_TAB *db_install( char *cellname ); 
extern DB_TAB *db_edit_pop( int level ); 

extern int db_add_arc(  
		DB_TAB *cell,
		int layer,
		OPTS *opts,
		NUM x1, NUM y1,
		NUM x2, NUM y2,
		NUM x3, NUM y3
	    );

extern int db_add_circ(
		DB_TAB *cell,
		int layer,
		OPTS *opts,
		NUM x1, NUM y1,
		NUM x2, NUM y2
	    );

extern struct db_inst * db_add_inst(
		DB_TAB *cell, 
		DB_TAB *subcell, 
		OPTS *opts,
		NUM x, NUM y
	    );

extern int db_add_line(
		DB_TAB *cell,
		int layer,
		OPTS *opts,
		COORDS *coords
	    );

extern int db_add_note(
		DB_TAB *cell,
		int layer,
		OPTS *opts,
		char *string,
		NUM x, NUM y      
	    );

extern int db_add_oval(
		DB_TAB *cell,
		int layer,
		OPTS *opts,
		NUM x1, NUM y1, 
		NUM x2, NUM y2, 
		NUM x3, NUM y3
	    );

extern int db_add_poly(
		DB_TAB *cell,
                int layer,
                OPTS *opts,
                COORDS *coords    
	    );

extern int db_add_rect(
		DB_TAB *cell,
                int layer,
                OPTS *opts,
                NUM x1, NUM y1,
                NUM x2, NUM y2
	    );

extern int db_add_text(
		DB_TAB *cell,
		int layer,
		OPTS *opts,
		char *string,
		NUM x, NUM y      
	    );

extern int db_save(
	/*	LEXER *lp,
		DB_TAB *sp,
		char *name */
	    );

extern void db_def_print(
		FILE *fp,
		DB_TAB *dp,
		int mode
	    );

extern int db_def_archive(
		DB_TAB *sp,
		int smash,	/* if !==0, smash the archive */
		int process	/* if !==0, include process file in archive */
	    );

extern void printdef(
		FILE *fp, 
		DB_DEFLIST *p, 
		DB_DEFLIST *pinstdef
	    );

#define FILL_OFF 0
#define FILL_ON 1
#define FILL_TOGGLE 2
extern void db_set_fill(int nest); 	/* set global fill level */

extern void db_set_nest(int nest); 	/* set global display nest level */
extern void db_set_bounds(int bounds); 	/* set global display bounds level */

void db_unlink_component(DB_TAB *cell, DB_DEFLIST *p);
// void db_unlink_component(DB_TAB *cell, struct db_deflist dp)


typedef struct selpnt {
    DB_DEFLIST *p;
    double *xsel;         /* pointers to coordinates in component */
    double *ysel;
    double xselorig;      /* original values prior to any applied offset */
    double yselorig;
    struct selpnt *next;
    struct selpnt *prev;
} SELPNT;

extern SELPNT *selpnt_new(double *x,double *y, DB_DEFLIST *pdef);
extern void selpnt_clear(SELPNT **selhead);
extern void selpnt_print(SELPNT **selhead);
extern void selpnt_save(SELPNT **selhead, double *x, double *y, DB_DEFLIST *pdef);

DB_DEFLIST *db_ident(
		DB_TAB *cell,
		NUM x,
		NUM y,
		int mode,
		int pick_layer,
		int comp,
		char *name
	    );

SELPNT  *db_ident2(
		DB_TAB *cell,
		NUM x,
		NUM y,
		int mode,
		int pick_layer,
		int comp,
		char *name
	    );

SELPNT *db_ident_region2(
		DB_TAB *cell,
		NUM xx1,
		NUM yy1,
		NUM xx2,
		NUM yy2,
		int mode,
		int pick_layer,
		int comp,
		char *name
	    );

STACK *db_ident_region(
		DB_TAB *cell,
		NUM xx1,
		NUM yy1,
		NUM xx2,
		NUM yy2,
		int mode,
		int pick_layer,
		int comp,
		char *name
	    );

extern double db_area(
		DB_DEFLIST *component
	    );

extern int db_list(
		DB_TAB *cell
	    );


extern int db_render(
		DB_TAB *cell,
		int nest,
		BOUNDS *bb,
		int mode
	    );

DB_DEFLIST *db_copy_component();

extern void db_move_component();
extern void db_set_layer();
extern int  db_list_unsaved();
extern int  db_remove_autosavefiles();
extern void db_drawbounds();
extern int  db_contains();
extern void db_notate();
extern void db_highlight();
extern void db_print_opts();
extern void db_insert_component();
extern void db_purge();
extern void db_unlink_cell(DB_TAB *sp);
extern int  db_exists();
extern void db_list_db();
extern double  dist(double x, double y);
extern int ask( /* LEXER *lp, char *msg */ ); 

extern int db_plot();			/* plot the device to a file */

extern void draw( NUM x, NUM y, BOUNDS *bb, int MODE);
extern void jump(BOUNDS *bb, int MODE);

extern void show_set_visible(DB_TAB *currep, int comp, int layer, int state);
extern void show_set_modify(DB_TAB *currep, int comp, int layer, int state);
extern int  show_check_modifiable(DB_TAB *currep, int comp, int layer);
extern int  show_check_visible(DB_TAB *currep, int comp, int layer);
extern void show_init(DB_TAB *currep);

/********************************************************/
/* support routines */
double max(), min();

/* matrix routines */
XFORM *compose();
void mat_rotate();
void mat_scale();
void mat_slant();
void mat_print();

/* primitives for outputting autoplot,X primitives */
void draw();
void jump();
void set_layer();
void set_line();
void startpoly();
void endpoly();

/* routines for expanding db entries into vectors */
void do_arc(),  do_circ(), do_line(), do_note();
void do_oval(), do_poly(), do_rect(), do_text();

void xform_point(XFORM *xp, double *xx, double *yy); 
XFORM *matrix_from_opts(OPTS *opts);

void license();		/* from license.h */

int db_arc_smash(DB_TAB *cell, XFORM *xform, int ortho);
int db_cksum(DB_DEFLIST *cell);
DB_DEFLIST *db_copy_deflist(DB_DEFLIST *head);
int db_checkpoint();
void db_free(DB_DEFLIST *dp);
void db_fsck(DB_DEFLIST *dp);
int readin();
int loadrep();
int aborted;	// used to abort drawing

void escstring(char *dst, char *src);

