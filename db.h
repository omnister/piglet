#include <stdlib.h>
#include <stdio.h>

#define ARC 273
#define CIRC 274
#define INST 275
#define LINE 276
#define NOTE 277
#define OVAL 278
#define POLY 279
#define RECT 280
#define TEXT 281

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
            
extern char *strsave( char *string );

extern void append_pair( PAIR p );
extern void discard_pairs( void );
extern COORDS *pair_alloc( PAIR p );
extern COORDS *first_pair, *last_pair; 

extern void append_opt( char *s );
extern void discard_opts( void );
extern OPTS   *opt_alloc( char *s );
extern OPTS   *first_opt, *last_opt; 

extern DB_TAB *db_lookup( char *cellname );
extern DB_TAB *db_install( char *cellname ); 

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

extern int db_add_inst(
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
		DB_TAB *sp
	    );

extern int db_def_print(
		FILE *fp,
		DB_TAB *dp
	    );

extern int db_def_archive(
		DB_TAB *sp
	    );

extern int db_render(
		DB_TAB *cell,
		XFORM *xf,      /* coordinate transform matrix */
		int nest,
		int mode
	    );

extern void draw( NUM x, NUM y, int MODE);
extern void jump(void);

/********************************************************/
/* keep track of current rep */
/* extern DB_TAB *currep; */

/* scratch pointer for new rep */
/* extern DB_TAB *newrep; */

