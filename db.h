#include <stdlib.h>

/* definition of hierarchical editor data structures */

/*
 *   (this chain from HEAD is the cell definition symbol table)
 *
 *   all insertions are made at TAIL to avoid reversing definition order
 *   everytime an archive is made and retrieved (a classic HP Piglet bug)
 *
 *
 *                                              TAIL--|
 *                                                    v 
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
 *
 */


#define MAXFILENAME 128

/* typedef NUM int; */
typedef double NUM;

typedef struct opt_list {
    char *optstring;
    struct opt_list *next;
} OPTS;

typedef struct coord_pair {
    NUM x,y;
} PAIR;

typedef struct coord_list {
    PAIR coord;
    struct coord_list *next;
} COORDS;

typedef struct xform {
     double r11;
     double r12;
     double r21;
     double r22;
     double dx;
     double dy;
} XFORM; 

/********************************************************/

typedef struct db_tab {
    char *name;             	/* cell name */
    double minx,miny;		/* for bounding box computation */
    double maxx,maxy;		/* ... */
    int modified;		/* for EXIT/SAVE and bounding box usage */
    int flag;			/* bookingkeeping flag for db_def_archive() */
    struct db_deflist *dbhead;  /* pointer to first cell definition */
    struct db_deflist *dbtail;  /* pointer to last cell definition */
    struct db_tab *next;    	/* to link to another */
} DB_TAB;

/********************************************************/


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
    struct db_tab *def;
    OPTS *opts;
    NUM x,y;
} DB_INST; 

typedef struct db_line {
    int layer;
    OPTS *opts;
    COORDS *coords;
} DB_LINE; 

typedef struct db_note {
    int layer;
    OPTS *opts;
    char *text;
    NUM x,y;
} DB_NOTE; 

typedef struct db_oval {
    int layer;
    OPTS *opts;
    NUM x1,y1,x2,y2,x3,y3;
} DB_OVAL; 

typedef struct db_poly {
    NUM layer;
    OPTS *opts;
    COORDS *coords;
} DB_POLY; 

typedef struct db_rect {
    int layer;
    OPTS *opts;
    NUM x1,y1,x2,y2;
} DB_RECT; 

typedef struct db_text {
    int layer;
    OPTS *opts;
    char *text;
    NUM x,y;
} DB_TEXT; 


/********************************************************/

typedef struct db_deflist {
    short   type;
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
    struct db_deflist *next;    /* to link to another */
} DB_DEFLIST;
            
extern char *emalloc();
extern char *strsave();

extern void append_pair();
extern void discard_pairs();
extern COORDS *pair_alloc();
extern COORDS *first_pair, *last_pair; 

extern void append_opt();
extern void discard_opts();
extern OPTS   *opt_alloc();
extern OPTS   *first_opt, *last_opt; 

extern DB_TAB *db_lookup();
extern DB_TAB *db_install(); 

extern int db_add_arc();
extern int db_add_circ();
extern int db_add_inst();
extern int db_add_line();
extern int db_add_note();
extern int db_add_oval();
extern int db_add_poly();
extern int db_add_rect();
extern int db_add_text();

extern int db_save();
extern int db_def_print();
extern int db_def_archive();
extern int db_render();

extern void draw();
extern void jump();

/********************************************************/

extern DB_TAB *currep;	/* keep track of current rep */
extern DB_TAB *newrep;  /* scratch pointer for new rep */

