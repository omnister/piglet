
#include "db.h"		/* hierarchical database routines */
#include "eprintf.h"	/* error reporting functions */

#include <stdio.h>
#include <math.h>
#include "token.h"
#include "xwin.h"       /* for snapxy() */

#define EPS 1e-6

/* master symbol table pointers */
static DB_TAB *HEAD = 0;
static DB_TAB *TAIL = 0;

COORDS *first_pair, *last_pair; 
OPTS   *first_opt, *last_opt;

/* global drawing variables */
XFORM *transform;	/* global xform matrix */
NUM xmax, ymax; 	/* globals for finding bounding boxes */
NUM xmin, ymin;
double max(), min();
int drawon=1;		/* 0 = dont draw, 1 = draw */
int nestlevel=9;
int X=1;		/* 1 = draw to X, 0 = emit autoplot commands */

/* routines for expanding db entries into vectors */
void do_arc(),  do_circ(), do_line(), do_note();
void do_oval(), do_poly(), do_rect(), do_text();

/* matrix routines */
XFORM *compose();
void mat_rotate();
void mat_scale();
void mat_slant();

/* primitives for outputting autoplot,X primitives */
void draw();
void jump();
void set_pen();
void set_line();

/********************************************************/

DB_TAB *db_lookup(s)           /* find s in db */
char *s;
{
    DB_TAB *sp;

    for (sp=HEAD; sp!=(DB_TAB *)0; sp=sp->next)
        if (strcmp(sp->name, s)==0)
            return sp;
    return 0;               	/* 0 ==> not found */
} 

DB_TAB *db_install(s)		/* install s in db */
char *s;
{
    DB_TAB *sp;

    /* this is written to ensure that a db_print is */
    /* not in reversed order...  New structures are */
    /* added at the end of the list using TAIL      */

    sp = (DB_TAB *) emalloc(sizeof(struct db_tab));
    sp->name = emalloc(strlen(s)+1); /* +1 for '\0' */
    strcpy(sp->name, s);

    sp->next   = (DB_TAB *) 0; 
    sp->dbhead = (struct db_deflist *) 0; 
    sp->dbtail = (struct db_deflist *) 0; 
    sp->maxx = 0.0; 
    sp->maxy = 0.0; 

    if (HEAD ==(DB_TAB *) 0) {	/* first element */
	HEAD = TAIL = sp;
    } else {
	TAIL->next = sp;		/* current tail points to new */
	TAIL = sp;			/* and tail becomes new */
    }

    return (sp);
}

int db_purge(s)			/* remove all definitions for s */
char *s;
{
    /* not yet implemented */
}

/* save a non-recursive archive of single cell to a file called "cell.d" */

int db_save(sp)           	/* print db */
DB_TAB *sp;
{

    FILE *fp;
    char buf[MAXFILENAME];
    int err=0;

    mkdir("./cells", 0777);
    snprintf(buf, MAXFILENAME, "./cells/%s.d", sp->name);
    err+=((fp = fopen(buf, "w+")) == 0); 

    db_def_print(fp, sp); 
    err+=(fclose(fp) != 0);

    return(err);
}


int db_def_archive(sp) 
DB_TAB *sp;
{
    FILE *fp;
    char buf[MAXFILENAME];
    int err=0;

    mkdir("./cells", 0777);
    snprintf(buf, MAXFILENAME, "./cells/%s_I", sp->name);
    err+=((fp = fopen(buf, "w+")) == 0); 

    fp = fopen(buf, "w+"); 

    db_def_arch_recurse(fp,sp);
    db_def_print(fp, sp);

    err+=(fclose(fp) != 0);

    return(err);
}

int db_def_arch_recurse(fp,sp) 
FILE *fp;
DB_TAB *sp;
{
    DB_TAB *dp;
    COORDS *coords;
    DB_DEFLIST *p; 

    /* clear out all flag bits in cell definition headers */
    for (dp=HEAD; dp!=(DB_TAB *)0; dp=dp->next) {
	sp->flag=0;
    }

    for (p=sp->dbhead; p!=(struct db_deflist *)0; p=p->next) {

	if (p->type == INST && !(p->u.i->def->flag)) {

	     db_def_arch_recurse(fp, p->u.i->def); 	/* recurse */
	     (p->u.i->def->flag)++;

	     db_def_print(fp, p->u.i->def);
	}
    }

    return(0); 	
}


int db_def_print(fp, dp) 
FILE *fp;
DB_TAB *dp;
{
    COORDS *coords;
    DB_DEFLIST *p; 
    int i;

    fprintf(fp, "PURGE %s;\n", dp->name);
    fprintf(fp, "EDIT ");
    fprintf(fp, "%s;\n",dp->name);
    fprintf(fp, "WIN %g,%g %g,%g\n", dp->minx, dp->miny, dp->maxx, dp->maxy);

    for (p=dp->dbhead; p!=(struct db_deflist *)0; p=p->next) {
	switch (p->type) {
        case ARC:  /* arc definition */
	    fprintf(fp, "#add ARC not inplemented yet\n");
	    break;

        case CIRC:  /* circle definition */

	    fprintf(fp, "ADD C%d ", p->u.c->layer);

	    db_print_opts(fp, p->u.c->opts, "WR");

	    fprintf(fp, "%g,%g %g,%g;\n",
		p->u.c->x1,
		p->u.c->y1,
		p->u.c->x2,
		p->u.c->y2
	    );
	    break;

        case LINE:  /* line definition */

	    fprintf(fp, "ADD L%d ", p->u.l->layer);

	    db_print_opts(fp, p->u.l->opts, "W");

	    i=1;
	    coords = p->u.l->coords;
	    while(coords != NULL) {
		fprintf(fp, " %g,%g", coords->coord.x, coords->coord.y);
		if ((coords = coords->next) != NULL && (++i)%4 == 0) {
		    fprintf(fp,"\n");
		}
	    }
	    fprintf(fp, ";\n");

	    break;

        case NOTE:  /* note definition */

	    fprintf(fp, "ADD N%d ", p->u.n->layer); 

	    db_print_opts(fp, p->u.n->opts, "MRYZF");

	    fprintf(fp, "\"%s\" %g,%g;\n",
		p->u.n->text,
		p->u.n->x,
		p->u.n->y
	    );
	    break;

        case OVAL:  /* oval definition */
	    fprintf(fp, "#add OVAL not inplemented yet\n");
	    break;

        case POLY:  /* polygon definition */
	    fprintf(fp, "ADD P%d", p->u.p->layer);
	    
	    db_print_opts(fp, p->u.p->opts, "W");

	    i=1;
	    coords = p->u.p->coords;
	    while(coords != NULL) {
		fprintf(fp, " %g,%g", coords->coord.x, coords->coord.y);
		if ((coords = coords->next) != NULL && (++i)%4 == 0) {
		    fprintf(fp,"\n");
		}
	    }
	    fprintf(fp, ";\n");

	    break;

	case RECT:  /* rectangle definition */
	    fprintf(fp, "ADD R%d ", p->u.r->layer);
	    db_print_opts(fp, p->u.r->opts, "W");
	    fprintf(fp, "%g,%g %g,%g;\n",
		p->u.r->x1,
		p->u.r->y1,
		p->u.r->x2,
		p->u.r->y2
	    );
	    break;

        case TEXT:  /* text definition */
	    fprintf(fp, "ADD T%d ", p->u.t->layer); 
	    db_print_opts(fp, p->u.t->opts, "MRYZF");
	    fprintf(fp, "%s %g,%g;\n",
		p->u.t->text,
		p->u.t->x,
		p->u.t->y
	    );
	    break;

        case INST:  /* instance call */
	    fprintf(fp, "ADD %s ", p->u.i->def->name);
	    db_print_opts(fp, p->u.i->opts, "MRXYZ");
	    fprintf(fp, "%g,%g;\n",
		p->u.i->x,
		p->u.i->y
	    );
	    break;

	default:
	    eprintf("unknown record type (%d) in db_def_print\n", p->type );
	    break;
	}
    }

    fprintf(fp, "SAVE;\n");
    fprintf(fp, "EXIT;\n\n");
}


int db_print_opts(fp, popt, validopts) /* print options */
FILE *fp;
OPTS *popt;
/* validopts is a string which sets which options are printed out */
/* an example for a line is "W", a note uses "MRYZF" */
char *validopts;
{
    char *p;

    for (p=validopts; *p != '\0'; p++) {
    	switch(toupper(*p)) {
	    case 'F':
		fprintf(fp, ":F%g ", popt->font_size);
	    	break;
	    case 'M':
		switch(popt->mirror) {
		    case MIRROR_OFF:
		    	break;
		    case MIRROR_X:
	    		fprintf(fp, ":MX ");
			break;
		    case MIRROR_Y:
	    		fprintf(fp, ":MY ");
			break;
		    case MIRROR_XY:
	    		fprintf(fp, ":MXY ");
			break;
		    default:
	    		weprintf("invalid mirror case\n");
		    	break;	
		}
	    	break;
	    case 'R':
		if (popt->rotation != 0.0) {
		    fprintf(fp, ":R%g ", popt->rotation);
		}
	    	break;
	    case 'W':
		if (popt->width != 0.0) {
		    fprintf(fp, ":W%g ", popt->width);
		}
	    	break;
	    case 'X':
		if (popt->scale != 1.0) {
		    fprintf(fp, ":X%g ", popt->scale);
		}
	    	break;
	    case 'Y':
		if (popt->aspect_ratio != 1.0) {
		    fprintf(fp, ":Y%g ", popt->aspect_ratio);
		}
	    	break;
	    case 'Z':
		if (popt->slant != 0.0) {
		    fprintf(fp, ":Z%g ", popt->slant);
		}
	    	break;
	    default:
		weprintf("invalid option %c\n", *p); 
	    	break;
	}
    }
}


int db_add_arc(cell, layer, opts, x1,y1,x2,y2,x3,y3) 
DB_TAB *cell;
int layer;
OPTS *opts;
NUM x1,y1,x2,y2,x3,y3;
{
    struct db_arc *ap;
    struct db_deflist *dp;
 
    ap = (struct db_arc *) emalloc(sizeof(struct db_arc));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;

    /* add definition at *end* of list */
    if (cell->dbhead == NULL) {
	cell->dbhead = cell->dbtail = dp;
    } else {
	cell->dbtail->next = dp;
	cell->dbtail = dp;
    }

    dp->u.a = ap;
    dp->type = ARC;

    ap->layer=layer;
    ap->opts=opts;
    ap->x1=x1;
    ap->y1=y1;
    ap->x2=x2;
    ap->y2=y2;
    ap->x3=x3;
    ap->y3=y3;

    return(0);
}

int db_add_circ(cell, layer, opts, x1,y1,x2,y2)
DB_TAB *cell;
int layer;
OPTS *opts;
NUM x1,y1,x2,y2;
{
    struct db_circ *cp;
    struct db_deflist *dp;
 
    cp = (struct db_circ *) emalloc(sizeof(struct db_circ));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;

    /* add definition at *end* of list */
    if (cell->dbhead == NULL) {
	cell->dbhead = cell->dbtail = dp;
    } else {
	cell->dbtail->next = dp;
	cell->dbtail = dp;
    }

    dp->u.c = cp;
    dp->type = CIRC;

    cp->layer=layer;
    cp->opts=opts;
    cp->x1=x1;
    cp->y1=y1;
    cp->x2=x2;
    cp->y2=y2;

    return(0);
}

int db_add_line(cell, layer, opts, coords)
DB_TAB *cell;
int layer;
OPTS *opts;
COORDS *coords;
{
    struct db_line *lp;
    struct db_deflist *dp;
 
    lp = (struct db_line *) emalloc(sizeof(struct db_line));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;

    /* add definition at *end* of list */
    if (cell->dbhead == NULL) {
	cell->dbhead = cell->dbtail = dp;
    } else {
	cell->dbtail->next = dp;
	cell->dbtail = dp;
    }

    dp->u.l = lp;
    dp->type = LINE;

    lp->layer=layer;
    lp->opts=opts;
    lp->coords=coords;

    return(0);
}

int db_add_note(cell, layer, opts, string ,x,y) 
DB_TAB *cell;
int layer;
OPTS *opts;
char *string;
NUM x,y;
{
    struct db_note *np;
    struct db_deflist *dp;
 
    np = (struct db_note *) emalloc(sizeof(struct db_note));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;

    /* add definition at *end* of list */
    if (cell->dbhead == NULL) {
	cell->dbhead = cell->dbtail = dp;
    } else {
	cell->dbtail->next = dp;
	cell->dbtail = dp;
    }

    dp->u.n = np;
    dp->type = NOTE;

    np->layer=layer;
    np->text=string;
    np->opts=opts;
    np->x=x;
    np->y=y;

    return(0);
}

int db_add_oval(cell, layer, opts, x1,y1,x2,y2,x3,y3) 
DB_TAB *cell;
int layer;
OPTS *opts;
NUM x1,y1, x2,y2, x3,y3;
{
    struct db_oval *op;
    struct db_deflist *dp;
 
    op = (struct db_oval *) emalloc(sizeof(struct db_oval));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;

    /* add definition at *end* of list */
    if (cell->dbhead == NULL) {
	cell->dbhead = cell->dbtail = dp;
    } else {
	cell->dbtail->next = dp;
	cell->dbtail = dp;
    }

    dp->u.o = op;
    dp->type = OVAL;

    op->layer=layer;
    op->opts=opts;
    op->x1=x1;
    op->y1=y1;
    op->x2=x2;
    op->y2=y2;
    op->x3=x3;
    op->y3=y3;

    return(0);
}

int db_add_poly(cell, layer, opts, coords)
DB_TAB *cell;
int layer;
OPTS *opts;
COORDS *coords;
{
    struct db_poly *pp;
    struct db_deflist *dp;
 
    pp = (struct db_poly *) emalloc(sizeof(struct db_poly));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;

    /* add definition at *end* of list */
    if (cell->dbhead == NULL) {
	cell->dbhead = cell->dbtail = dp;
    } else {
	cell->dbtail->next = dp;
	cell->dbtail = dp;
    }

    dp->u.p = pp;
    dp->type = POLY;

    pp->layer=layer;
    pp->opts=opts;
    pp->coords=coords;

    return(0);
}

int db_add_rect(cell, layer, opts, x1,y1,x2,y2)
DB_TAB *cell;
int layer;
OPTS *opts;
NUM x1,y1,x2,y2;
{
    struct db_rect *rp;
    struct db_deflist *dp;
 
    rp = (struct db_rect *) emalloc(sizeof(struct db_rect));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;

    /* add definition at *end* of list */
    if (cell->dbhead == NULL) {
	cell->dbhead = cell->dbtail = dp;
    } else {
	cell->dbtail->next = dp;
	cell->dbtail = dp;
    }

    dp->u.r = rp;
    dp->type = RECT;

    rp->layer=layer;
    rp->x1=x1;
    rp->y1=y1;
    rp->x2=x2;
    rp->y2=y2;
    rp->opts = opts;

    return(0);
}

int db_add_text(cell, layer, opts, string ,x,y) 
DB_TAB *cell;
int layer;
OPTS *opts;
char *string;
NUM x,y;
{
    struct db_text *tp;
    struct db_deflist *dp;
 
    tp = (struct db_text *) emalloc(sizeof(struct db_text));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;

    /* add definition at *end* of list */
    if (cell->dbhead == NULL) {
	cell->dbhead = cell->dbtail = dp;
    } else {
	cell->dbtail->next = dp;
	cell->dbtail = dp;
    }

    dp->u.t = tp;
    dp->type = TEXT;

    tp->layer=layer;
    tp->text=string;
    tp->opts=opts;
    tp->x=x;
    tp->y=y;

    return(0);
}

int db_add_inst(cell, subcell, opts, x, y)
DB_TAB *cell;
DB_TAB *subcell;
OPTS *opts;
NUM x,y;
{
    struct db_inst *ip;
    struct db_deflist *dp;
 
    ip = (struct db_inst *) emalloc(sizeof(struct db_inst));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;

    /* add definition at *end* of list */
    if (cell->dbhead == NULL) {
	cell->dbhead = cell->dbtail = dp;
    } else {
	cell->dbtail->next = dp;
	cell->dbtail = dp;
    }

    dp->u.i = ip;
    dp->type = INST;

    ip->def=subcell;
    ip->x=x;
    ip->y=y;
    ip->opts=opts;
}

/* a simple test harness */
/*
main () { 	

    DB_TAB *currep, *oldrep;

    currep = db_install("1foo");
    currep = db_install("2xyz");
	db_add_rect(currep,1,2.0,3.0,4.0,5.0);
	db_add_rect(currep,6,7.0,8.0,9.0,10.0);
    oldrep = currep;
    currep = db_install("3abc");
	db_add_rect(currep,11,12.0,13.0,14.0,15.0);
	db_add_inst(currep,oldrep,16.0,17.0);
    db_print();
}
*/


COORDS *pair_alloc(p)
PAIR p;
{	
    COORDS *temp;
    temp = (COORDS *) emalloc(sizeof(COORDS));
    temp->coord = p;
    temp->next = NULL;
    return(temp);
}


void append_pair(p)
PAIR p;
{
    last_pair->next = pair_alloc(p);
    last_pair = last_pair->next;
}

void discard_pairs()
{	
    COORDS *temp;

    while(first_pair != NULL) {
	temp = first_pair;
	first_pair = first_pair->next;
	free((char *)temp);
    }
    last_pair = NULL;
}		

OPTS *opt_create()
{
    OPTS *tmp;
    tmp = (OPTS *) emalloc(sizeof(OPTS));
    
    /* set defaults */
    tmp->font_size = 10.0;       /* :F<font_size> */
    tmp->mirror = MIRROR_OFF;    /* :M<x,xy,y>    */
    tmp->rotation = 0.0;         /* :R<rotation,resolution> */
    tmp->width = 0.0;            /* :W<width> */
    tmp->aspect_ratio = 1.0;     /* :Y<yx_aspect_ratio> */
    tmp->scale=1.0;              /* :X<scale> */
    tmp->slant=0.0;              /* :Z<slant_degrees> */

    return(tmp);
}     


char *strsave(s)   /* save string s somewhere */
char *s;
{
    char *p;

    if ((p = (char *) emalloc(strlen(s)+1)) != NULL)
	strcpy(p,s);
    return(p);
}

/********************************************************/
/* rendering routine */
/* db_render() sets globals xmax, ymax, xmin, ymin;
/* db_bounds() accesses them for use in com_win();
/********************************************************/

void db_bounds(xxmin, yymin, xxmax, yymax)
NUM *xxmin;
NUM *yymin;
NUM *xxmax;
NUM *yymax;
{
    *xxmin = xmin;
    *yymin = ymin;
    *xxmax = xmax;
    *yymax = ymax;
    snapxy(xxmin, yymin);
    snapxy(xxmax, yymax);

}

void db_set_nest(nest) 
int nest;
{
    extern int nestlevel;
    nestlevel = nest;
}

int db_render(cell,xf, nest, mode)
DB_TAB *cell;
XFORM *xf; 	/* coordinate transform matrix */
int nest;	/* nesting level */
int mode; 	/* 0=regular rendering, 1=xor rubberband */
{
    DB_DEFLIST *p;
    OPTS *op;
    XFORM *xp, *xa;
    extern XFORM *transform;
    extern int nestlevel;
    double optval;
    int debug=0;
    double xminsave;
    double xmaxsave;
    double yminsave;
    double ymaxsave;

    if (debug) printf ("#rendering %s at level %d\n", cell->name, nest); 

    if (!X && (nest == 0)) {	/* autoplot output */
	printf("nogrid\n");
	printf("isotropic\n");
	printf("back\n");
    }

    transform = xf; 		/* set global transform */
    				/* gets used in draw() */

    if (nest == 0) {
        if (debug) printf("initializing global bounding box to 0\n");
	xmax=0.0;		/* initialize globals to 0,0 location*/
	xmin=0.0;		/* draw() routine will modify these */
	ymax=0.0;
	ymin=0.0;
    }

    if (debug) printf("bounds = %g %g %g %g\n", xmin, xmax, ymin, ymax);

    for (p=cell->dbhead; p!=(DB_DEFLIST *)0; p=p->next) {
	switch (p->type) {
        case ARC:  /* arc definition */
	    do_arc(p, mode);
	    break;
        case CIRC:  /* circle definition */
	    do_circ(p, mode);
	    break;
        case LINE:  /* line definition */
	    do_line(p, mode);
	    break;
        case NOTE:  /* note definition */
	    do_note(p, mode);
	    break;
        case OVAL:  /* oval definition */
	    do_oval(p, mode);
	    break;
        case POLY:  /* polygon definition */
	    do_poly(p, mode);
	    break;
	case RECT:  /* rectangle definition */
	    do_rect(p, mode);
	    break;
        case TEXT:  /* text definition */
	    do_text(p, mode);
	    break;
        case INST:  /* recursive instance call */

	    /* create a unit xform matrix */

	    xp = (XFORM *) emalloc(sizeof(XFORM));  
	    xp->r11 = 1.0;
	    xp->r12 = 0.0;
	    xp->r21 = 0.0;
	    xp->r22 = 1.0;
	    xp->dx  = 0.0;
	    xp->dy  = 0.0;

	    /* ADD [I] devicename [.cname] [:Mmirror] [:Rrot] [:Xscl]
	     *	   [:Yyxratio] [:Zslant] [:Snx ny xstep ystep]
	     *	   [{:F0 | :F1}romfile] coord   
	     */

	    /* create transformation matrix from options */

	    mat_rotate(xp, p->u.i->opts->rotation);
	    mat_scale(xp, p->u.i->opts->scale, p->u.i->opts->scale); 
	    mat_scale(xp, 1.0, p->u.i->opts->aspect_ratio); 
	    mat_slant(xp,  p->u.i->opts->slant); 
	    switch (p->u.i->opts->mirror) {
	    	case MIRROR_OFF:
		    break;
	    	case MIRROR_X:
		    xp->r22 *= -1.0;
		    break;
	    	case MIRROR_Y:
		    xp->r11 *= -1.0;
		    break;
	    	case MIRROR_XY:
		    xp->r11 *= -1.0;
		    xp->r22 *= -1.0;
		    break;
	    }

	    xp->dx += p->u.i->x;
	    xp->dy += p->u.i->y;

	    transform = compose(xp,xf);		/* set global transform */

            /* save old min/max values for currep */

	    xminsave=xmin;
	    xmaxsave=xmax;
	    yminsave=ymin;
	    ymaxsave=ymax;

	    /* find bounding box on screen for instance */

	    xmax=0.0;		/* initialize globals to 0,0 location*/
	    xmin=0.0;		/* draw() routine will modify these */
	    ymax=0.0;
	    ymin=0.0;

	    if (nest >= nestlevel) { 
		drawon = 0;
	    } else {
		drawon = 1;
	    }

	    /* render instance */
	    db_render(p->u.i->def, transform, nest+1, mode);
	    if (debug) printf("# in db_render at level %d\n", nest); 

	    /* don't draw anything below nestlevel */
	    if (nest > nestlevel) { 
		drawon = 0;
	    } else {
		drawon = 1;
	    }

	    /* if at nestlevel, draw bounding box */
	    if (nest == nestlevel) { 
		set_pen(0);
		jump();
		    /* a square bounding box outline in white */
		    draw(xmax,ymax,mode); draw(xmax,ymin,mode);
		    draw(xmin,ymin,mode); draw(xmin,ymax,mode);
		    draw(xmax,ymax,mode);
		jump();
		    /* and diagonal lines from corner to corner */
		    draw(xmax,ymax,mode);
		    draw(xmin,ymin,mode);
		jump();
		    draw(xmin,ymax,mode);
		    draw(xmax,ymin,mode);
	    }

	    free(transform); free(xp);
	    transform = xf;		/* set transform back */

	    /* now pass bounding box back */
	    xmin=min(xminsave, xmin+p->u.i->x);
	    xmax=max(xmaxsave, xmax+p->u.i->x);
	    ymin=min(yminsave, ymin+p->u.i->y);
	    ymax=max(ymaxsave, ymax+p->u.i->y);

	    break;
	default:
	    eprintf("unknown record type: %d in db_render\n", p->type);
	    exit(1);
	    break;
	}
    }


    snapxy(&xmin, &ymin);
    snapxy(&xmax, &ymax);

    if (debug) printf("setting bounds %g %g %g %g\n", xmin, xmax, ymin, ymax);
    
    cell->minx = xmin;
    cell->maxx = xmax;
    cell->miny = ymin;
    cell->maxy = ymax;
}


/******************** plot geometries *********************/

/* ADD Emask [.cname] [@sname] [:Wwidth] [:Rres] coord coord coord */ 
void do_arc(def, mode)
DB_DEFLIST *def;
int mode;
{
    NUM x1,y1,x2,y2,x3,y3;

    /* printf("# rendering arc  (not implemented)\n"); */

    x1=def->u.a->x1;	/* #1 end point */
    y1=def->u.a->y1;

    x2=def->u.a->x2;	/* #2 end point */
    y2=def->u.a->y2;

    x3=def->u.a->x3;	/* point on curve */
    y3=def->u.a->y3;
}

/* ADD Cmask [.cname] [@sname] [:Wwidth] [:Rres] coord coord */
void do_circ(def,mode)
DB_DEFLIST *def;
int mode;		/* drawing mode */
{
    int i;
    double r,theta,x,y,x1,y1,x2,y2;

    /* FIXME: should eventually parse resolution */
    int res=64;	/* resolution */

    /* printf("# rendering circle\n"); */

    x1 = (double) def->u.c->x1; 	/* center point */
    y1 = (double) def->u.c->y1;

    x2 = (double) def->u.c->x2;	/* point on circumference */
    y2 = (double) def->u.c->y2;
    
    jump();
    set_pen(def->u.c->layer);

    x = (double) (x1-x2);
    y = (double) (y1-y2);
    r = sqrt(x*x+y*y);
    for (i=0; i<=res; i++) {
	theta = ((double) i)*2.0*M_PI/((double) res);
	x = r*sin(theta);
	y = r*cos(theta);
	draw(x1+x, y1+y,mode);
    }
}

/* ADD Lmask [.cname] [@sname] [:Wwidth] coord coord [coord ...] */
  

void do_line(def,mode)
DB_DEFLIST *def;
int mode; 	/* drawing mode */
{
    COORDS *temp;
    OPTS *op;

    double x1,y1,x2,y2,x3,y3,dx,dy,d,a;
    double xa,ya,xb,yb,xc,yc,xd,yd;
    double k,dxn,dyn;
    double width=0.0;
    int segment;
    int debug=0;	/* set to 1 for copious debug output */

    width = def->u.l->opts->width;

    /* there are four cases for rendering lines with miters 
     *
     * "===" is rendered segment, "|" is a square end,
     * "/" is a mitered end, "----" is a non-rendered segment.
     * 
     * 1) single segment: 
     *      (temp->next == NULL)
     * 
     *	    |=====|
     *
     * 2) first segment, but more than one
     *      (temp->next != NULL)
     *
     *	    |=====/-----/...
     *
     * 3) middle segment bracketed by active segments:
     *      (temp->next != NULL)
     *
     *      -----/======/-----
     *
     * 4) final segment, preceeded by an active segment:
     *      (temp->next == NULL)
     *      -----/======|
     */

    /* printf("# rendering line\n"); */
    set_pen(def->u.l->layer);

    temp = def->u.l->coords;
    x2=temp->coord.x;
    y2=temp->coord.y;
    temp = temp->next;

    segment = 0;
    do {
	x1=x2; x2=temp->coord.x;
	y1=y2; y2=temp->coord.y;

	if (width != 0) {
	    if (segment == 0) {
		dx = x2-x1; 
		dy = y2-y1;
		a = 1.0/(2.0*sqrt(dx*dx+dy*dy));
		if (debug) printf("# in 1, a=%g\n",a);
		dx *= a; 
		dy *= a;
	    } else {
		if (debug) printf("# in 2\n"); 
		dx=dxn;
		dy=dyn;
	    }

	    if ( temp->next != NULL ) {
		x3=temp->next->coord.x;
		y3=temp->next->coord.y;
		dxn = x3-x2; 
		dyn = y3-y2;

		/* prevent a singularity if the last */
		/* rubber band segment has zero length */
		if (dxn==0 && dyn==0) {
		    a = 0;
		} else {
		    a = 1.0/(2.0*sqrt(dxn*dxn+dyn*dyn));
		}

		if (debug) printf("# in 3, a=%g\n",a); 
		dxn *= a; 
		dyn *= a;
	    }
	    
	    if ( temp->next == NULL && segment == 0) {

		if (debug) printf("# in 4\n"); 
		xa = x1+dy*width; ya = y1-dx*width;
		xb = x2+dy*width; yb = y2-dx*width;
		xc = x2-dy*width; yc = y2+dx*width;
		xd = x1-dy*width; yd = y1+dx*width;

	    } else if (temp->next != NULL && segment == 0) {  

		if (debug) printf("# in 5\n"); 
		xa = x1+dy*width; ya = y1-dx*width;
		xd = x1-dy*width; yd = y1+dx*width;

		if (fabs(dx+dxn) < EPS) {
		    k = (dx - dxn)/(dyn + dy); 
		    if (debug) printf("# in 6, k=%g\n",k);
		} else {
		    k = (dyn - dy)/(dx + dxn);
		    if (debug) printf("# in 7, k=%g\n",k);
		}

		/* check for co-linearity of segments */
		if ((fabs(dx-dxn)<EPS && fabs(dy-dyn)<EPS) ||
		    (fabs(dx+dxn)<EPS && fabs(dy+dyn)<EPS)) {
		    if (debug) printf("# in 8\n"); 
		    xb = x2+dy*width; yb = y2-dx*width;
		    xc = x2-dy*width; yc = y2+dx*width;
		} else { 
		    if (debug) printf("# in 19\n");
		    xb = x2+dy*width + k*dx*width;
		    yb = y2-dx*width + k*dy*width;
		    xc = x2-dy*width - k*dx*width; 
		    yc = y2+dx*width - k*dy*width;
		}

	    } else if ((temp->next == NULL && segment >= 0) ||
	              ( temp->next->coord.x == x2 && 
		        temp->next->coord.y == y2)) {

		if (debug) printf("# in 10\n");
		xb = x2+dy*width; yb = y2-dx*width;
		xc = x2-dy*width; yc = y2+dx*width;

	    } else if (temp->next != NULL && segment >= 0) {  

		if (debug) printf("# in 11\n");
		if (fabs(dx+dxn) < EPS) {
		    k = (dx - dxn)/(dyn + dy); 
		    if (debug) printf("# in 12, k=%g\n",k);
		} else {
		    if (debug) printf("# in 13, k=%g\n",k); 
		    k = (dyn - dy)/(dx + dxn);
		}

		/* check for co-linearity of segments */
		if ((fabs(dx-dxn)<EPS && fabs(dy-dyn)<EPS) ||
		    (fabs(dx+dxn)<EPS && fabs(dy+dyn)<EPS)) {
		    if (debug) printf("# in 8\n"); 
		    xb = x2+dy*width; yb = y2-dx*width;
		    xc = x2-dy*width; yc = y2+dx*width;
		} else { 
		    if (debug) printf("# in 9\n"); 
		    xb = x2+dy*width + k*dx*width;
		    yb = y2-dx*width + k*dy*width;
		    xc = x2-dy*width - k*dx*width; 
		    yc = y2+dx*width - k*dy*width;
		}
	    }

	    jump();
	
	    draw(xa, ya,mode);
	    draw(xb, yb,mode);

	    if( width != 0) {
		draw(xc, yc,mode);
		draw(xd, yd,mode);
		draw(xa, ya,mode);
	    }

	    /* printf("#dx=%g dy=%g dxn=%g dyn=%g\n",dx,dy,dxn,dyn); */
	    /* check for co-linear reversal of path */
	    if ((fabs(dx+dxn)<EPS && fabs(dy+dyn)<EPS)) {
		if (debug) printf("# in 20\n"); 
		xa = xc; ya = yc;
		xd = xb; yd = yb;
	    } else {
		if (debug) printf("# in 21\n"); 
		xa = xb; ya = yb;
		xd = xc; yd = yc;
	    }
	} else {		/* width == 0 */
	    jump();
	    draw(x1,y1,mode);
	    draw(x2,y2,mode);
	    jump();
	}

	temp = temp->next;
	segment++;

    } while(temp != NULL);
}


/* ADD Nmask [.cname] [:Mmirror] [:Rrot] [:Yyxratio] [:Zslant] 
[:Ffontsize] "string" coord    */

void do_note(def,mode)
DB_DEFLIST *def;
int mode;
{

    extern XFORM *transform;
    XFORM *xp, *xf;
    NUM x,y;
    OPTS  *op;
    double optval;

    /* create a unit xform matrix */
    xp = (XFORM *) emalloc(sizeof(XFORM));  
    xp->r11 = 1.0; xp->r12 = 0.0;
    xp->r21 = 0.0; xp->r22 = 1.0;
    xp->dx  = 0.0; xp->dy  = 0.0;

    /* NOTE: To work properly, these transformations have to */
    /* occur in the proper order, for example, rotation must come */
    /* after slant transformation or else it wont work properly */

    mat_scale(xp, def->u.n->opts->font_size, def->u.n->opts->font_size);
    mat_scale(xp, 1.0, def->u.n->opts->aspect_ratio);
    mat_slant(xp, def->u.n->opts->slant);
    mat_rotate(xp, def->u.n->opts->rotation);
    switch (def->u.n->opts->mirror) {
	case MIRROR_OFF:
	    break;
	case MIRROR_X:
	    xp->r22 *= -1.0;
	    break;
	case MIRROR_Y:
	    xp->r11 *= -1.0;
	    break;
	case MIRROR_XY:
	    xp->r11 *= -1.0;
	    xp->r22 *= -1.0;
	    break;
    }

    jump(); set_pen(def->u.n->layer);

    xp->dx += def->u.n->x;
    xp->dy += def->u.n->y;

    writestring(def->u.n->text, xp);

    free(xp);
}


/* ADD Omask [.cname] [@sname] [:Wwidth] [:Rres] coord coord coord   */

void do_oval(def, mode)
DB_DEFLIST *def;
int mode;
{
    NUM x1,y1,x2,y2,x3,y3;

    /* printf("# rendering oval (not implemented)\n"); */

    x1=def->u.o->x1;	/* focii #1 */
    y1=def->u.o->y1;

    x2=def->u.o->x2;	/* focii #2 */
    y2=def->u.o->y2;

    x3=def->u.o->x3;	/* point on curve */
    y3=def->u.o->y3;

}

/* ADD Pmask [.cname] [@sname] [:Wwidth] coord coord coord [coord ...]  */ 

void do_poly(def, mode)
DB_DEFLIST *def;
int mode;
{
    COORDS *temp;
    
    /* printf("# rendering polygon\n"); */
    
    jump();
    set_pen(def->u.p->layer);

    temp = def->u.p->coords;
    
    /* FIXME: should eventually check for closure here and probably */
    /* at initial entry of points into db.  If last point != first */
    /* point, then the polygon should be closed automatically and an */
    /* error message generated */

    while(temp != NULL) {
	    draw(temp->coord.x, temp->coord.y, mode);
	    temp = temp->next;
    }
}


/* ADD Rmask [.cname] [@sname] [:Wwidth] coord coord  */

void do_rect(def, mode)
DB_DEFLIST *def;
int mode;	/* drawing mode */
{
    double x1,y1,x2,y2;

    /* printf("# rendering rectangle\n"); */

    x1 = (double) def->u.r->x1;
    y1 = (double) def->u.r->y1;
    x2 = (double) def->u.r->x2;
    y2 = (double) def->u.r->y2;
    
    jump();
    set_pen(def->u.c->layer);

    draw(x1, y1, mode); draw(x1, y2, mode);
    draw(x2, y2, mode); draw(x2, y1, mode);
    draw(x1, y1, mode);
}



/* ADD Tmask [.cname] [:Mmirror] [:Rrot] [:Yyxratio]
 *             [:Zslant] [:Ffontsize] "string" coord  
 */

void do_text(def, mode)
DB_DEFLIST *def;
int mode;
{

    extern XFORM *transform;
    XFORM *xp, *xf;
    NUM x,y;
    OPTS  *op;
    double optval;

    /* printf("# rendering text as a note (not fully implemented)\n"); */

    /* create a unit xform matrix */

    xp = (XFORM *) emalloc(sizeof(XFORM));  
    xp->r11 = 1.0;
    xp->r12 = 0.0;
    xp->r21 = 0.0;
    xp->r22 = 1.0;
    xp->dx  = 0.0;
    xp->dy  = 0.0;

    /* NOTE: To work properly, these transformations have to */
    /* occur in the proper order, for example, rotation must come */
    /* after slant transformation or else it wont work properly */

    mat_scale(xp, def->u.n->opts->font_size, def->u.n->opts->font_size);
    mat_scale(xp, 1.0, def->u.n->opts->aspect_ratio);
    mat_slant(xp, def->u.n->opts->slant);
    mat_rotate(xp, def->u.n->opts->rotation);
    switch (def->u.n->opts->mirror) {
	case MIRROR_OFF:
	    break;
	case MIRROR_X:
	    xp->r22 *= -1.0;
	    break;
	case MIRROR_Y:
	    xp->r11 *= -1.0;
	    break;
	case MIRROR_XY:
	    xp->r11 *= -1.0;
	    xp->r22 *= -1.0;
	    break;
    }

    jump(); set_pen(def->u.t->layer);

    xp->dx += def->u.t->x;
    xp->dy += def->u.t->y;

    writestring(def->u.t->text, xp);

    free(xp);
}

/***************** coordinate transformation utilities ************/

XFORM *compose(xf1, xf2)
XFORM *xf1, *xf2;
{
     XFORM *xp;
     xp = (XFORM *) emalloc(sizeof(XFORM)); 

     xp->r11 = (xf1->r11 * xf2->r11) + (xf1->r12 * xf2->r21);
     xp->r12 = (xf1->r11 * xf2->r12) + (xf1->r12 * xf2->r22);
     xp->r21 = (xf1->r21 * xf2->r11) + (xf1->r22 * xf2->r21);
     xp->r22 = (xf1->r21 * xf2->r12) + (xf1->r22 * xf2->r22);
     xp->dx  = (xf1->dx  * xf2->r11) + (xf1->dy  * xf2->r21) + xf2->dx;
     xp->dy  = (xf1->dx  * xf2->r12) + (xf1->dy  * xf2->r22) + xf2->dy;

     return (xp);
}

/* in-place rotate a transform matrix by theta degrees */
void mat_rotate(xp, theta)
XFORM *xp;
double theta;
{
    double s,c,t;
    s=sin(2.0*M_PI*theta/360.0);
    c=cos(2.0*M_PI*theta/360.0);

    t = c*xp->r11 - s*xp->r12;
    xp->r12 = c*xp->r12 + s*xp->r11;
    xp->r11 = t;

    t = c*xp->r21 - s*xp->r22;
    xp->r22 = c*xp->r22 + s*xp->r21;
    xp->r21 = t;

    t = c*xp->dx - s*xp->dx;
    xp->dy = c*xp->dy + s*xp->dx;
    xp->dx = t;
}

/* in-place scale transform */
void mat_scale(xp, sx, sy) 
XFORM *xp;
double sx, sy;
{
    double t;

    xp->r11 *= sx;
    xp->r12 *= sy;
    xp->r21 *= sx;
    xp->r22 *= sy;
    xp->dx  *= sx;
    xp->dy  *= sy;
}

/* in-place slant transform (for italics) */
void mat_slant(xp, theta) 
XFORM *xp;
double theta;
{
    double s,c,t;
    s=sin(2.0*M_PI*theta/360.0);
    c=cos(2.0*M_PI*theta/360.0);

    xp->r21 += s/c;
}

void printmat(xa)
XFORM *xa;
{
     printf("\n");
     printf("\t%g\t%g\t%g\n", xa->r11, xa->r12, 0.0);
     printf("\t%g\t%g\t%g\n", xa->r21, xa->r22, 0.0);
     printf("\t%g\t%g\t%g\n", xa->dx, xa->dy, 1.0);
}   

/***************** low-level X, autoplot primitives ******************/

/* The "nseg" variable is used to convert draw(x,y) calls into 
 * x1,y1,x2,y2 line segments.  The rule is: jump() sets nseg to zero, and
 * draw() calls XDrawLine with the xold,yold,x,y only if nseg>0.  "nseg" is
 * declared global because it must be shared by both draw() and jump().
 * An alternative would have been to make all lines with a function like
 * drawline(x1,y1,x2,y2), but that would have been nearly 2x more 
 * inefficient when  drawing lines with more than two nodes (all shared nodes
 * get sent twice).  By using jump() and sending the points serially, we
 * have the option of building a linked list and using XDrawLines() rather
 * than XDrawLine().  In addition, the draw()/jump() paradigm is used by
 * autoplot, so has a long history.
 */

static int nseg=0;


void draw(x, y, mode)	/* draw x,y transformed by extern global xf */
NUM x,y;			/* location in real coordinate space */
int mode;
{
    extern XFORM *transform;	/* global for efficiency */
    NUM xx, yy;
    static NUM xxold, yyold;

    /* globals for computing bounding boxes */
    /* NOTE: This must be done prior to conversion into screen space */
    xmax = max(x,xmax);
    xmin = min(x,xmin);
    ymax = max(y,ymax);
    ymin = min(y,ymin);

    /* now compute coordinates in screen space */
    xx = x*transform->r11 + y*transform->r21 + transform->dx;
    yy = x*transform->r12 + y*transform->r22 + transform->dy;

    if (X) {
	if (nseg) {
	    if (mode == 0) { 	/* regular drawing */
		if (drawon) xwin_draw_line((int) xxold, (int) yyold, (int) xx, (int) yy);
	    } else {		/* rubber band drawing */
		if (drawon) xwin_xor_line((int) xxold, (int) yyold, (int) xx, (int) yy);
	    }
	}
	xxold=xx;
	yyold=yy;
	nseg++;
    } else {
	if (drawon) printf("%4.6g %4.6g\n",xx,yy);	/* autoplot output */
    }
}

void jump(void) 
{
    if (X) {
	nseg=0;  		/* for X */
    } else {
	if (drawon) printf("jump\n");  	/* autoplot */
    }
}

void set_pen(pnum)
int pnum;
{
    if (X) {
	xwin_set_pen((pnum%7));		/* for X */
    } else {
	if (drawon) printf("pen %d\n", (pnum%7));	/* autoplot */
    }
    set_line((((int)(pnum/7))%5));
}

void set_line(lnum)
int lnum;
{
    if (X) {
	xwin_set_line((lnum%5)+1);		/* for X */
    } else {
	if (drawon) printf("line %d\n", (lnum%5)+1);	/* autoplot */
    }
}

double max(a,b) 
double a, b;
{
    return(a>=b?a:b);
}

double min(a,b) 
double a, b;
{
    return(a<=b?a:b);
}

