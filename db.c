
#include "db.h"
#include <stdio.h>
#include <math.h>
#include "y.tab.h"

/* master symbol table pointers */
static struct db_tab *HEAD = 0;
static struct db_tab *TAIL = 0;

COORDS *first_pair, *last_pair; 
OPTS   *first_opt, *last_opt;

/* routines for expanding db entries into vectors */
void do_arc(),  do_circ(), do_line(), do_note();
void do_oval(), do_poly(), do_rect(), do_text();

/* matrix routines */
XFORM *compose();
void rotate();
void scale();

/* primitives for outputting autoplot commands */
void draw();
void jump();
void set_pen();

/********************************************************/

DB_TAB *db_lookup(s)           /* find s in db */
char *s;
{
    struct db_tab *sp;

    for (sp=HEAD; sp!=(struct db_tab *)0; sp=sp->next)
        if (strcmp(sp->name, s)==0)
            return sp;
    return 0;               	/* 0 ==> not found */
} 

DB_TAB *db_install(s)		/* install s in db */
char *s;
{
    struct db_tab *sp;
    char *emalloc();

    /* this is written to ensure that a db_print is */
    /* not in reversed order...  New structures are */
    /* added at the end of the list using TAIL      */

    sp = (struct db_tab *) emalloc(sizeof(struct db_tab));
    sp->name = emalloc(strlen(s)+1); /* +1 for '\0' */
    strcpy(sp->name, s);

    sp->next   = (struct db_tab *) 0; 
    sp->dbhead = (struct db_deflist *) 0; 
    sp->dbtail = (struct db_deflist *) 0; 

    if (HEAD ==(struct db_tab *) 0) {	/* first element */
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

int db_print()           	/* print db */
{
    struct db_tab *sp;

    for (sp=HEAD; sp!=(struct db_tab *)0; sp=sp->next) {
        printf("edit ");
        printf("%s;\n",sp->name);
	if (sp->dbhead != (struct db_deflist *) 0) {
	    db_def_print(sp->dbhead); 
	}
        printf("save;\n");
        printf("exit;\n\n");
    }
    return 0;
}

int db_def_print(dp) 
DB_DEFLIST *dp;
{
    COORDS *coords;
    DB_DEFLIST *p; 
    for (p=dp; p!=(struct db_deflist *)0; p=p->next) {
	switch (p->type) {
        case ARC:  /* arc definition */
	    printf("#add ARC not inplemented yet\n");
	    break;
        case CIRC:  /* circle definition */
	    printf("add c%d %g,%g %g,%g;\n",
		p->u.c->layer,
		p->u.c->x1,
		p->u.c->y1,
		p->u.c->x2,
		p->u.c->y2
	    );
	    break;
        case LINE:  /* line definition */

	    printf("add l%d", p->u.l->layer);

	    coords = p->u.l->coords;
	    while(coords != NULL) {
		printf(" %g,%g", coords->coord.x, coords->coord.y);
		coords = coords->next;
	    }
	    printf(";\n");

	    break;
        case NOTE:  /* note definition */
	    printf("#add NOTE not inplemented yet\n");
	    break;
        case OVAL:  /* oval definition */
	    printf("#add OVAL not inplemented yet\n");
	    break;
        case POLY:  /* polygon definition */
	    printf("add p%d", p->u.p->layer);

	    coords = p->u.p->coords;
	    while(coords != NULL) {
		printf(" %g,%g", coords->coord.x, coords->coord.y);
		coords = coords->next;
	    }
	    printf(";\n");

	    break;
	case RECT:  /* rectangle definition */
	    printf("add r%d %g,%g %g,%g;\n",
		p->u.r->layer,
		p->u.r->x1,
		p->u.r->y1,
		p->u.r->x2,
		p->u.r->y2
	    );
	    break;
        case TEXT:  /* text definition */
	    printf("#add TEXT not inplemented yet\n");
	    break;
        case INST:  /* instance call */
	    printf("add %s ", p->u.i->def->name);
	    db_print_opts(p->u.i->opts);
	    printf("%g,%g;\n",
		p->u.i->x,
		p->u.i->y
	    );
	    break;
	default:
	    fprintf(stderr, "unknown record type in db_def_print\n");
	    exit(1);
	    break;
	}
    }
}

int db_print_opts(opts)           	/* print options */
OPTS *opts;
{
    OPTS  *op;

    for (op=opts; op!=(OPTS *)0; op=op->next) {
        printf("%s ",op->optstring);
    }
    return 0;
}

int db_add_arc(cell, layer, x1,y1,x2,y2,x3,y3) 
struct db_tab *cell;
int layer;
NUM x1,y1,x2,y2,x3,y3;
{
    struct db_arc *ap;
    struct db_deflist *dp;
    char *emalloc();
 
    ap = (struct db_arc *) emalloc(sizeof(struct db_arc));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));

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
    ap->x1=x1;
    ap->y1=y1;
    ap->x2=x2;
    ap->y2=y2;
    ap->x3=x3;
    ap->y3=y3;

    return(0);
}

int db_add_circ(cell, layer,x1,y1,x2,y2)
struct db_tab *cell;
int layer;
NUM x1,y1,x2,y2;
{
    struct db_circ *cp;
    struct db_deflist *dp;
    char *emalloc();
 
    cp = (struct db_circ *) emalloc(sizeof(struct db_circ));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));

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
    cp->x1=x1;
    cp->y1=y1;
    cp->x2=x2;
    cp->y2=y2;

    return(0);
}

int db_add_line(cell, layer, coords)
struct db_tab *cell;
int layer;
COORDS *coords;
{
    struct db_line *lp;
    struct db_deflist *dp;
    char *emalloc();
 
    lp = (struct db_line *) emalloc(sizeof(struct db_line));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));

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
    lp->coords=coords;

    return(0);
}

int db_add_note(cell, layer, string ,x,y) 
struct db_tab *cell;
int layer;
char *string;
NUM x,y;
{
    struct db_note *np;
    struct db_deflist *dp;
    char *emalloc();
 
    np = (struct db_note *) emalloc(sizeof(struct db_note));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));

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
    np->x=x;
    np->y=y;

    return(0);
}

int db_add_oval(cell, layer, x1,y1,x2,y2,x3,y3) 
struct db_tab *cell;
int layer;
NUM x1,y1, x2,y2, x3,y3;
{
    struct db_oval *op;
    struct db_deflist *dp;
    char *emalloc();
 
    op = (struct db_oval *) emalloc(sizeof(struct db_oval));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));

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
    op->x1=x1;
    op->y1=y1;
    op->x2=x2;
    op->y2=y2;
    op->x3=x3;
    op->y3=y3;

    return(0);
}

int db_add_poly(cell, layer, coords)
struct db_tab *cell;
int layer;
COORDS *coords;
{
    struct db_poly *pp;
    struct db_deflist *dp;
    char *emalloc();
 
    pp = (struct db_poly *) emalloc(sizeof(struct db_poly));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));

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
    pp->coords=coords;

    return(0);
}

int db_add_rect(cell, layer, opts, x1,y1,x2,y2)
struct db_tab *cell;
int layer;
OPTS *opts;
NUM x1,y1,x2,y2;
{
    struct db_rect *rp;
    struct db_deflist *dp;
    char *emalloc();
 
    rp = (struct db_rect *) emalloc(sizeof(struct db_rect));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));

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

int db_add_text(cell, layer, string ,x,y) 
struct db_tab *cell;
int layer;
char *string;
NUM x,y;
{
    struct db_text *tp;
    struct db_deflist *dp;
    char *emalloc();
 
    tp = (struct db_text *) emalloc(sizeof(struct db_text));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));

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
    tp->x=x;
    tp->y=y;

    return(0);
}

int db_add_inst(cell, subcell, opts, x, y)
struct db_tab *cell;
struct db_tab *subcell;
OPTS *opts;
NUM x,y;
{
    struct db_inst *ip;
    struct db_deflist *dp;
    char *emalloc();
 
    ip = (struct db_inst *) emalloc(sizeof(struct db_inst));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));

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

char *emalloc(n)    /* check return from malloc */
unsigned n;
{
    char *p, *malloc();

    p = malloc(n);
    if (p == 0)
        fprintf(stderr, "out of memory", (char *) 0);
    return p;
}   

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
    temp = (COORDS *) malloc(sizeof(COORDS));
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

OPTS *opt_alloc(s)
char *s;
{
    OPTS *temp;
    temp = (OPTS *) malloc(sizeof(OPTS));
    temp->optstring = s;
    temp->next = NULL;
    return(temp);
}     

void append_opt(s)
char *s;
{
    last_opt->next = opt_alloc(s);
    last_opt = last_opt->next;
}

void discard_opts()
{
    OPTS *temp;

    while(first_opt != NULL) {
      temp = first_opt;
      first_opt = first_opt->next;
      free((char *)temp);
    }
    last_opt = NULL;
}             

char *strsave(s)   /* save string s somewhere */
char *s;
{
    char *p;

    if ((p = (char *) malloc(strlen(s)+1)) != NULL)
	strcpy(p,s);
    return(p);
}

/********************************************************/
/* rendering routines */
/********************************************************/

int db_render(cell,xf)
DB_TAB *cell;
XFORM *xf; 	/* coordinate transform matrix */
{
    DB_DEFLIST *p;
    XFORM *xp, *xa;
    p=cell->dbhead;

    printf("isotropic\n");
    printf("back\n");

    for (p=cell->dbhead; p!=(DB_DEFLIST *)0; p=p->next) {
	switch (p->type) {
        case ARC:  /* arc definition */
	    do_arc(p,xf);
	    break;
        case CIRC:  /* circle definition */
	    do_circ(p,xf);
	    break;
        case LINE:  /* line definition */
	    do_line(p,xf);
	    break;
        case NOTE:  /* note definition */
	    do_note(p,xf);
	    break;
        case OVAL:  /* oval definition */
	    do_oval(p,xf);
	    break;
        case POLY:  /* polygon definition */
	    do_poly(p,xf);
	    break;
	case RECT:  /* rectangle definition */
	    do_rect(p,xf);
	    break;
        case TEXT:  /* text definition */
	    do_text(p,xf);
	    break;
        case INST:  /* recursive instance call */

	    /* create a unit xform matrix */

	    xp = (XFORM *) malloc(sizeof(XFORM));  
	    xp->r11 = 1.0;
	    xp->r12 = 0.0;
	    xp->r21 = 0.0;
	    xp->r22 = 1.0;
	    xp->dx  = p->u.i->x;
	    xp->dy  = p->u.i->y;

	    xa = compose(xf,xp);

	    db_render(p->u.i->def, xa);
	    free(xa);
	    free(xp);

	    break;
	default:
	    fprintf(stderr, "unknown record type in db_def_print\n");
	    exit(1);
	    break;
	}
    }
}


/******************** plot geometries *********************/

void do_arc(def,xf)
DB_DEFLIST *def;
XFORM *xf;
{
    NUM x1,y1,x2,y2,x3,y3;

    printf("# rendering arc  (not implemented)\n");

    x1=def->u.a->x1;	/* #1 end point */
    y1=def->u.a->y1;

    x2=def->u.a->x2;	/* #2 end point */
    y2=def->u.a->y2;

    x3=def->u.a->x3;	/* point on curve */
    y3=def->u.a->y3;
}

void do_circ(def, xf)
DB_DEFLIST *def;
XFORM *xf;
{
    int i;
    double r,theta,x,y,x1,y1,x2,y2;

    printf("# rendering circle\n");

    x1 = (double) def->u.c->x1; 	/* center point */
    y1 = (double) def->u.c->y1;

    x2 = (double) def->u.c->x2;	/* point on circumference */
    y2 = (double) def->u.c->y2;
    
    jump();
    set_pen(def->u.c->layer);

    x = (double) (x1-x2);
    y = (double) (y1-y2);
    r = sqrt(x*x+y*y);
    for (i=0; i<=16; i++) {
	theta = ((double) i)*2.0*3.1415926/16.0;
	x = r*sin(theta);
	y = r*cos(theta);
	draw(x1+x, y1+y, xf);
    }
}

void do_line(def, xf)
DB_DEFLIST *def;
XFORM *xf;
{
    COORDS *temp;

    printf("# rendering line\n");
    
    jump();
    set_pen(def->u.l->layer);


    temp = def->u.l->coords;
    while(temp != NULL) {
	    draw(temp->coord.x, temp->coord.y, xf);
	    temp = temp->next;
    }
}

void do_note(def,xf)
DB_DEFLIST *def;
XFORM *xf;
{
    printf("# rendering note (not implemented)\n");
}

void do_oval(def,xf)
DB_DEFLIST *def;
XFORM *xf;
{
    NUM x1,y1,x2,y2,x3,y3;

    printf("# rendering oval (not implemented)\n");

    x1=def->u.o->x1;	/* focii #1 */
    y1=def->u.o->y1;

    x2=def->u.o->x2;	/* focii #2 */
    y2=def->u.o->y2;

    x3=def->u.o->x3;	/* point on curve */
    y3=def->u.o->y3;

}

void do_poly(def, xf)
DB_DEFLIST *def;
XFORM *xf;
{
    COORDS *temp;
    
    printf("# rendering polygon\n");
    
    jump();
    set_pen(def->u.p->layer);

    temp = def->u.p->coords;
    /* should eventually check for closure */
    while(temp != NULL) {
	    draw(temp->coord.x, temp->coord.y, xf);
	    temp = temp->next;
    }
}

void do_rect(def, xf)
DB_DEFLIST *def;
XFORM *xf;
{
    double x1,y1,x2,y2;

    printf("# rendering rectangle\n");

    x1 = (double) def->u.r->x1;
    y1 = (double) def->u.r->y1;
    x2 = (double) def->u.r->x2;
    y2 = (double) def->u.r->y2;
    
    jump();
    set_pen(def->u.c->layer);

    draw(x1, y1, xf); draw(x1, y2, xf);
    draw(x2, y2, xf); draw(x2, y1, xf);
    draw(x1, y1, xf);
}

void do_text(def,xf)
DB_DEFLIST *def;
XFORM *xf;
{
    printf("# rendering text (not implemented)\n");
}

/***************** coordinate transformation utilities ************/

XFORM *compose(xf1, xf2)
XFORM *xf1, *xf2;
{
     XFORM *xp;
     xp = (XFORM *) malloc(sizeof(XFORM)); 

     xp->r11 = (xf1->r11 * xf2->r11) + (xf1->r12 * xf2->r21);
     xp->r12 = (xf1->r11 * xf2->r12) + (xf1->r12 * xf2->r22);
     xp->r21 = (xf1->r21 * xf2->r11) + (xf1->r22 * xf2->r21);
     xp->r22 = (xf1->r21 * xf2->r12) + (xf1->r22 * xf2->r22);
     xp->dx  = (xf1->dx  * xf2->r11) + (xf1->dy  * xf2->r21) + xf2->dx;
     xp->dy  = (xf1->dx  * xf2->r12) + (xf1->dy  * xf2->r22) + xf2->dy;

     return (xp);
}

/* in-place rotate a transform matrix by theta degrees */
void rotate(xp, theta)
XFORM *xp;
double theta;
{
    double s,c,t;
    s=sin(2.0*M_PI*theta/360.0);
    c=cos(2.0*M_PI*theta/360.0);

    t = c*xp->r11 + s*xp->r12;
    xp->r12 = c*xp->r12 - s*xp->r11;
    xp->r11 = t;

    t = c*xp->r21 + s*xp->r22;
    xp->r22 = c*xp->r22 - s*xp->r21;
    xp->r21 = t;

    t = c*xp->dx + s*xp->dx;
    xp->dy = c*xp->dy - s*xp->dx;
    xp->dx = t;
}

void printmat(xa)
XFORM *xa;
{
     printf("\n");
     printf("\t%g\t%g\t%g\n", xa->r11, xa->r12, 0.0);
     printf("\t%g\t%g\t%g\n", xa->r21, xa->r22, 0.0);
     printf("\t%g\t%g\t%g\n", xa->dx, xa->dy, 1.0);
}   

/* in-place scale transform */
void scale(xp, sx, sy) 
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

/***************** low level autoplot primitives ******************/

    /* 
     * time pig < ../../regress/cdrcfr3_I   > /dev/null
     * 7.76s real    7.56s user    0.02s system  
    */

void draw(x, y, xf)	/* draw x,y transformed by xf */
NUM x,y;
XFORM *xf;
{

    NUM xx, yy;

    xx = x*xf->r11 + y*xf->r21 + xf->dx;
    yy = x*xf->r12 + y*xf->r22 + xf->dy;

    printf("%4.6f %4.6f\n",xx,yy);
}

void xdraw(x, y, xf)	/* draw x,y transformed by xf */
NUM x,y;
XFORM *xf;
{

    NUM xx, yy;
    if (xf->r11 == 1.0 && 
        xf->r12 == 0.0 && 
	xf->r21 == 0.0 &&  
	xf->r22 == 1.0) 
    {
/*	printf("using simple xform\n");
	printmat(xf); */
	xx += xf->dx;
	yy += xf->dy;
    } else {
/*	printf("using complex xform\n");
	printmat(xf); */
	xx = x*xf->r11 + y*xf->r21 + xf->dx;
	yy = x*xf->r12 + y*xf->r22 + xf->dy;
    }
    printf("%4.6f %4.6f\n",xx,yy);
}

void jump() 
{
    printf("jump\n");
}

void set_pen(lnum)
int lnum;
{
    printf("pen %d\n", (lnum%5)+1);
}
