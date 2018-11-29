#include <math.h>
#include <X11/Xlib.h>
#include <string.h>

#include "db.h"
#include "rubber.h"
#include "xwin.h"
#include "postscript.h"
#include "eprintf.h"
#include "equate.h"
#include "ev.h"
#include "readfont.h"

#define FUZZBAND 0.01	/* how big the fuzz around lines is as */
                        /* a fraction of minimum window dimension */
			/* for the purpose of picking objects */

/* global variables that need to eventually be on a stack for nested edits */
static int drawon=1;		  /* 0 = dont draw, 1 = draw (used in nesting)*/
static int showon=1;		  /* 0 = layer currently turned off */

// static int nestlevel=9;		  // nesting level
// int physicalnest=1;	  	  // 1=physical (WIN:N), 0=logical (WIN:L)

static int draw_fill=FILL_OFF;
static int layer_fill=FILL_OFF;
static int X=1;		  /* 1 = draw to X, 0 = emit autoplot commands */
FILE *PLOT_FD;		  /* file descriptor for plotting */

int filled_object = 0;		/* global for managing polygon filling */
int n_poly_points = 0;		/* number of points in filled polygon */

#define MAXPOLY 1024		/* maximum polygon size */
XPoint Poly[MAXPOLY];

			
void clipl();   	  /* polygon clipping pipeline: clip left side */
void clipr();   	  /* clip right side */
void clipt();		  /* clip top */
void clipb();		  /* clip bottom */
void clip_setwindow();	  /* set clipping window */

static double xmin;
static double xmax;
static double ymin;
static double ymax;


int pickcheck();
void emit();
static int debug = 0;

/****************************************************/
/* rendering routines				    */
/* db_render() sets globals xmax, ymax, xmin, ymin; */
/****************************************************/

void db_set_fill(int fill) 
{
    extern int draw_fill;
    if (fill == FILL_TOGGLE) {
	if (draw_fill == FILL_ON) {
	    draw_fill = FILL_OFF;
	} else {
	    draw_fill = FILL_ON;
	}
    } else  if (fill == FILL_ON || fill == FILL_OFF) {
	draw_fill = fill;
    } else {
	printf("bad fill argument in db_set_fill() %d\n", fill);
	exit(1);
    }
}

void db_set_physical(int physical) 	// 1=physical (WIN:N), 0=logical (WIN:L)
{
    if (currep != NULL) {
	currep->physical = physical;
    }
}

void db_set_nest(int nest) 
{
    if (currep != NULL) {
	currep->nestlevel = nest;
    }
}

void db_bounds_update(BOUNDS *mybb, BOUNDS *childbb) 
{

    if (mybb->init==0) {
    	mybb->xmin = childbb->xmin;
    	mybb->ymin = childbb->ymin;
    	mybb->xmax = childbb->xmax;
    	mybb->ymax = childbb->ymax;
	mybb->init++;
    } else if (childbb->init != 0 && mybb->init != 0) {
        /* pass bounding box back */
	mybb->xmin=min(mybb->xmin, childbb->xmin);
	mybb->ymin=min(mybb->ymin, childbb->ymin);
	mybb->xmax=max(mybb->xmax, childbb->xmax);
	mybb->ymax=max(mybb->ymax, childbb->ymax);
    }
    return;
}

/* a fast approximation to euclidian distance */
/* with no sqrt() and about 5% accuracy */

double dist(double dx,double dy) 
{
    dx = dx<0?-dx:dx; /* absolute value */
    dy = dy<0?-dy:dy;

    if (dx > dy) {
	return(dx + 0.3333*dy);
    } else {
	return(dy + 0.3333*dx);
    }

    // if you want more accuracy 
    // at the cost of another multiply use
    // 0.9615dx + 0.39*dy
}


/* called with pick point x,y and bounding box in p return -1 if xy */
/* inside bb, otherwise return a number  between zero and 1 */
/* monotonically related to inverse distance of xy from bounding box */

double bb_distance(double x, double y, DB_DEFLIST *p, double fuzz)
{
    int outcode;
    double retval;
    double tmp;
    outcode = 0;

    /*  9 8 10
	1 0 2
	5 4 6 */

    if (p->xmin > p->xmax) { 
	tmp = p->xmax;
	p->xmax = p->xmin;
	p->xmin = tmp;
    
    }
    if (p->ymin > p->ymax) {
	tmp = p->ymax;
	p->ymax = p->ymin;
	p->ymin = tmp;
    }

    if (x <= p->xmin-fuzz)  outcode += 1; 
    if (x >  p->xmax+fuzz)  outcode += 2;
    if (y <= p->ymin-fuzz)  outcode += 4;
    if (y >  p->ymax+fuzz)  outcode += 8;

    switch (outcode) {
	case 0:		/* p is inside BB */
	    retval = -1.0; 	
	    break;
	case 1:		/* p is West */
	    retval = 1.0/(1.0 + (p->xmin-x));
	    break;
	case 2:		/* p is East */
	    retval = 1.0/(1.0 + (x-p->xmax));
	    break;
	case 4:		/* p is South */
	    retval = 1.0/(1.0 + (p->ymin-y));	
	    break;
	case 5:		/* p is SouthWest */
	    retval = 1.0/(1.0 + dist(p->ymin-y,p->xmin-x));
	    break;
	case 6:		/* p is SouthEast */
	    retval = 1.0/(1.0 + dist(p->ymin-y,x-p->xmax));
	    break;
	case 8:		/* p is North */
	    retval = 1.0/(1.0 + (y-p->ymax));	
	    break;
	case 9:		/* p is NorthWest */
	    retval = 1.0/(1.0 + dist(y-p->ymax, p->xmin-x));
	    break;
	case 10:	/* p is NorthEast */
	    retval = 1.0/(1.0 + dist(y-p->ymax, x-p->xmax));
	    break;
	default:
	    printf("bad case in bb_distance %d\n", outcode);
	    exit(1);
	    break;
    }
    /* printf("%.5g %.5g %.5g %.5g %.5g %d\n",
    	p->xmin, p->ymin, p->xmax, p->ymax, retval, outcode); */
    return (retval);
}


/* 
 * db_ident() takes a pick point (x,y) and renders through 
 * the database to find the best candidate near the point.
 *
 * Run through all components in the device and
 * give each a score.  The component with the best score is 
 * returned as the final pick.
 * 
 * three different scores:
 *    CASE 1: ; pick point is outside of BoundingBox (BB), 
 *      higher score the closer to BB
 *	(0<  score < 1)	
 *	score is 1/(1+distance to cell BB)
 *    CASE 2: ; pick inside of BB, doesn't touch any rendered lines
 *	(1<= score < 2)
 *	score is 1+1/(1+min_dimension_of cell BB)
 *	score is weighted so that smaller BBs get better score
 *	This allows picking the inner of nested rectangles
 *    CASE 3: ; pick inside of BB, touches a rendered line
 *	(2<= score < 3)
 *	score is 2+1/(1+min_dimension_of line BB)
 *
 *    CASE 1 serves as a crude selector in the case that the
 *    pick point is not inside any bounding box.  It also serves
 *    as a crude screening to avoid having to test all the components
 *    in detail. 
 * 
 *    CASE 2 is for picks that fall inside a bounding box. It has a
 *    higher score than anything in CASE 1 so it will take priority
 *    Smaller bounding boxes get higher ranking.
 *
 *    CASE 3 overrides CASE 1 and CASE 2.  It comes into play when
 *    the pick point is near an actual drawn segment of a component.
 *
 */

DB_DEFLIST *db_ident(
    DB_TAB *cell,		/* device being edited */
    double x, double y,	 	/* pick point */
    int mode, 			/* 0=ident (any visible), 1=pick (pick restricted to modifiable objects) */
    int pick_layer,		/* layer restriction, or 0=all */
    int comp,			/* comp restriction */
    char *name			/* instance name restrict or NULL */
)
{
    DB_DEFLIST *p;
    DB_DEFLIST *p_best = NULL;	/* gets returned */
    int debug=0;
    BOUNDS childbb;
    double pick_score=0.0;
    double pick_best=0.0;
    double fuzz;
    int layer;
    int bad_comp;

    if (cell == NULL) {
    	printf("no cell currently being edited!\n");
	return(0);
    }

    if (mode && debug) {		/* pick rather than ident mode */
    	printf("db_ident: called with layer %d, comp %d, name %s\n", 
		pick_layer, comp, name);
    }

    /* pick fuzz should be a fraction of total window size */
    
    fuzz = max((cell->maxx-cell->minx),(cell->maxy-cell->miny))*FUZZBAND;

    for (p=cell->dbhead; p!=(DB_DEFLIST *)0; p=p->next) {

	childbb.init=0;

	/* screen components by crude bounding boxes then */
	/* render each one in D_PICK mode for detailed test */

	switch (p->type) {
	case ARC:   /* arc definition */
	    layer = p->u.a->layer;
	    break;
	case CIRC:  /* arc definition */
	    layer = p->u.c->layer;
	    break;
	case LINE:  /* arc definition */
	    layer = p->u.l->layer;
	    break;
	case NOTE:  /* note definition */
	    layer = p->u.n->layer;
	    break;
	case OVAL:  /* oval definition */
	    layer = p->u.o->layer;
	    break;
	case POLY:  /* polygon definition */
	    layer = p->u.p->layer;
	    break;
	case RECT:  /* rectangle definition */
	    layer = p->u.r->layer;
	    break;
	case TEXT:  /* text definition */
	    layer = p->u.t->layer;
	    break;
	case INST:  /* instance definition */
	    layer = 0;
	    break;
	default:
	    printf("error in db_ident switch\n");
	    break;
	}

	/* check all the restrictions on picking something */

	if (name != NULL) {comp = INST;}

	bad_comp=0;
	if ((pick_layer != 0 && layer != pick_layer) ||
	    (comp != 0 && comp != ALL && p->type != comp)  ||
	    (mode == 0 && !show_check_visible(currep, p->type, layer)) ||
	    (mode == 1 && !show_check_modifiable(currep, p->type, layer)) ||
	    (p->type==INST && name!=NULL && strncmp(name, p->u.i->name, MAXFILENAME))!=0) {
		bad_comp++;
	}

	if (!bad_comp) {
	    if ((pick_score=bb_distance(x,y,p,fuzz)) < 0.0) { 	/* inside BB */

		/* pick point inside BB gives a score ranging from */
		/* 1.0 to 2.0, with smaller BB's getting higher score */

		pick_score = 1.0 + 1.0/(1.0+min(fabs(p->xmax-p->xmin),
		    fabs(p->ymax-p->ymin)));


		/* a bit of a hack, but we overload the childbb */
		/* structure to pass the point into the draw routine. */
		/* If a hit, then childbb.xmax is set to 1.0 */
		/* seemed cleaner than more arguments, or */
		/* yet another global variable */

		childbb.xmin = x;
		childbb.ymin = y;
		childbb.xmax = 0.0;

		switch (p->type) {
		case ARC:  /* arc definition */
		    do_arc(p, &childbb, D_PICK); 
		    if (debug) printf("in arc %g\n", childbb.xmax);
		    break;
		case CIRC:  /* circle definition */
		    do_circ(p, &childbb, D_PICK); 
		    if (debug) printf("in circ %g\n", childbb.xmax);
		    break;
		case LINE:  /* line definition */
		    do_line(p, &childbb, D_PICK); 
		    if (debug) printf("in line %g\n", childbb.xmax);
		    break;
		case NOTE:  /* note definition */
		    do_note(p, &childbb, D_PICK); 
		    if (debug) printf("in note %g\n", childbb.xmax);
		    break;
		case OVAL:  /* oval definition */
		    do_oval(p, &childbb, D_PICK);
		    if (debug) printf("in oval %g\n", childbb.xmax);
		    break;
		case POLY:  /* polygon definition */
		    do_poly(p, &childbb, D_PICK);
		    if (debug) printf("in poly %g\n", childbb.xmax);
		    break;
		case RECT:  /* rectangle definition */
		    do_rect(p, &childbb, D_PICK);
		    if (debug) printf("in rect %g\n", childbb.xmax);
		    break;
		case TEXT:  /* text definition */
		    do_text(p, &childbb, D_PICK); 
		    if (debug) printf("in text %g\n", childbb.xmax);
		    break;
		case INST:  /* recursive instance call */
		    if (debug) printf("in inst %g\n", childbb.xmax);
		    break;
		default:
		    eprintf("unknown record type: %d in db_ident\n", p->type);
		    return(0);
		    break;
		}

		pick_score += childbb.xmax*3.0;

		/* now do some special cases - exact hits on 0,0 coords */

                if (p->type == INST && p->u.i->x == x && p->u.i->y == y) {
		    pick_score += 4.0;
		} else  if (p->type == CIRC && p->u.c->x1 == x && p->u.c->y1 == y) {
		    pick_score += 4.0;
		}

	    } else {
	        /* bb_distance() gives pick_score {0...1} based on bb distances */
	    }
	}

	/* keep track of the best of the lot */

	/* 0 < pick_score < 1 -> pick is outside bb */
	/* 1 < pick_score < 2 -> pick is inside bb */
	/* 4 < pick_score < 5 -> pick is inside bb and a fuzzy hit on some line */

	if (pick_score > pick_best) {
	    if (debug) printf("pick_score %g, best %g\n",
	        pick_score, pick_best);
	    fflush(stdout);
	    pick_best=pick_score;
	    p_best = p;
	}
    }

    return(p_best);
}

SELPNT *db_ident2(
    DB_TAB *cell,		/* device being edited */
    double x, double y,	 	/* pick point */
    int mode, 			/* 0=ident (any visible), 1=pick (only modifiable objects) */
    int pick_layer,		/* layer restriction, or 0=all */
    int comp,			/* comp restriction */
    char *name			/* instance name restrict or NULL */
)
{
    DB_DEFLIST *p;
    DB_DEFLIST *p_best=NULL;	
    COORDS *coords;
    SELPNT *selpnt;	/* gets returned */
    int debug=0;
    BOUNDS childbb;
    double pick_score=0.0;
    double pick_best=0.0;
    double fuzz;
    int layer;
    int bad_comp;

    if (cell == NULL) {
    	printf("no cell currently being edited!\n");
	return(0);
    }

    if (mode && debug) {		/* pick rather than ident mode */
    	printf("db_ident: called with layer %d, comp %d, name %s\n", 
		pick_layer, comp, name);
    }

    /* pick fuzz should be a fraction of total window size */
    
    fuzz = max((cell->maxx-cell->minx),(cell->maxy-cell->miny))*FUZZBAND;

    for (p=cell->dbhead; p!=(DB_DEFLIST *)0; p=p->next) {

	childbb.init=0;

	/* screen components by crude bounding boxes then */
	/* render each one in D_PICK mode for detailed test */

	switch (p->type) {
	case ARC:   /* arc definition */
	    layer = p->u.a->layer;
	    break;
	case CIRC:  /* arc definition */
	    layer = p->u.c->layer;
	    break;
	case LINE:  /* arc definition */
	    layer = p->u.l->layer;
	    break;
	case NOTE:  /* note definition */
	    layer = p->u.n->layer;
	    break;
	case OVAL:  /* oval definition */
	    layer = p->u.o->layer;
	    break;
	case POLY:  /* polygon definition */
	    layer = p->u.p->layer;
	    break;
	case RECT:  /* rectangle definition */
	    layer = p->u.r->layer;
	    break;
	case TEXT:  /* text definition */
	    layer = p->u.t->layer;
	    break;
	case INST:  /* instance definition */
	    layer = 0;
	    break;
	default:
	    printf("error in db_ident switch\n");
	    break;
	}

	/* check all the restrictions on picking something */

	if (name != NULL) {comp = INST;}

	bad_comp=0;
	if ((pick_layer != 0 && layer != pick_layer) ||
	    (comp != 0 && comp != ALL && p->type != comp)  ||
	    (mode == 0 && !show_check_visible(currep, p->type, layer)) ||
	    (mode == 1 && !show_check_modifiable(currep, p->type, layer)) ||
	    (p->type==INST && name!=NULL && strncmp(name, p->u.i->name, MAXFILENAME))!=0) {
		bad_comp++;
	}

	if (!bad_comp) {
	    if ((pick_score=bb_distance(x,y,p,fuzz)) < 0.0) { 	/* inside BB */

		/* pick point inside BB gives a score ranging from */
		/* 1.0 to 2.0, with smaller BB's getting higher score */

		pick_score = 1.0 + 1.0/(1.0+min(fabs(p->xmax-p->xmin),
		    fabs(p->ymax-p->ymin)));


		/* a bit of a hack, but we overload the childbb */
		/* structure to pass the point into the draw routine. */
		/* If a hit, then childbb.xmax is set to 1.0 */
		/* seemed cleaner than more arguments, or */
		/* yet another global variable */

		childbb.xmin = x;
		childbb.ymin = y;
		childbb.xmax = 0.0;

		switch (p->type) {
		case ARC:  /* arc definition */
		    do_arc(p, &childbb, D_PICK); 
		    if (debug) printf("in arc %g\n", childbb.xmax);
		    break;
		case CIRC:  /* circle definition */
		    do_circ(p, &childbb, D_PICK); 
		    if (debug) printf("in circ %g\n", childbb.xmax);
		    break;
		case LINE:  /* line definition */
		    do_line(p, &childbb, D_PICK); 
		    if (debug) printf("in line %g\n", childbb.xmax);
		    break;
		case NOTE:  /* note definition */
		    do_note(p, &childbb, D_PICK); 
		    if (debug) printf("in note %g\n", childbb.xmax);
		    break;
		case OVAL:  /* oval definition */
		    do_oval(p, &childbb, D_PICK);
		    if (debug) printf("in oval %g\n", childbb.xmax);
		    break;
		case POLY:  /* polygon definition */
		    do_poly(p, &childbb, D_PICK);
		    if (debug) printf("in poly %g\n", childbb.xmax);
		    break;
		case RECT:  /* rectangle definition */
		    do_rect(p, &childbb, D_PICK);
		    if (debug) printf("in rect %g\n", childbb.xmax);
		    break;
		case TEXT:  /* text definition */
		    do_text(p, &childbb, D_PICK); 
		    if (debug) printf("in text %g\n", childbb.xmax);
		    break;
		case INST:  /* recursive instance call */
		    if (debug) printf("in inst %g\n", childbb.xmax);
		    break;
		default:
		    eprintf("unknown record type: %d in db_ident\n", p->type);
		    return(0);
		    break;
		}

		pick_score += childbb.xmax*3.0;

		/* now do some special cases - exact hits on 0,0 coords */

                if (p->type == INST && p->u.i->x == x && p->u.i->y == y) {
		    pick_score += 4.0;
		} else  if (p->type == CIRC && p->u.c->x1 == x && p->u.c->y1 == y) {
		    pick_score += 4.0;
		}

	    } else {
	        /* bb_distance() gives pick_score {0...1} based on bb distances */
	    }
	}

	/* keep track of the best of the lot */

	/* 0 < pick_score < 1 -> pick is outside bb */
	/* 1 < pick_score < 2 -> pick is inside bb */
	/* 4 < pick_score < 5 -> pick is inside bb and a fuzzy hit on some line */

	if (pick_score > pick_best) {
	    if (debug) printf("pick_score %g, best %g\n",
	        pick_score, pick_best);
	    fflush(stdout);
	    pick_best=pick_score;
	    p_best = p;
	}
    }

    selpnt = NULL;

    if (p_best != NULL) {
	p = p_best;
	/* selpnt_clear(&selpnt); */

	switch (p->type) {

	case ARC:
	    selpnt_save(&selpnt, &(p->u.a->x1), &(p->u.a->y1), NULL);
	    selpnt_save(&selpnt, &(p->u.a->x2), &(p->u.a->y2), NULL);
	    selpnt_save(&selpnt, &(p->u.a->x3), &(p->u.a->y3), NULL);
	    break;

	case CIRC:
	    selpnt_save(&selpnt, &(p->u.c->x1), &(p->u.c->y1), NULL);
	    selpnt_save(&selpnt, &(p->u.c->x2), &(p->u.c->y2), NULL);
	    break;

	case INST:
	    selpnt_save(&selpnt, &(p->u.i->x), &(p->u.i->y), NULL);
	    break;

	case LINE:
	    for (coords=p->u.l->coords;coords!=NULL;coords=coords->next) {
		selpnt_save(&selpnt, &(coords->coord.x), &(coords->coord.y),NULL);
	    }
	    break;

	case NOTE:
	    selpnt_save(&selpnt, &(p->u.n->x), &(p->u.n->y), NULL);
	    break;

	case POLY:
	    for (coords=p->u.p->coords;coords!=NULL;coords=coords->next) {
		selpnt_save(&selpnt, &(coords->coord.x), &(coords->coord.y),NULL);
	    }
	    break;

	case RECT:
	    selpnt_save(&selpnt, &(p->u.r->x1), &(p->u.r->y1), NULL);
	    selpnt_save(&selpnt, &(p->u.r->x2), &(p->u.r->y2), NULL);

	    break;

	case TEXT:
	    selpnt_save(&selpnt, &(p->u.t->x), &(p->u.t->y), NULL);
	    break;

	default:
	    printf("    not a stretchable object\n");
	}

	selpnt_save(&selpnt, NULL, NULL, p);	/* save comp */
    }
    return(selpnt);
}

int bb_overlap(
    double x1, 
    double y1,
    double x2,
    double y2,
    double xc1,
    double yc1,
    double xc2,
    double yc2
) {

    int err = 0;
    double tmp;

    /* canonicalize inputs */

    if (x1 > x2)   { tmp = x2;   x2 = x1;   x1 = tmp; }
    if (y1 > y2)   { tmp = y2;   y2 = y1;   y1 = tmp; }
    if (xc1 > xc2) { tmp = xc2; xc2 = xc1; xc1 = tmp; }
    if (yc1 > yc2) { tmp = yc2; yc2 = yc1; yc1 = tmp; }

    if (y2 < yc1 ) err++;
    if (xc2 < x1 ) err++;
    if (yc2 < y1 ) err++;
    if (xc1 > x2 ) err++;

    /* clip one rectangle against another */

    if (y1 < yc1 && yc1 < y2)   y1 = yc1;     /* trim bottom */
    if (x1 < xc2 && xc2 < x2)   x2 = xc2;     /* trim right */
    if (y1 < yc2 && yc2 < y2)   y2 = yc2;     /* trim top */
    if (x1 < xc1 && xc1 < x2)   x1 = xc1;     /* trim left */

    if (x1 >= x2)   err++;
    if (y1 >= y2)   err++;

    if (!err) {
        return 1;
    } else {
        return 0;
    }
}

int bounded(double *x, double *y, double xmin, double ymin, double xmax, double ymax) {

     double tmp;

     /* canonicalize bounding box */

     if (xmax < xmin) { tmp = xmax; xmax = xmin; xmin = tmp; }
     if (ymax < ymin) { tmp = ymax; ymax = ymin; ymin = tmp; }

     if ((*x >= xmin && *x <= xmax) && (*y >= ymin && *y <= ymax)) {
        return 1;
     } else {
        return 0;
     }
}

                                               
SELPNT *db_ident_region2(
    DB_TAB *cell,		/* device being edited */
    NUM x1, NUM y1,	 		/* pick point */
    NUM x2, NUM y2,	 		/* pick point */
    int mode,			/* 0 = ident (any visible) */
				/* 1 = enclose pick (pick restricted to modifiable objects) */
				/* 2 = boundary overlap (restricted to modifiable objects) */
    int pick_layer,			/* layer restriction, or 0=all */
    int comp,			/* comp restriction */
    char *name			/* instance name restrict or NULL */
) {
    DB_DEFLIST *p;
    SELPNT *selpnt=NULL;	/* gets returned */
    /* int debug=0; */
    // BOUNDS childbb;
    double tmp;
    int layer;
    int bad_comp;
    int gotpnts;
    double *xsel;
    double *ysel;
    COORDS *coords;
    double *xmin, *ymin, *xmax, *ymax;
    int sel;

    if (cell == NULL) {
    	printf("no cell currently being edited!\n");
	return(0);
    }

    selpnt=NULL;

    for (p=cell->dbhead; p!=(DB_DEFLIST *)0; p=p->next) {

	// childbb.init=0;

	/* screen components by layers and bounding boxes */

	switch (p->type) {
	case ARC:   /* arc definition */
	    layer = p->u.a->layer;
	    break;
	case CIRC:  /* arc definition */
	    layer = p->u.c->layer;
	    break;
	case LINE:  /* arc definition */
	    layer = p->u.l->layer;
	    break;
	case NOTE:  /* note definition */
	    layer = p->u.n->layer;
	    break;
	case OVAL:  /* oval definition */
	    layer = p->u.o->layer;
	    break;
	case POLY:  /* polygon definition */
	    layer = p->u.p->layer;
	    break;
	case RECT:  /* rectangle definition */
	    layer = p->u.r->layer;
	    break;
	case TEXT:  /* text definition */
	    layer = p->u.t->layer;
	    break;
	case INST:  /* instance definition */
	    layer = 0;
	    break;
	default:
	    printf("error in db_ident switch\n");
	    break;
	}

	/* check all the restrictions on picking something */

	if (name != NULL) { comp = INST; }

	bad_comp=0;
	if ((pick_layer != 0 && layer != pick_layer) ||
	    (comp != 0 && comp != ALL && p->type != comp)  ||
	    (!mode && !show_check_visible(currep, p->type, layer)) ||
	    (mode && !show_check_modifiable(currep, p->type, layer)) ||
	    (p->type==INST && name!=NULL && strncmp(name, p->u.i->name, MAXFILENAME))!=0) {
		bad_comp++;
	}

	if (!bad_comp) {
	    if (x1 > x2) {			/* canonicalize coords */
	    	tmp = x2; x2 = x1; x1 = tmp;
	    }
	    if (y1 > y2) {
	    	tmp = y2; y2 = y1; y1 = tmp;
	    }


	    if ( ( (mode==0 || mode==1) &&   /* check for enclosure */
	           (x1<=p->xmin && x2>=p->xmax && y1<=p->ymin && y2>=p->ymax) ) ||
		 ( (mode==2) && 	    /* check for mutual overlap */
		   (bb_overlap(x1, y1, x2, y2, p->xmin, p->ymin, p->xmax, p->ymax)) ) ) {

		gotpnts = 0;
		switch (p->type) {

		case ARC:

		    xsel = &(p->u.a->x3);
		    ysel = &(p->u.a->y3);
		    if (bounded (xsel, ysel, x1, y1, x2, y2)) {
			selpnt_save(&selpnt, xsel, ysel, NULL);
			gotpnts++;
		    }

		    xsel = &(p->u.a->x2);
		    ysel = &(p->u.a->y2);
		    if (bounded (xsel, ysel, x1, y1, x2, y2)) {
			selpnt_save(&selpnt, xsel, ysel, NULL);
			gotpnts++;
		    }

		    xsel = &(p->u.a->x1);
		    ysel = &(p->u.a->y1);
		    if (bounded (xsel, ysel, x1, y1, x2, y2)) {
			selpnt_save(&selpnt, xsel, ysel, NULL);
			gotpnts++;
		    }
		    break;

		case CIRC:

		    xsel = &(p->u.c->x2);
		    ysel = &(p->u.c->y2);
		    if (bounded (xsel, ysel, x1, y1, x2, y2)) {
			selpnt_save(&selpnt, xsel, ysel, NULL);
			gotpnts++;
		    }

		    xsel = &(p->u.c->x1);
		    ysel = &(p->u.c->y1);
		    if (bounded (xsel, ysel, x1, y1, x2, y2)) {
			selpnt_save(&selpnt, xsel, ysel, NULL);
			gotpnts++;
		    }
		    break;

		case INST:

		    xsel = &(p->u.i->x);
		    ysel = &(p->u.i->y);
		    if (bounded (xsel, ysel, x1, y1, x2, y2)) {
			selpnt_save(&selpnt, xsel, ysel, NULL);
			gotpnts++;
		    }
		    break;

		case LINE:

		    for (coords=p->u.l->coords;coords!=NULL;coords=coords->next) {
			xsel = &(coords->coord.x);
			ysel = &(coords->coord.y);
			if (bounded (xsel, ysel, x1, y1, x2, y2)) {
			    selpnt_save(&selpnt, xsel, ysel, NULL);
			    gotpnts++;
			}
		    }
		    break;

		case NOTE:

		    xsel = &(p->u.n->x);
		    ysel = &(p->u.n->y);
		    if (bounded (xsel, ysel, x1, y1, x2, y2)) {
			selpnt_save(&selpnt, xsel, ysel, NULL);
			gotpnts++;
		    }
		    break;

		case POLY:

		    for (coords=p->u.p->coords;coords!=NULL;coords=coords->next) {
			xsel = &(coords->coord.x);
			ysel = &(coords->coord.y);
			if (bounded (xsel, ysel, x1, y1, x2, y2)) {
			    selpnt_save(&selpnt, xsel, ysel, NULL);
			    gotpnts++;
			}
		    }
		    break;

		case RECT:
		    xmin = &(p->u.r->x1);
		    xmax = &(p->u.r->x2);
		    ymin = &(p->u.r->y1);
		    ymax = &(p->u.r->y2);

		    sel=0;

		    if (bounded (xmin, ymin, x1, y1, x2, y2)) sel += 1;
		    if (bounded (xmin, ymax, x1, y1, x2, y2)) sel += 2;
		    if (bounded (xmax, ymax, x1, y1, x2, y2)) sel += 4;
		    if (bounded (xmax, ymin, x1, y1, x2, y2)) sel += 8;

		    switch (sel) {
		    case 0:
			break;
		    case 1:
			selpnt_save(&selpnt, xmin, ymin, NULL);
			gotpnts++;
			break;
		    case 2:
			selpnt_save(&selpnt, xmin, ymax, NULL);
			gotpnts++;
			break;
		    case 3:	/* left side */
			selpnt_save(&selpnt, xmin, NULL, NULL);
			gotpnts++;
			break;
		    case 4:
			selpnt_save(&selpnt, xmax, ymax, NULL);
			gotpnts++;
			break;
		    case 6:	/* top side */
			selpnt_save(&selpnt, NULL, ymax, NULL);
			gotpnts++;
			break;
		    case 8:
			selpnt_save(&selpnt, xmax, ymin, NULL);
			gotpnts++;
			break;
		    case 9:	/* bottom side */
			selpnt_save(&selpnt,NULL, ymin, NULL);
			gotpnts++;
			break;
		    case 12:	/* right side */
			selpnt_save(&selpnt,xmax, NULL, NULL);
			gotpnts++;
			break;
		    case 15:	/* entire rectangle */
			selpnt_save(&selpnt,xmin, ymin, NULL);
			selpnt_save(&selpnt,xmax, ymax, NULL);
			gotpnts+=2;
			break;
		    default:
			printf("   COM_STRETCH: case %d should never occur!\n",sel);
			break;
		    }

		    break;

		case TEXT:

		    xsel = &(p->u.t->x);
		    ysel = &(p->u.t->y);

		    if (bounded (xsel, ysel, x1, y1, x2, y2)) {
			selpnt_save(&selpnt,xsel, ysel, NULL);
			gotpnts+=2;
		    }
		    break;

		default:
		    printf("    not a stretchable object\n");
		    db_notate(p);	/* print information */
		    break;
		}

		if (gotpnts) {
		    selpnt_save(&selpnt, NULL, NULL, p);	/* save comp */
		} 
	    }
	}
    }
    return(selpnt);
}

STACK *db_ident_region(
    DB_TAB *cell,		/* device being edited */
    NUM x1, NUM y1,	 		/* pick point */
    NUM x2, NUM y2,	 		/* pick point */
    int mode, 			/* 0 = ident (any visible) */
				    /* 1 = enclose pick (pick restricted to modifiable objects) */
				    /* 2 = boundary overlap (restricted to modifiable objects) */
    int pick_layer,		/* layer restriction, or 0=all */
    int comp,			/* comp restriction */
    char *name			/* instance name restrict or NULL */
) {
    DB_DEFLIST *p;
    STACK *stack=NULL;	/* gets returned */
    /* int debug=0; */
    // BOUNDS childbb;
    double tmp;
    int layer;
    int bad_comp;

    if (cell == NULL) {
    	printf("no cell currently being edited!\n");
	return(0);
    }

    stack=NULL;

    for (p=cell->dbhead; p!=(DB_DEFLIST *)0; p=p->next) {

	// childbb.init=0;

	/* screen components by layers and bounding boxes */

	switch (p->type) {
	case ARC:   /* arc definition */
	    layer = p->u.a->layer;
	    break;
	case CIRC:  /* arc definition */
	    layer = p->u.c->layer;
	    break;
	case LINE:  /* arc definition */
	    layer = p->u.l->layer;
	    break;
	case NOTE:  /* note definition */
	    layer = p->u.n->layer;
	    break;
	case OVAL:  /* oval definition */
	    layer = p->u.o->layer;
	    break;
	case POLY:  /* polygon definition */
	    layer = p->u.p->layer;
	    break;
	case RECT:  /* rectangle definition */
	    layer = p->u.r->layer;
	    break;
	case TEXT:  /* text definition */
	    layer = p->u.t->layer;
	    break;
	case INST:  /* instance definition */
	    layer = 0;
	    break;
	default:
	    printf("error in db_ident switch\n");
	    break;
	}

	/* check all the restrictions on picking something */

	if (name != NULL) { comp = INST; }

	bad_comp=0;
	if ((pick_layer != 0 && layer != pick_layer) ||
	    (comp != 0 && comp != ALL && p->type != comp)  ||
	    (!mode && !show_check_visible(currep, p->type, layer)) ||
	    (mode && !show_check_modifiable(currep, p->type, layer)) ||
	    (p->type==INST && name!=NULL && strncmp(name, p->u.i->name, MAXFILENAME))!=0) {
		bad_comp++;
	}

	if (!bad_comp) {
	    if (x1 > x2) {			/* canonicalize coords */
	    	tmp = x2; x2 = x1; x1 = tmp;
	    }
	    if (y1 > y2) {
	    	tmp = y2; y2 = y1; y1 = tmp;
	    }

	    if (mode == 0 || mode == 1 ) {	/* check for enclosure */
		if (x1 <= p->xmin  && x2 >= p->xmax && y1 <= p->ymin && y2 >= p->ymax) {
		    stack_push(&stack, (char *)p);
		}
	    } else if (mode == 2)  { /* check for mutual overlap */
		if (bb_overlap(x1, y1, x2, y2, p->xmin, p->ymin, p->xmax, p->ymax)) {
		    stack_push(&stack, (char *)p);
		}
	    }
	}
    }
    return(stack);
}

void db_drawbounds( double xmin, double ymin, double xmax, double ymax, int mode)
{
    BOUNDS bb;

    bb.init=0;
    set_layer(0,0);

    if (xmin == xmax) {
        jump(&bb, mode);
	draw(xmax, ymin, &bb, mode);
	draw(xmax, ymax, &bb, mode);
    } else if (ymin == ymax) {
        jump(&bb, mode);
	draw(xmin, ymin, &bb, mode);
	draw(xmax, ymin, &bb, mode);
    } else {
	jump(&bb, mode);
	    /* a square bounding box outline in white */
	    draw(xmax, ymax, &bb, mode); 
	    draw(xmax, ymin, &bb, mode);
	    draw(xmin, ymin, &bb, mode);
	    draw(xmin, ymax, &bb, mode);
	    draw(xmax, ymax, &bb, mode);
	jump(&bb, mode);
	    /* and diagonal lines from corner to corner */
	    draw(xmax, ymax, &bb, mode);
	    draw(xmin, ymin, &bb, mode);
	jump(&bb, mode) ;
	    draw(xmin, ymax, &bb, mode);
	    draw(xmax, ymin, &bb, mode);
    }
}

double db_area(DB_DEFLIST *p)	/* return the area of component p */
{

    double area = 0.0;
    double x,y, xstart, ystart, xold, yold, r2;
    COORDS *coords;
    int i, count;
    int debug=0;
    double len; 

    if (p == NULL) {
    	printf("db_area: no component!\n");
	return(0);
    }

    switch (p->type) {
    case ARC:  /* arc definition */
	area = -1.0;
	break;
    case CIRC:  /* circle definition */
	r2 = pow(p->u.c->x1 - p->u.c->x2, 2.0) +
	    pow(p->u.c->y2 - p->u.c->y1, 2.0);
	area = M_PI*r2;
	break;
    case LINE:  /* line definition */
	if (p->u.l->opts->width == 0.0) {
	    area = 0.0;
	} else {
	    i=0;
	    len=0.0;
	    for (coords = p->u.l->coords; coords != NULL; coords = coords->next, i++) {
		xold=x; x=coords->coord.x;
		yold=y; y=coords->coord.y;
	    	if (debug) printf("xy = %g %g, i=%d, len=%g\n", x,y,i,len);
		if (i > 0) {
		   len += sqrt(pow(x-xold,2.0)+pow(y-yold,2.0));
		}
	    }
	    area = len * p->u.l->opts->width;
	}
	break;
    case NOTE:  /* note definition */
	/* return just area of bounding box */
	area = fabs(p->xmax - p->xmin) * fabs(p->ymax - p->ymin);
	break;
    case OVAL:  /* oval definition */
    	area = -1.0;
	break;
    case POLY:  /* polygon definition */

	/* it is easy enough to calculate the area inline */
	/* the following code works whether or not the last */
	/* point is equal to the first point */

	/* this looks like magic, but it is really just a */
	/* textbook version of a poly area algorithm */
	/* modified to work with a linked list */

	coords = p->u.p->coords;
	count = 0;
	area = 0.0;
	while(coords != NULL) {
	    xold=x; x=coords->coord.x;
	    yold=y; y=coords->coord.y; 
	    if (count == 0) {
		xstart=x;
		ystart=y;
	    } else if (count > 0) {
		area += xold * y;
		area -= yold * x;
	    }
	    coords = coords->next;
	    count++;
	}
	area += x * ystart;
	area -= y * xstart;
	area /= 2.0;
	area = (area<0.0)?-area:area;

	break;
    case RECT:  /* rectangle definition */
	area = fabs(p->xmax - p->xmin) * fabs(p->ymax - p->ymin);
	break;
    case TEXT:  /* text definition */
	/* return just area of bounding box */
	area = fabs(p->xmax - p->xmin) * fabs(p->ymax - p->ymin);
	break;
    case INST:  /* instance call */
	/* return just area of bounding box */
	area = fabs(p->xmax - p->xmin) * fabs(p->ymax - p->ymin);
	break;
    default:
	eprintf("unknown record type: %d in db_area\n", p->type);
	area = -1.0;
	break;
    }
    return(area);
}

void db_notate(DB_DEFLIST *p)   /* print out identifying information */
{
    int debug=0;

    if (p == NULL) {
    	printf("db_notate: no component!\n");
    } else {
	if (debug) printf("%ld:", (long int) p);
	switch (p->type) {
	case ARC:  /* arc definition */
	    printf("   ARC %d (%s) end1=%.5g,%.5g end2=%.5g,%.5g pt_on_circumference=%.5g,%.5g ", 
		    p->u.a->layer, equate_get_label(p->u.a->layer), p->u.a->x1, p->u.a->y1, 
		    p->u.a->x2, p->u.a->y2, p->u.a->x3, p->u.a->y3);
	    db_print_opts(stdout, p->u.a->opts, ARC_OPTS);
	    printf("\n");
	    break;
	case CIRC:  /* circle definition */
	    printf("   CIRC %d (%s) center=%.5g,%.5g pt_on_circumference=%.5g,%.5g ", 
		    p->u.c->layer, equate_get_label(p->u.c->layer), p->u.c->x1, 
		    p->u.c->y1, p->u.c->x2, p->u.c->y2);
	    db_print_opts(stdout, p->u.c->opts, CIRC_OPTS);
	    printf("Radius = %.5g", sqrt(pow(p->u.c->x1-p->u.c->x2,2.0)+pow(p->u.c->y1 - p->u.c->y2, 2.0)));
	    printf("\n");
	    break;
	case LINE:  /* line definition */
	    printf("   LINE %d (%s) LL=%.5g,%.5g UR=%.5g,%.5g ", 
		    p->u.l->layer, equate_get_label(p->u.l->layer), 
		    p->xmin, p->ymin, p->xmax, p->ymax);
	    db_print_opts(stdout, p->u.l->opts, LINE_OPTS);
	    printf("\n");
	    break;
	case NOTE:  /* note definition */
	    printf("   NOTE %d (%s) LL=%.5g,%.5g UR=%.5g,%.5g ", 
		    p->u.n->layer, equate_get_label(p->u.n->layer), 
		    p->xmin, p->ymin, p->xmax, p->ymax);
	    db_print_opts(stdout, p->u.n->opts, NOTE_OPTS);
	    printf(" \"%s\"\n", p->u.n->text);
	    break;
	case OVAL:  /* oval definition */
	    break;
	case POLY:  /* polygon definition */
	    printf("   POLY %d (%s) LL=%.5g,%.5g UR=%.5g,%.5g ", 
		    p->u.p->layer, equate_get_label(p->u.p->layer),
		    p->xmin, p->ymin, p->xmax, p->ymax);
	    db_print_opts(stdout, p->u.p->opts, POLY_OPTS);
	    printf("\n");
	    break;
	case RECT:  /* rectangle definition */
	    printf("   RECT %d (%s) LL=%.5g,%.5g UR=%.5g,%.5g ", 
		    p->u.r->layer, equate_get_label(p->u.r->layer),
		    p->xmin, p->ymin, p->xmax, p->ymax);
	    db_print_opts(stdout, p->u.r->opts, RECT_OPTS);
	    printf("Width = %.5g, Height = %.5g", fabs(p->u.c->x1-p->u.c->x2), fabs(p->u.c->y1 - p->u.c->y2));
	    printf("\n");
	    break;
	case TEXT:  /* text definition */
	    printf("   TEXT %d (%s) LL=%.5g,%.5g UR=%.5g,%.5g ", 
		    p->u.t->layer, equate_get_label(p->u.r->layer),
		    p->xmin, p->ymin, p->xmax, p->ymax);
	    db_print_opts(stdout, p->u.t->opts, TEXT_OPTS);
	    printf(" \"%s\"\n", p->u.t->text);
	    break;
	case INST:  /* instance call */
	    printf("   INST %s LL=%.5g,%.5g UR=%.5g,%.5g ", 
		    p->u.i->name, p->xmin, p->ymin, p->xmax, p->ymax);
	    db_print_opts(stdout, p->u.i->opts, INST_OPTS);
	    printf("\n");
	    break;
	default:
	    eprintf("unknown record type: %d in db_notate\n", p->type);
	    break;
	}
    }
}

void db_highlight(DB_DEFLIST *p)	/* component to display */
{
    BOUNDS bb;
    DB_TAB *def;
    bb.init=0;
    XFORM *xp;
    double x1, y1, x2, y2, xx, yy;

    if (p == NULL) {
    	printf("db_highlight: no component!\n");
	return;
    }

    switch (p->type) {
    case 0:  	// do nothing for a null type
        break;
    case ARC:  /* arc definition */
	db_drawbounds(p->xmin, p->ymin, p->xmax, p->ymax, D_RUBBER);
	do_arc(p, &bb, D_RUBBER);
	break;
    case CIRC:  /* circle definition */
	db_drawbounds(p->xmin, p->ymin, p->xmax, p->ymax, D_RUBBER);
	xwin_draw_origin(p->u.c->x1, p->u.c->y1);
	do_circ(p, &bb, D_RUBBER);
	break;
    case LINE:  /* line definition */
	// db_drawbounds(p->xmin, p->ymin, p->xmax, p->ymax, D_RUBBER);
	do_line(p, &bb, D_RUBBER);
	break;
    case NOTE:  /* note definition */
	db_drawbounds(p->xmin, p->ymin, p->xmax, p->ymax, D_RUBBER);
	xwin_draw_origin(p->u.n->x, p->u.n->y);
	// do_note(p, &bb, D_RUBBER);	
	break;
    case OVAL: /* oval definition */
	db_drawbounds(p->xmin, p->ymin, p->xmax, p->ymax, D_RUBBER);
	do_oval(p, &bb, D_RUBBER); break;
    case POLY: /* polygon definition */
	db_drawbounds(p->xmin, p->ymin, p->xmax, p->ymax, D_RUBBER);
	do_poly(p, &bb, D_RUBBER); break;
    case RECT: /* rectangle definition */
	db_drawbounds(p->xmin, p->ymin, p->xmax, p->ymax, D_RUBBER);
	do_rect(p, &bb, D_RUBBER); break;
    case TEXT: /* text definition */
	db_drawbounds(p->xmin, p->ymin, p->xmax, p->ymax, D_RUBBER);
	xwin_draw_origin(p->u.t->x, p->u.t->y);
	// do_note(p, &bb, D_RUBBER);
	break;
    case INST: /* instance call */
	def = db_lookup(p->u.i->name);

	xp = matrix_from_opts(p->u.i->opts);
	xp->dx = p->u.i->x;
	xp->dy = p->u.i->y;

	x1 = def->minx;
	y1 = def->miny;
	x2 = def->maxx;
        y2 = def->maxy;

	jump(&bb, D_RUBBER);
	set_layer(0,0);

	xx = x1; yy = y1;
	xform_point(xp, &xx, &yy);
	draw(xx, yy, &bb, D_RUBBER);

	xx = x1; yy = y2;
	xform_point(xp, &xx, &yy);
	draw(xx, yy, &bb, D_RUBBER);

	xx = x2; yy = y2;
	xform_point(xp, &xx, &yy);
	draw(xx, yy, &bb, D_RUBBER);

	xx = x2; yy = y1;
	xform_point(xp, &xx, &yy);
	draw(xx, yy, &bb, D_RUBBER);

	xx = x1; yy = y1;
	xform_point(xp, &xx, &yy);
	draw(xx, yy, &bb, D_RUBBER);

	xx = x2; yy = y2;
	xform_point(xp, &xx, &yy);
	draw(xx, yy, &bb, D_RUBBER);

	jump(&bb, D_RUBBER);

	xx = x1; yy = y2;
	xform_point(xp, &xx, &yy);
	draw(xx, yy, &bb, D_RUBBER);

	xx = x2; yy = y1;
	xform_point(xp, &xx, &yy);
	draw(xx, yy, &bb, D_RUBBER);

	jump(&bb, D_RUBBER);

	xx = 0.0; yy = 0.0;
	xform_point(xp, &xx, &yy);
	xwin_draw_origin(xx,yy);

	jump(&bb, D_RUBBER);

	free(xp);

	break;
    default:
	eprintf("unknown record type: %d in db_highlight\n", p->type);
	break;
    }
}

int db_list(DB_TAB *cell)
{
    DB_DEFLIST *p;

    if (currep == NULL) {
    	printf("not currently editing any rep!\n");
	return(0);
    }

    for (p=cell->dbhead; p!=(DB_DEFLIST *)0; p=p->next) {
	db_notate(p);
    }
    printf("   grid is %.5g %.5g %.5g %.5g %.5g %.5g\n",
	currep->grid_xd, currep->grid_yd, currep->grid_xs,
	currep->grid_ys, currep->grid_xo, currep->grid_yo);

    printf("   logical level = %d\n", currep->logical_level);
    printf("   lock angle    = %g\n", currep->lock_angle);
    printf("   num prims     = %d\n", currep->prims);
    printf("   nesting is = ");
    if (currep->physical) {
	printf("%d physical\n", currep->nestlevel);
    } else {
	printf("%d logical\n", currep->nestlevel);
    }

    if (currep->modified) {
	printf("   cell is modified\n");
    } else {
	printf("   cell is not modified\n");
    }


    return(1);
}

// dx,dy page size in inches
int db_plot(char *name, OMODE plottype, double dx,
            double dy, int color, double pen) {
    BOUNDS bb;
    char buf[MAXFILENAME];
    double x1, y1, x2, y2;

    bb.init=0;

    if (currep == NULL) {
    	printf("not currently editing any rep!\n");
	return(0);
    }

    if (plottype  == AUTOPLOT) {
	snprintf(buf, MAXFILENAME, "%s.pd", name);
	printf("plotting autoplot format to %s\n", buf);
	fflush(stdout);
    } else if (plottype  == POSTSCRIPT) {
	snprintf(buf, MAXFILENAME, "%s.ps", name);
	printf("plotting postscript to %s\n", buf);
	fflush(stdout);
    } else if (plottype == GERBER) {
	snprintf(buf, MAXFILENAME, "%s.gb", name);
	printf("plotting gerber to %s\n", buf);
	fflush(stdout);
    } else if (plottype == DXF) {
	snprintf(buf, MAXFILENAME, "%s.dxf", name);
	printf("plotting dxf to %s\n", buf);
	fflush(stdout);
    } else if (plottype == HPGL) {
	snprintf(buf, MAXFILENAME, "%s.hpgl", name);
	printf("plotting hpgl to %s\n", buf);
	fflush(stdout);
    } else if (plottype == SVG) {
	snprintf(buf, MAXFILENAME, "%s.svg", name);
	printf("plotting svg to %s\n", buf);
	fflush(stdout);
    } else if (plottype == WEB) {
	snprintf(buf, MAXFILENAME, "%s.html", name);
	printf("plotting svg to %s\n", buf);
	fflush(stdout);
    } else {
       printf("bad plottype: %d\n", plottype);
       return(0);
    }

    if((PLOT_FD=fopen(buf, "w+")) == 0) {
    	printf("db_plot: can't open plotfile: \"%s\"\n", buf);
	return(0);
    } 

    X=0;				    /* turn X display off */

    x1 = currep->vp_xmin;
    y1 = currep->vp_ymin;
    x2 = currep->vp_xmax;
    y2 = currep->vp_ymax;

    printf("currep min/max bounds: %g, %g, %g, %g\n", 
    	currep->minx, currep->miny, currep->maxx, currep->maxy);
    printf("currep vp bounds: %g, %g, %g, %g\n", 
    	x1, y1, x2, y2);

    R_to_V(&x1, &y1);
    R_to_V(&x2, &y2);

    printf("currep screen bounds: %g, %g, %g, %g\n", 
    	x1, y1, x2, y2);

    printf("gwidth/height: %g, %g\n", 
    	(double) g_width, (double) g_height);
    printf("bounds to ps: %g, %g, %g, %g\n", 
    	x1, y2, x2, y1);


    /* FIXME: make version number a global or at least subroutine */

    ps_set_file(PLOT_FD);
    ps_set_linewidth(pen);	// linewidth
    ps_set_color(color);	// color==1, bw=0

    /* void ps_preamble(fp,dev, prog, pdx, pdy, llx, lly, urx, ury) */
    // ps_preamble(currep->name, "piglet version 0.95h", 8.5, 11.0, 
    /* x1, y2, x2, y1); */

    ps_preamble(currep->name, "piglet version 0.95h", dx, dy, 
        0.0, 0.0, (double) g_width, (double) g_height);

    db_render(currep, 0, &bb, D_NORM);      /* dump plot */

    ps_postamble();

    X=1;				    /* turn X back on*/
    return(1);
}

void xform_point(XFORM *xp, double *xx, double *yy) 
{
    double x,y;

    x = (*xx)*xp->r11 + (*yy)*xp->r21 + xp->dx;
    y = (*xx)*xp->r12 + (*yy)*xp->r22 + xp->dy;

    *xx=x;
    *yy=y;
}

/* return 1 if no shear, non-ortho rotate, or non-isotropic scale */
int is_ortho(OPTS *opts)	      
{
    int retval = 1;	/* assume is ortho */

    /* opts->mirror; */
    /* opts->scale */

    if (fmod(opts->rotation,90.0) != 0.0) retval = 0;
    if (opts->slant != 0.0) retval = 0;
    if (opts->aspect_ratio != 1.0) retval = 0;
    if (opts->slant != 0.0) retval = 0;

    return(retval);
}

XFORM *matrix_from_opts(OPTS *opts) /* initialize xfrom matrix from options */
{
    XFORM *xp;

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

    /* NOTE: To work properly, these transformations have to */
    /* occur in the proper order, for example, rotation must come */
    /* after slant transformation */

    if (opts != NULL) {
	switch (opts->mirror) {
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
	mat_scale(xp, opts->scale, opts->scale); 
	mat_scale(xp, 1.0/opts->aspect_ratio, 1.0); 
	mat_slant(xp,  opts->slant); 
	mat_rotate(xp, opts->rotation);
    }

    return(xp);
} 

char * prim_name(DB_DEFLIST *p) {
    switch (p->type) {
    case ARC:  /* arc definition */
	return("arc");
    case CIRC:  /* circle definition */
	return("circle");
    case LINE:  /* line definition */
	return("line");
    case NOTE:  /* note definition */
	return("note");
    case OVAL:  /* oval definition */
	return("oval");
    case POLY:  /* polygon definition */
	return("poly");
    case RECT:  /* rectangle definition */
	return("rectangle");
    case TEXT:  /* text definition */
	return("text");
    case INST:  /* recursive instance call */
    	return("instance");
    default:
	return("unknown");
    }
}

void trace(DB_DEFLIST *p, int level) {
    switch (p->type) {
    case ARC:  /* arc definition */
	break;
    case CIRC:  /* circle definition */
	break;
    case LINE:  /* line definition */
	printf("%d: line\n", level);
	break;
    case NOTE:  /* note definition */
	break;
    case OVAL:  /* oval definition */
	break;
    case POLY:  /* polygon definition */
	break;
    case RECT:  /* rectangle definition */
	break;
    case TEXT:  /* text definition */
	break;
    case INST:  /* recursive instance call */
	printf("%d: inst %s %g %g\n", level, p->u.i->name, p->u.i->x, p->u.i->y);
	break;
    default:
	break;
    }
}

// ---------------------------------------------------------
// Massive speed-up for hierarchies with lots of leaf cell
// reuse:
//
// we generate a sequence number with each top level redraw.  
// after we render any cell and have updated it's bounding box
// we tag it with the current sequence number.  Later on, in 
// the hierarchy, we don't rerender the cell again if it is
// below the nesting layer.  We just reuse the cached bounding
// box instead

static int seq=0;
int seqnum() {		// return current sequence number
   return (seq);
}

int nextseq() {		// generate a new sequence number
   seq++;
   return (seq);
}
// ---------------------------------------------------------

// return effective nest level for a cell
// considering logical or physical mode

int effective_nest(DB_TAB *cell, int nest) { 	
    if (currep->physical) {		// physical nesting
    	return(nest);
    } else {				// logical nesting
  	return(cell->logical_level);
    }
}

char msg[128];	//max size of dump output comments

int db_render(
    DB_TAB *cell,
    int nest,		/* nesting level */
    BOUNDS *bb,
    int mode 		/* drawing mode: one of D_READIN, D_NORM, D_RUBBER, D_BB, D_PICK */
) {
    extern XFORM *global_transform;
    extern XFORM unity_transform;

    DB_DEFLIST *p;
    XFORM *xp;
    XFORM *save_transform;
    // XFORM *xxp;
    int prims = 0;	// keep track of complexity of drawing

    DB_TAB *celltab;
    BOUNDS childbb;
    BOUNDS backbb;
    BOUNDS mybb;
    mybb.init=0; 
    backbb.init=0; 
    double xx, yy;
    double x, y;

    // int debug=0;

    if (aborted) {
       return(0);
    }

    if (cell == NULL) {
        printf("bad reference in db_render\n");
    	return(0);
    }
		
   // annotate the plot file with the coordinates of this cell
   // and some details like scale and rotation

   double theta;

   // transform (0.0,0.0) and (1.0,0.0) and subtract them
   x=1.0; y=0.0; 
   xx = x*global_transform->r11 + y*global_transform->r21 + global_transform->dx;
   yy = x*global_transform->r12 + y*global_transform->r22 + global_transform->dy;
   x=0.0; y=0.0; 
   xx -= x*global_transform->r11 + y*global_transform->r21 + global_transform->dx;
   yy -= x*global_transform->r12 + y*global_transform->r22 + global_transform->dy;
   theta=360.0*atan2(yy,xx)/(2.0*M_PI);

   // make 0.0<=theta<=360.0

   for(; theta<0.0; theta+=360.0) {
       ;
   }

   sprintf(msg,"label %f %f %s %d scale=%f, theta=%f", 
   	global_transform->dx,
	global_transform->dy,
	cell->name,
	nest,
	sqrt(xx*xx+yy*yy), theta);
   ps_comment(msg);

    /* clip to two pixels inside of actual window so we can see enclosing polys */
    clip_setwindow(2.0, 2.0, (double) (g_width-3), (double) (g_height-3));

    if (nest == 0) {	// at top of hierarchy
	unity_transform.r11 = 1.0;
	unity_transform.r12 = 0.0;
	unity_transform.r21 = 0.0;
	unity_transform.r22 = 1.0;
	unity_transform.dx  = 0.0;
	unity_transform.dy  = 0.0;
        global_transform = &unity_transform;
	nextseq();
    }


    if (effective_nest(cell,nest) > currep->nestlevel) { 
	drawon = 0;
    } else {
	drawon = 1; 
    }

    if (nest == 0) {
	mybb.xmax=0.0;		/* initialize globals to 0,0 location*/
	mybb.xmin=0.0;		/* draw() routine will modify these */
	mybb.ymax=0.0;
	mybb.ymin=0.0;
	mybb.init++;
    }

    // implement BACKground command

    if (nest == 0 && currep != NULL &&  currep->background != NULL) {
	db_render(db_lookup(currep->background), nest+1, &backbb, mode);
    }

    // when we only did physical nesting, we didn't
    // rerender anything that wasn't visible if it was already
    // rendered for this particular update sequence number.  Just trusted
    // the cached bounding box.  Now that we also do logic levels, we need
    // to render all the way down.

    // if ((cell->seqflag != seqnum()) || (nest <= currep->nestlevel)) {
    //
    if ((cell->seqflag != seqnum())||1){
	for (p=cell->dbhead; p!=(DB_DEFLIST *)0 && !aborted; p=p->next) {

	    childbb.init=0;
	    prims++;

	    // if (debug) printf("in do_%s\n", prim_name(p) );
	    // if (debug) trace(p, nest);

	    switch (p->type) {
	    case ARC:  /* arc definition */
		do_arc(p, &childbb, mode);
		break;
	    case CIRC:  /* circle definition */
		do_circ(p, &childbb, mode);
		break;
	    case LINE:  /* line definition */
		do_line(p, &childbb, mode);
		break;
	    case NOTE:  /* note definition */
		do_note(p, &childbb, mode);
		break;
	    case OVAL:  /* oval definition */
		do_oval(p, &childbb, mode);
		break;
	    case POLY:  /* polygon definition */
		do_poly(p, &childbb, mode);
		break;
	    case RECT:  /* rectangle definition */
		do_rect(p, &childbb, mode);
		break;
	    case TEXT:  /* text definition */
		do_text(p, &childbb, mode);
		break;
	    case INST:  /* recursive instance call */

		if( !show_check_visible(currep, INST,0)) {
		    ;
		}

		save_transform = global_transform;

		int row, col;				// RCW
		childbb.init=0;
		double dxc,dyc,dxr,dyr;

		for (row=0; row<p->u.i->opts->rows; row++) {		// array instance RCW
		    if (aborted) break;
		    for (col=0; col<p->u.i->opts->cols; col++) {		

			/* create transformation matrix from options */
			xp = matrix_from_opts(p->u.i->opts);

			dxc = (p->u.i->colx - p->u.i->x)/max(p->u.i->opts->cols-1.0, 1.0);
			dyc = (p->u.i->coly - p->u.i->y)/max(p->u.i->opts->cols-1.0, 1.0);
			dxr = (p->u.i->rowx- p->u.i->x)/max(p->u.i->opts->rows-1.0, 1.0);
			dyr = (p->u.i->rowy - p->u.i->y)/max(p->u.i->opts->rows-1.0, 1.0);

			// array instance offsets RCW	
			xp->dx += p->u.i->x+ ((double)row)*dxr + ((double)col)*dxc;
			xp->dy += p->u.i->y+ ((double)col)*dyc + ((double)row)*dyr;

			global_transform = compose(xp,save_transform);


    			if (effective_nest(cell,nest) > currep->nestlevel || 
				!show_check_visible(currep, INST, 0)) { 
			    drawon = 0;
			} else {
			    drawon = 1;
			}

			// instances are called by name, not by pointer, otherwise
			// it is possible to PURGE a cell in memory and then
			// reference a bad pointer, causing a crash...  so instances
			// are always accessed with a db_lookup() call

			// try to reread the definition from disk this can only
			// happen when a memory copy has been purged

			if (db_lookup(p->u.i->name) == NULL) {
			    loadrep(p->u.i->name);
			} 

			if ((celltab = db_lookup(p->u.i->name)) != NULL) {
			    prims += db_render(celltab, nest+1, &childbb, mode);
			} else {
			    printf("skipping load of %s, not in memory or disk\n", p->u.i->name);
			}

			/*
			// Display text for .<label> and @<label> here
			if (p->u.i->opts->sname != NULL) {
			    // need to set optstring to null if "@" opt has no extra characters
			    xxp = (XFORM *) emalloc(sizeof(XFORM)); 
			    xxp->r11=1.0; xxp->r12=0.0; xxp->r21=0.0;
			    xxp->r22=1.0; xxp->dx=0.0; xxp->dy=0.0;
			    mat_scale(xxp, 10.0, 10.0);
			    writestring(p->u.i->opts->sname, xxp , -1, 4, &childbb, mode);
			    free(xxp);
			}
			*/

			p->xmin = childbb.xmin;
			p->xmax = childbb.xmax;
			p->ymin = childbb.ymin;
			p->ymax = childbb.ymax;

			// p->xmin = min(childbb.xmin, p->xmin);
			// p->xmax = max(childbb.xmax, p->xmax);
			// p->ymin = min(childbb.ymin, p->ymin);
			// p->ymax = max(childbb.ymax, p->ymin);

			/* don't draw anything below nestlevel */
    			if (effective_nest(cell,nest) > currep->nestlevel) {
			    drawon = 0;
			} else {
			    drawon = 1;
			}

			/* if at nestlevel, draw bounding box */
    			if (effective_nest(cell,nest) == currep->nestlevel) {
			    set_layer(0,0);
			    if (mode != D_READIN) {
				db_drawbounds(childbb.xmin, childbb.ymin, 
				    childbb.xmax, childbb.ymax, D_BB);
			    } else {
				db_drawbounds(childbb.xmin, childbb.ymin, 
				    childbb.xmax, childbb.ymax, D_READIN);
			    }
			}


			free(global_transform); 
			free(xp);	
			global_transform = save_transform;	/* set transform back */
		    }  
		}

		break;
	    default:
		eprintf("unknown record type: %d in db_render\n", p->type);
		return(1);
		break;
	    }

	    p->xmin = childbb.xmin;
	    p->xmax = childbb.xmax;
	    p->ymin = childbb.ymin;
	    p->ymax = childbb.ymax;

	    /* now pass bounding box back */
	    db_bounds_update(&mybb, &childbb);
	}
    } else {
	childbb.init=0;
	jump(&mybb, D_BB);
	set_layer(0,0);
	draw(cell->minx, cell->maxy, &childbb, mode);
	draw(cell->maxx, cell->maxy, &childbb, mode);
	draw(cell->maxx, cell->miny, &childbb, mode);
	draw(cell->minx, cell->miny, &childbb, mode);
	draw(cell->minx, cell->maxy, &childbb, mode);
	draw(cell->maxx, cell->miny, &childbb, mode);
	prims = cell->prims;

	/* now pass bounding box back */
	db_bounds_update(&mybb, &childbb);
    }

    sprintf(msg,"} %d: %s %g %g %g %g", 
	nest, cell->name, childbb.xmin, childbb.ymin, childbb.xmax, childbb.ymax);
    ps_comment(msg);

    ps_flush();		// write all pending lines

    // this is used to create SVG imagemap rectangles
    double vxmin=mybb.xmin;
    double vxmax=mybb.xmax;
    double vymin=mybb.ymin;
    double vymax=mybb.ymax;

    R_to_V(&vxmin, &vymin);
    R_to_V(&vxmax, &vymax);

    ps_link(nest, cell->name, vxmin, vymin, vxmax, vymax); // RCW

    db_bounds_update(bb, &mybb);

    int dmode;
 
    if (nest == 0) { 
	if (currep->nestlevel == 0) {
	    if (mode == D_READIN) {
	       dmode = D_READIN;
	    } else {
	       dmode = D_BB;
	    }
	    jump(&mybb, dmode);
	    set_layer(12,0);
	    draw(bb->xmin, bb->ymax, &mybb, dmode);
	    draw(bb->xmax, bb->ymax, &mybb, dmode);
	    draw(bb->xmax, bb->ymin, &mybb, dmode);
	    draw(bb->xmin, bb->ymin, &mybb, dmode);
	    draw(bb->xmin, bb->ymax, &mybb, dmode);
	}

	/* update cell and globals for db_bounds() */
	cell->minx =  bb->xmin;
	cell->maxx =  bb->xmax;
	cell->miny =  bb->ymin;
	cell->maxy =  bb->ymax;
    } 
    cell->seqflag = seqnum();
    cell->prims = prims;
    return(prims);
}

void startpoly(BOUNDS *bb, int mode)
{
    filled_object = 1;		/* global for managing polygon filling */
    n_poly_points = 0;		/* number of points in filled polygon */
    // if (!X) {
    // 	ps_start_poly();
    // }
}

void endpoly(BOUNDS *bb, int mode) 
{
    clipl(2, 0.0, 0.0, bb, mode);	/* flush pipe */
    filled_object = 0;			/* global for managing polygon filling */
    if (X) {
        if (n_poly_points >=3) {
	    xwin_fill_poly(&Poly, n_poly_points); /* call XFillPolygon w/saved pts */
	}
    } else {
	// ps_end_poly();
    }
}

void savepoly(int x, int y)
{
    /* append points to Xwin Xpoint structure */
    Poly[n_poly_points].x = x;
    Poly[n_poly_points].y = y;
    if (n_poly_points++  > MAXPOLY) {
    	printf("exceeded maximum polygon size (MAXPOLY) in savepoly()\n");
	exit(1);
    }
}


void clip_setwindow(double x1, double y1, double x2, double y2)
{
    extern double xmin, ymin, xmax, ymax;
    if (x1 < x2) {
        xmin=x1;
        xmax=x2;
    } else {
        xmin=x2;
        xmax=x1;
    }
    if (y1 < y2) {
        ymin=y1;
        ymax=y2;
    } else {
        ymin=y2;
        ymax=y1;
    }
    if (debug) printf("#set clip window: %g %g %g %g\n", xmin, ymin, xmax, ymax);
}


void clipl(
    int init,
    double x,
    double y,
    BOUNDS *bb,	       /* bounding box */
    int mode	       /* D_NORM=regular, D_RUBBER=rubberband, */
		       /* D_BB=bounding box, D_PICK=pick checking */
) {
    static int npts=0;
    static double xold=0.0;
    static double yold=0.0;
    static double xorig, yorig;
    static int oldinside;
    double dx, dy, m;
    double bound;
    int inside, code;

    npts++;
	
    if (init == 1) {
    	npts = 0; xold = 0.0; yold = 0.0;
	clipr(1,0.0,0.0,bb,mode);  	/* CUSTOMIZE */
	return;
    }  

    if (debug) printf("#clipl got: %g %g: init %d\n", x, y, init);
    bound = xmin;

    if (init == 2) {		/* process closing segment */
       x = xorig; y = yorig;
    }

    inside = (x > bound);	/* CUSTOMIZE */
    code = oldinside+2*inside;
	
    if (npts == 1) {
	xorig = x; yorig = y;
    	if (inside) {
       	   clipr(0, x, y,bb,mode);  
	}
    } else if (npts > 1) {
       dx = x-xold; dy = y-yold;
       if (dx != 0.0) {
	  m = dy/dx;
       } else {
	  m = 1.0;
       }

       if (code == 0) {		/* both outside */
            ;
       } else if (code == 1) { 	/* leaving */
	    clipr(0, bound, yold + m*(bound-xold),bb,mode); 
       } else if (code == 2) { 	/* entering */
	    clipr(0, bound, yold + m*(bound-xold),bb,mode); 
	    clipr(0, x, y,bb,mode); 
       } else if (code == 3) {	/* both points inside */
	    clipr(0, x, y,bb,mode); 
       }
    } 

    if (init == 2) {		/* process closing segment */
	clipr(init, 0.0, 0.0 ,bb,mode); 
    }

    xold=x; yold=y;
    oldinside = inside;
}

void clipr(
    int init,
    double x,
    double y,
    BOUNDS *bb,		   /* bounding box */
    int mode		   /* D_NORM=regular, D_RUBBER=rubberband, */
		           /* D_BB=bounding box, D_PICK=pick checking */
) {
    static int npts=0;
    static double xold=0.0;
    static double yold=0.0;
    static double xorig, yorig;
    static int oldinside;
    double dx, dy, m;
    double bound;
    int inside, code;

    npts++;
	
    if (init == 1) {
    	npts = 0; xold = 0.0; yold = 0.0;
	clipt(1,0.0,0.0,bb,mode);  
	return;
    }  

    if (debug) printf("#clipr got: %g %g: init %d\n", x, y, init);
    bound = xmax;

    if (init == 2) {		/* process closing segment */
       x = xorig; y = yorig;
    }

    inside = (x < bound); 		/* CUSTOMIZE */
    code = oldinside+2*inside;

    if (npts == 1) {
	xorig = x; yorig = y;
    	if (inside) {
	   clipt(0, x, y,bb,mode); 
	}
    } else if (npts > 1) {
       dx = x-xold; dy = y-yold;
       if (dx != 0.0) {
	  m = dy/dx;
       } else {
	  m = 1.0;
       }

       if (code == 0) {		/* both outside */
	    ;
       } else if (code == 1) { 	/* leaving */
	    clipt(0, bound, yold + m*(bound-xold),bb,mode); 
       } else if (code == 2) { 	/* entering */
	    clipt(0, bound, yold + m*(bound-xold),bb,mode); 
	    clipt(0, x, y,bb,mode); 
       } else if (code == 3) {	/* both points inside */
	    clipt(0, x, y,bb,mode); 
       }
    } 

    if (init == 2) {		/* process closing segment */
	clipt(init, 0.0, 0.0 ,bb,mode);
    }

    xold=x; yold=y;
    oldinside = inside;
}

void clipt(
    int init,
    double x,
    double y,
    BOUNDS *bb,		   /* bounding box */
    int mode		   /* D_NORM=regular, D_RUBBER=rubberband, */
		           /* D_BB=bounding box, D_PICK=pick checking */
) {
    static int npts=0;
    static double xold=0.0;
    static double yold=0.0;
    static double xorig, yorig;
    static int oldinside;
    double dx, dy, m;
    double bound;
    int inside, code;

    npts++;
	
    if (init == 1) {
    	npts = 0; xold = 0.0; yold = 0.0;
	clipb(1,0.0,0.0,bb,mode);  
	return;
    }  

    if (debug) printf("#clipt got: %g %g: init %d\n", x, y, init);
    bound = ymax;

    if (init == 2) {		/* process closing segment */
       x = xorig; y = yorig;
    }

    inside = (y < bound); 	/* CUSTOMIZE */
    code = oldinside+2*inside;

    if (npts == 1) {
	xorig = x; yorig = y;
    	if (inside) {
	   clipb(0, x, y,bb,mode); 
	}
    } else if (npts > 1) {
       dx = x-xold; dy = y-yold;
       if (dx != 0.0) {
	  m = dy/dx;
       } else {
	  m = 1.0;
       }

       if (code == 0) {
	    ;
       } else if (code == 1) { 	/* leaving */
	    if (dx) {
		clipb(0, xold + (bound-yold)/m, bound,bb,mode);	 
	    } else {
		clipb(0, xold, bound,bb,mode); 
	    }
       } else if (code == 2) { 	/* entering */
	    if (dx) {
		clipb(0, xold + (bound-yold)/m, bound,bb,mode);	
	    } else {
		clipb(0, xold, bound,bb,mode);	
	    }
	    clipb(0, x, y,bb,mode);  
       } else if (code == 3) {	/* both points inside */
	    clipb(0, x, y,bb,mode);  
       }
    } 

    if (init == 2) {		/* process closing segment */ 
        clipb(init, 0.0, 0.0 ,bb,mode);
    }

    xold=x; yold=y;
    oldinside = inside;
}

void clipb(
    int init,
    double x,
    double y,
    BOUNDS *bb,		   /* bounding box */
    int mode		   /* D_NORM=regular, D_RUBBER=rubberband, */
		           /* D_BB=bounding box, D_PICK=pick checking */
) {
    static int npts=0;
    static double xold=0.0;
    static double yold=0.0;
    static double xorig, yorig;
    static int oldinside;
    double dx, dy, m;
    double bound;
    int inside, code;

    npts++;
	
    if (init == 1) {
    	npts = 0; xold = 0.0; yold = 0.0;
	emit(1,0.0,0.0,bb,mode);  
	return;
    }  

    if (debug) printf("#clipb got: %g %g: init %d\n", x, y, init);
    bound = ymin;

    if (init == 2) {		/* process closing segment */
       x = xorig; y = yorig;
    }

    inside = (y > bound);		/* CUSTOMIZE */
    code = oldinside+2*inside;

    if (npts == 1) {
	xorig = x; yorig = y;
    	if (inside) {
	   emit(0, x, y,bb,mode);  
	}
    } else if (npts > 1) {
       dx = x-xold; dy = y-yold;
       if (dx != 0.0) {
	  m = dy/dx;
       } else {
	  m = 1.0;
       }

       if (code == 0) {
            ;
       } else if (code == 1) { 	/* leaving */
	    if (dx) {
		emit(0, xold + (bound-yold)/m, bound,bb,mode);	
	    } else {
		emit(0, xold, bound,bb,mode); 
	    }
       } else if (code == 2) { 	/* entering */
	    if (dx) {
		emit(0, xold + (bound-yold)/m, bound,bb,mode);	
	    } else {
		emit(0, xold, bound,bb,mode);	
	    }
	    emit(0, x, y,bb,mode); 
       } else if (code == 3) {	/* both points inside */
	    emit(0, x, y,bb,mode); 
       }
    } 

    if (init == 2) {
         emit(init, 0.0, 0.0 ,bb,mode);
    }

    xold=x; yold=y;
    oldinside = inside;
}

void emit(
    int init,
    double x, double y,
    BOUNDS *bb,		   /* bounding box */
    int mode		   /* D_NORM=regular, D_RUBBER=rubberband, */
		           /* D_BB=bounding box, D_PICK=pick checking */
) {
    static int nseg=0; 
    static double xxold = 0.0;
    static double yyold = 0.0;
    static double xorig, yorig;
    double xp, yp;
    int debug=0;

    if (debug) printf("#emit got: %g %g\n", x, y);

    nseg++;

    if (nseg == 1) {
	xorig = x; yorig = y;
    }

    if (init == 1) {
    	nseg = 0;
    } else if (init == 2) {
       x = xorig; y = yorig;	/* close the polygon */
    }

    if (nseg && mode != D_READIN) {
	if (mode == D_PICK) {   /* no drawing just pick checking */

	    /* a bit of a hack, but we overload the bb structure */
	    /* to transmit the pick points ... pick comes in on */
	    /* xmin, ymin and a boolean 0,1 goes back on xmax */

	    /* transform the pick point into screen coords */
	    xp = bb->xmin;
	    yp = bb->ymin;
	    R_to_V(&xp, &yp);

	    bb->xmax += (double) pickcheck(xxold, yyold, x, y, xp, yp, 3.0);
	} else if ((mode == D_RUBBER)) { /* xor rubber band drawing */
	    if (drawon) {
		if (X && (nseg > 1)) {
		    xwin_rubber_line((int)xxold,(int)yyold,(int)x,(int)y);
		}
	    }
	} else {		/* regular drawing */
	    if (showon && drawon) {
		if (X) {
		    if (nseg > 1) {
			xwin_draw_line((int)xxold,(int)yyold,(int)x,(int)y);
		    }
		} else {
		    if (nseg == 1) {
			ps_start_line(x, y, filled_object);
		    } else {
			if (x!=xxold || y!=yyold) {
			    ps_continue_line(x, y);
			}
		    }
		}
		/* save coords for filling polygons later */
		if (filled_object && (draw_fill || layer_fill)) {
		    savepoly((int)x, (int)y);
		}
	    }
	}
    }
    xxold=x;
    yyold=y;
}

/***************** low-level X, autoplot primitives ******************/

/* The "nseg" variable is used to convert draw(x,y) calls into 
 * x1,y1,x2,y2 line segments.  The rule is: jump() sets nseg to zero, and
 * draw() calls XDrawLine with the xold,yold,x,y only if nseg>0.  "nseg"
 * is declared global because it must be shared by both draw() and
 * jump().  An alternative would have been to make all lines with a
 * function like drawline(x1,y1,x2,y2), but that would have been nearly
 * 2x more inefficient when drawing lines with more than two nodes (all
 * shared nodes get sent twice).  By using jump() and sending the points
 * serially, we have the option of improving the efficiency later by
 * building a linked list and using XDrawLines() rather than
 * XDrawLine().  In addition, the draw()/jump() paradigm is used by Bob
 * Jewett's autoplot(1) program, so has a long history. 
 */

static int nseg=0;

void draw(                 /* draw x,y transformed by extern global xf */
    NUM x, NUM y,	   /* location in real coordinate space */
    BOUNDS *bb,		   /* bounding box */
    int mode		   /* D_NORM=regular, D_RUBBER=rubberband, */
			   /* D_BB=bounding box, D_PICK=pick checking */
) {
    extern XFORM *global_transform;	/* global for efficiency */
    NUM xx, yy;
    int debug=0;

    if (debug) printf("in draw, mode=%d, x=%g, y=%g\n", mode, x, y);

    if (mode == D_BB) {	
	/* skip screen transform to draw bounding */
	/* boxes in nested hierarchies, also don't */
	/* update bounding boxes (sort of a free */
	/* drawing mode in absolute coordinates) */
	xx = x;
	yy = y;
    } else if (mode==D_NORM || mode==D_RUBBER || mode==D_PICK || mode == D_READIN) {
	/* compute transformed coordinates */
	xx = x*global_transform->r11 + 
	    y*global_transform->r21 + global_transform->dx;
	yy = x*global_transform->r12 + 
	    y*global_transform->r22 + global_transform->dy;
	if (mode == D_NORM || mode == D_READIN) {
	    /* merge bounding boxes */
	    if(bb->init == 0) {
		bb->xmin=bb->xmax=xx;
		bb->ymin=bb->ymax=yy;
		bb->init++;
	    } else {
		bb->xmax = max(xx,bb->xmax);
		bb->xmin = min(xx,bb->xmin);
		bb->ymax = max(yy,bb->ymax);
		bb->ymin = min(yy,bb->ymin);
	    }
	} 
    } else {
    	printf("bad mode: %d in draw() function\n", mode);
	exit(1);
    }

    /* Before I added the clip() routine, there was a problem with  */
    /* extreme zooming on a large drawing.  When the transformed points */
    /* exceeded ~2^18, the X11 routines would overflow and the improperly */
    /* clipped lines would alias back onto the screen...  Adding the */
    /* Cohen-Sutherland (and later Cohen-Hodgman) clipper completely */
    /* eliminated this problem.  I couldn't find the dynamic range of Xlib */
    /* documented anywhere.  I was surprised that the range was not equal */
    /* to the full 2^32 bit range of an unsigned int (RCW). Note: 12/28/2004 */
    /* I did see a reference to a 16 bit limitation for primitives in Xlib */
    /* in the O'Reilly book */

    if (mode != D_READIN) {
	if (!nseg) {
	    clipl(1, 0.0, 0.0, bb, mode);	/* initialize pipe */
	} 
	R_to_V(&xx, &yy);	/* convert to screen coordinates */ 
	if (!X) {
	    /* for postscript, flip y polarity */
	    // now flipped in postscript.c
	    // yy = (double) g_height - yy;    
	}
	clipl(0, xx, yy, bb, mode);	
    }
    nseg++;
}

/* pickcheck(): called with a line segment L=(x1,y1),
 * (x2, y2) a point (x3, y3) and a fuzz factor eps, returns 1 if p3 is
 * within a rectangular bounding box eps larger than L, 0 otherwise. 
 *
 * ...a bit of a hack, but we overload the bb structure to transmit the
 * pick points x3,y3 ...  pick comes in on x3=bb->xmin, y3=bb->ymin and
 * a boolean 0,1 goes back on bb->xmax
 */

int pickcheck(
    double x1,
    double y1,
    double x2,
    double y2,
    double x3,
    double y3,
    double eps
) {
    double xn,yn, xp,yp, xr,yr;
    double d,s,c;

    /* normalize everything to x1,y1 */
    xn = x2-x1; yn = y2-y1;
    xp = x3-x1; yp = y3-y1;

    /* rotate pick point to vertical */
    d = sqrt(xn*xn+yn*yn);
    s = xn/d;		/* sin() */
    c = yn/d;		/* cos() */
    xr = xp*c - yp*s;

    /* test against a rectangle */
    if (xr >= -eps && xr <= eps) {
	/* save some time by computing */
	/* yr only after testing x */
	yr = yp*c + xp*s;
	if (yr >= -eps  && yr <= d+eps) {
	    return(1);
	}
    }
    return(0);
}

void jump(BOUNDS *bb, int mode) 
{
    nseg=0;  
    filled_object = 0;		/* automatically close polygon */
}


void set_layer(
    int lnum,	/* layer number */
    int comp	/* component type */
) {
    extern int showon;

    if (comp) {
	showon = show_check_visible(currep, comp, lnum);
    } else {
        showon=1;
    }

    layer_fill = (equate_get_fill(lnum) > 0);	

    // if win:o<layer> has overridden the fill, then make it solid
    if (equate_get_override(lnum)) {
    	layer_fill = 1;
    }

    if (X) {
	xwin_set_pen_line_fill(  
		equate_get_color(lnum), 
		equate_get_linetype(lnum), 
		equate_get_fill(lnum) 
		);	
    } else {
	ps_set_layer(lnum);
        ps_set_pen(equate_get_color(lnum));	
	ps_set_line(equate_get_linetype(lnum));
	ps_set_fill(equate_get_fill(lnum));	//RCW/

	//if (layer_fill) {
	//    ps_set_fill(equate_get_fill(lnum));
	//} else {
	//    ps_set_fill(1);
	//}
    }
}

double max(double a, double b) 
{
    return(a>=b?a:b);
}

double min(double a, double b) 
{
    return(a<=b?a:b);
}

