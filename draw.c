#include <math.h>
#include <X11/Xlib.h>
/*
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
*/

#include "db.h"
#include "rubber.h"
#include "xwin.h"
#include "postscript.h"
#include "eprintf.h"
#include "equate.h"


#define FUZZBAND 0.01	/* how big the fuzz around lines are */
                        /* compared to minimum window dimension */
			/* for the purpose of picking objects */

/* global variables that need to eventually be on a stack for nested edits */
int drawon=1;		  /* 0 = dont draw, 1 = draw (used in nesting)*/
int showon=1;		  /* 0 = layer currently turned off */
int nestlevel=9;
int draw_fill=FILL_OFF;
int layer_fill=FILL_OFF;
int X=1;		  /* 1 = draw to X, 0 = emit autoplot commands */
FILE *PLOT_FD;		  /* file descriptor for plotting */

int filled_object = 0;		/* global for managing polygon filling */
int n_poly_points = 0;		/* number of points in filled polygon */

#define MAXPOLY 1024		/* maximum polygon size */
XPoint Poly[MAXPOLY];

			
void clipl();   	  /* polygon clipping pipeline: clip left side */
void clipr();   	  /* clip right side */
void clipt();		  /* clip top */
void clipb();		  /* clip bottom */

int pickcheck();
void emit();
int debug = 0;

/****************************************************/
/* rendering routines				    */
/* db_render() sets globals xmax, ymax, xmin, ymin; */
/****************************************************/

void db_set_fill(fill) 
int fill;
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

void db_set_nest(nest) 
int nest;
{
    extern int nestlevel;
    nestlevel = nest;
}

void db_bounds_update(mybb, childbb) 
BOUNDS *mybb;
BOUNDS *childbb;
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

double dist(dx,dy) 
double dx,dy;
{
    dx = dx<0?-dx:dx; /* absolute value */
    dy = dy<0?-dy:dy;

    if (dx > dy) {
	return(dx + 0.3333*dy);
    } else {
	return(dy + 0.3333*dx);
    }
}


/* called with pick point x,y and bounding box in p return zero if xy */
/* inside bb, otherwise return a number  between zero and 1 */
/* monotonically related to inverse distance of xy from bounding box */

double bb_distance(x, y, p, fuzz)
double x,y;
DB_DEFLIST *p;
double fuzz;
{
    int outcode;
    double retval;
    outcode = 0;

    /*  9 8 10
	1 0 2
	5 4 6 */

    /* FIXME - this check can eventually go away for speed */
    if (p->xmin > p->xmax) { printf("bad!\n"); }
    if (p->ymin > p->ymax) { printf("bad!\n"); }

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
 *    CASE 1: ; pick point is outside of BB, higher score the 
 *      closer to BB
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


DB_DEFLIST *db_ident(cell, x, y, mode, pick_layer, comp, name)
DB_TAB *cell;			/* device being edited */
double x, y;	 		/* pick point */
int mode; 			/* 0=ident, 1=pick */
int pick_layer;			/* layer restriction, or 0=all */
int comp;			/* comp restriction */
char *name;			/* instance name restrict or NULL (not used)*/
{
    DB_DEFLIST *p;
    DB_DEFLIST *p_best = NULL;
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

	bad_comp=0;
	if ((pick_layer != 0 && layer != pick_layer) ||
	    (comp != 0 && comp != ALL && p->type != comp)  ||
	    (mode == 0 && !show_check_visible(currep, p->type, layer)) ||
	    (mode == 1 && !show_check_modifiable(currep, p->type, layer))) {
		bad_comp++;
	}

	if (!bad_comp) {
	    if ((pick_score=bb_distance(x,y,p,fuzz)) < 0.0) { 	/* inside BB */

		/* pick point inside BB gives a score ranging from */
		/* 1.0 to 2.0, with smaller BB's getting higher score */

		pick_score= 1.0 + 1.0/(1.0+min(fabs(p->xmax-p->xmin),
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
	    }
	}

	/* keep track of the best of the lot */
	if (pick_score > pick_best) {
	    if (debug) printf("pick_score %g, best %g\n", pick_score, pick_best);
	    fflush(stdout);
	    pick_best=pick_score;
	    p_best = p;
	}
    }

    return(p_best);
}

void db_drawbounds(xmin, ymin, xmax, ymax, mode)
double xmin, ymin, xmax, ymax;
int mode;
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

double db_area(p)
DB_DEFLIST *p;			/* return the area of component p */
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
	r2 = pow(p->u.c->x1 - p->u.c->x2, 2.0) +  pow(p->u.c->y2 - p->u.c->y1, 2.0);
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

void db_notate(p)
DB_DEFLIST *p;			/* print out identifying information */
{

    if (p == NULL) {
    	printf("db_notate: no component!\n");
    }

    switch (p->type) {
    case ARC:  /* arc definition */
	printf("   ARC %d end1=%.5g,%.5g end2=%.5g,%.5g point_on_circumference=%.5g,%.5g ", 
		p->u.a->layer, p->u.a->x1, p->u.a->y1, p->u.a->x2, p->u.a->y2,
		p->u.a->x3, p->u.a->y3);
	db_print_opts(stdout, p->u.a->opts, ARC_OPTS);
	printf("\n");
	break;
    case CIRC:  /* circle definition */
	printf("   CIRC %d center=%.5g,%.5g point_on_circumference=%.5g,%.5g ", 
		p->u.c->layer, p->u.c->x1, p->u.c->y1, p->u.c->x2, p->u.c->y2);
	db_print_opts(stdout, p->u.c->opts, CIRC_OPTS);
	printf("\n");
	break;
    case LINE:  /* line definition */
	printf("   LINE %d LL=%.5g,%.5g UR=%.5g,%.5g ", 
		p->u.l->layer, p->xmin, p->ymin, p->xmax, p->ymax);
	db_print_opts(stdout, p->u.l->opts, LINE_OPTS);
	printf("\n");
	break;
    case NOTE:  /* note definition */
	printf("   NOTE %d LL=%.5g,%.5g UR=%.5g,%.5g ", 
		p->u.n->layer, p->xmin, p->ymin, p->xmax, p->ymax);
	db_print_opts(stdout, p->u.n->opts, NOTE_OPTS);
	printf(" \"%s\"\n", p->u.n->text);
	break;
    case OVAL:  /* oval definition */
	break;
    case POLY:  /* polygon definition */
	printf("   POLY %d LL=%.5g,%.5g UR=%.5g,%.5g ", 
		p->u.p->layer, p->xmin, p->ymin, p->xmax, p->ymax);
	db_print_opts(stdout, p->u.p->opts, POLY_OPTS);
	printf("\n");
	break;
    case RECT:  /* rectangle definition */
	printf("   RECT %d LL=%.5g,%.5g UR=%.5g,%.5g ", 
		p->u.r->layer, p->xmin, p->ymin, p->xmax, p->ymax);
	db_print_opts(stdout, p->u.r->opts, RECT_OPTS);
	printf("\n");
	break;
    case TEXT:  /* text definition */
	printf("   TEXT %d LL=%.5g,%.5g UR=%.5g,%.5g ", 
		p->u.t->layer, p->xmin, p->ymin, p->xmax, p->ymax);
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

void db_highlight(p)
DB_DEFLIST *p;			/* component to display */
{
    BOUNDS bb;
    bb.init=0;

    if (p == NULL) {
    	printf("db_highlight: no component!\n");
    }

    switch (p->type) {
    case ARC:  /* arc definition */
	db_drawbounds(p->xmin, p->ymin, p->xmax, p->ymax, D_RUBBER);
	do_arc(p, &bb, D_RUBBER);
	break;
    case CIRC:  /* circle definition */
	db_drawbounds(p->xmin, p->ymin, p->xmax, p->ymax, D_RUBBER);
	do_circ(p, &bb, D_RUBBER);
	break;
    case LINE:  /* line definition */
	do_line(p, &bb, D_RUBBER);
	break;
    case NOTE:  /* note definition */
	db_drawbounds(p->xmin, p->ymin, p->xmax, p->ymax, D_RUBBER);
	do_note(p, &bb, D_RUBBER);
	break;
    case OVAL:  /* oval definition */
	db_drawbounds(p->xmin, p->ymin, p->xmax, p->ymax, D_RUBBER);
	do_oval(p, &bb, D_RUBBER);
	break;
    case POLY:  /* polygon definition */
	do_poly(p, &bb, D_RUBBER);
	break;
    case RECT:  /* rectangle definition */
	do_rect(p, &bb, D_RUBBER);
	break;
    case TEXT:  /* text definition */
	db_drawbounds(p->xmin, p->ymin, p->xmax, p->ymax, D_RUBBER);
	do_note(p, &bb, D_RUBBER);
	break;
    case INST:  /* instance call */
	db_drawbounds(p->xmin, p->ymin, p->xmax, p->ymax, D_RUBBER);
	break;
    default:
	eprintf("unknown record type: %d in db_highlight\n", p->type);
	break;
    }
}

int db_list(cell)
DB_TAB *cell;
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
    if (currep->modified) {
	printf("   cell is modified\n");
    } else {
	printf("   cell is not modified\n");
    }

    return(1);
}

int db_plot() {
    BOUNDS bb;
    bb.init=0;
    char buf[MAXFILENAME];

    if (currep == NULL) {
    	printf("not currently editing any rep!\n");
	return(0);
    }

    snprintf(buf, MAXFILENAME, "%s.ps", currep->name);
    printf("plotting postcript to %s\n", buf);
    fflush(stdout);

    if((PLOT_FD=fopen(buf, "w+")) == 0) {
    	printf("db_plot: can't open plotfile: \"%s\"\n", buf);
	return(0);
    } 

    X=0;				    /* turn X display off */
    ps_preamble(PLOT_FD, currep->name, "piglet version 0.8",
	8.5, 11.0, currep->vp_xmin, currep->vp_ymin, currep->vp_xmax, currep->vp_ymax);
    db_render(currep, 0, &bb, D_NORM);  /* dump plot */
    ps_postamble(PLOT_FD);
    X=1;				    /* turn X back on*/
    return(1);
}


int db_render(cell, nest, bb, mode)
DB_TAB *cell;
int nest;	/* nesting level */
BOUNDS *bb;
int mode; 	/* drawing mode: one of D_NORM, D_RUBBER, D_BB, D_PICK */
{
    extern XFORM *global_transform;
    extern XFORM unity_transform;

    DB_DEFLIST *p;
    XFORM *xp;
    XFORM *save_transform;
    extern int nestlevel;
    int debug=0;

    BOUNDS childbb;
    BOUNDS mybb;
    mybb.init=0; 

    if (cell == NULL) {
        printf("bad reference in db_render\n");
    	return(0);
    }

    if (nest == 0) {
	unity_transform.r11 = 1.0;
	unity_transform.r12 = 0.0;
	unity_transform.r21 = 0.0;
	unity_transform.r22 = 1.0;
	unity_transform.dx  = 0.0;
	unity_transform.dy  = 0.0;
        global_transform = &unity_transform;
    }

    if (!X && (nest == 0)) {	/* autoplot output */
	/*
	fprintf(PLOT_FD, "nogrid\n");
	fprintf(PLOT_FD, "isotropic\n");
	fprintf(PLOT_FD, "back\n");
	*/
    }

    if (nest == 0) {
	mybb.xmax=0.0;		/* initialize globals to 0,0 location*/
	mybb.xmin=0.0;		/* draw() routine will modify these */
	mybb.ymax=0.0;
	mybb.ymin=0.0;
	mybb.init++;
    }

    for (p=cell->dbhead; p!=(DB_DEFLIST *)0; p=p->next) {

	childbb.init=0;

	switch (p->type) {
        case ARC:  /* arc definition */
	    if (debug) printf("in do_arc\n");
	    do_arc(p, &childbb, mode);
	    break;
        case CIRC:  /* circle definition */
	    if (debug) printf("in do_circ\n");
	    do_circ(p, &childbb, mode);
	    break;
        case LINE:  /* line definition */
	    if (debug) printf("in do_line\n");
	    do_line(p, &childbb, mode);
	    break;
        case NOTE:  /* note definition */
	    if (debug) printf("in do_note\n");
	    do_note(p, &childbb, mode);
	    break;
        case OVAL:  /* oval definition */
	    if (debug) printf("in do_oval\n");
	    do_oval(p, &childbb, mode);
	    break;
        case POLY:  /* polygon definition */
	    if (debug) printf("in do_poly\n");
	    do_poly(p, &childbb, mode);
	    if (debug) printf("poly bounds = %.5g,%.5g %.5g,%.5g\n",
		childbb.xmin, childbb.ymin, childbb.xmax, childbb.ymax);
	    break;
	case RECT:  /* rectangle definition */
	    if (debug) printf("in do_rect\n");
	    do_rect(p, &childbb, mode);
	    break;
        case TEXT:  /* text definition */
	    if (debug) printf("in do_text\n");
	    do_text(p, &childbb, mode);
	    break;
        case INST:  /* recursive instance call */
	    if (debug) printf("in do_inst\n");

	    if( !show_check_visible(currep, INST,0)) {
	    	;
	    }

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

	    /* NOTE: To work properly, these transformations have to */
	    /* occur in the proper order, for example, rotation must come */
	    /* after slant transformation */

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

	    mat_scale(xp, p->u.i->opts->scale, p->u.i->opts->scale); 
	    mat_scale(xp, 1.0/p->u.i->opts->aspect_ratio, 1.0); 
	    mat_slant(xp,  p->u.i->opts->slant); 
	    mat_rotate(xp, p->u.i->opts->rotation);
	    xp->dx += p->u.i->x;
	    xp->dy += p->u.i->y;

	    save_transform = global_transform;
	    global_transform = compose(xp,global_transform);

	    if (nest >= nestlevel || !show_check_visible(currep, INST,0)) {
		drawon = 0;
	    } else {
		drawon = 1;
	    }

	    /* instances are called by name, not by pointer */
	    /* otherwise, it is possible to PURGE a cell in */
	    /* memory and then reference a bad pointer, causing */
	    /* a crash... so instances are always accessed */
	    /* with a db_lookup() call */

	    childbb.init=0;

	    if (db_lookup(p->u.i->name) == NULL) {

		printf("skipping reference to %s, no longer in memory\n", p->u.i->name);

	    	/* FIXME try to reread the definition from disk */
		/* this can only happen when a memory copy has */
		/* been purged */

	    } else {
		db_render(db_lookup(p->u.i->name), nest+1, &childbb, mode);
	    }

	    p->xmin = childbb.xmin;
	    p->xmax = childbb.xmax;
	    p->ymin = childbb.ymin;
	    p->ymax = childbb.ymax;

	    /* don't draw anything below nestlevel */
	    if (nest > nestlevel) {
		drawon = 0;
	    } else {
		drawon = 1;
	    }

            if (nest == nestlevel) { /* if at nestlevel, draw bounding box */
		set_layer(0,0);
		db_drawbounds(childbb.xmin, childbb.ymin, childbb.xmax, childbb.ymax, D_BB);
	    }

	    free(global_transform); free(xp);	
	    global_transform = save_transform;	/* set transform back */

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

    db_bounds_update(bb, &mybb);
 
    if (nest == 0) { 
	jump(&mybb, D_BB);
	/*
	set_layer(12,0);
	draw(bb->xmin, bb->ymax, &mybb, D_BB);
	draw(bb->xmax, bb->ymax, &mybb, D_BB);
	draw(bb->xmax, bb->ymin, &mybb, D_BB);
	draw(bb->xmin, bb->ymin, &mybb, D_BB);
	draw(bb->xmin, bb->ymax, &mybb, D_BB);
	*/

	/* update cell and globals for db_bounds() */
	cell->minx =  bb->xmin;
	cell->maxx =  bb->xmax;
	cell->miny =  bb->ymin;
	cell->maxy =  bb->ymax;
    } 
    return(1);
}

void startpoly(bb,mode)
BOUNDS *bb;
int mode;
{
    filled_object = 1;		/* global for managing polygon filling */
    n_poly_points = 0;		/* number of points in filled polygon */
}

void endpoly(bb,mode) 
BOUNDS *bb;
int mode;
{
    clipl(2, 0.0, 0.0, bb, mode);	/* flush pipe */
    filled_object = 0;		/* global for managing polygon filling */
    if (n_poly_points >= 3) {
	xwin_fill_poly(&Poly, n_poly_points); /* call XFillPolygon with saved points */
    }
}

void savepoly(x, y)
int x;
int y;
{
    /* append points to Xwin Xpoint structure */
    Poly[n_poly_points].x = x;
    Poly[n_poly_points].y = y;
    if (n_poly_points++  > MAXPOLY) {
    	printf("exceeded maximum polygon size (MAXPOLY) in savepoly()\n");
	exit(1);
    }
}

void clipl(init, x, y, bb, mode) 
int init;
double x;
double y;
BOUNDS *bb;		   /* bounding box */
int mode;		   /* D_NORM=regular, D_RUBBER=rubberband, */
			   /* D_BB=bounding box, D_PICK=pick checking */
{
    int debug=0;
    static int npts=0;
    static int nout;
    static double xold=0.0;
    static double yold=0.0;
    static double xorig, yorig;
    double dx, dy, m;
    static int oldstate;
    double bound;
    int state, code;

    npts++;
	
    if (init == 1) {
    	npts = 0; nout = 0;
	/* init next in chain: l,r,t,b */
	if (debug) printf("#clipl initialized\n");
	clipr(1,0.0,0.0,bb,mode);  	/* CUSTOMIZE */
	return;
    }  

    if (debug) printf("#clipl got: %g %g\n", x, y);
    bound = -10.0;	/* CUSTOMIZE */

    if (init == 2) {		/* process closing segment */
       x = xorig;
       y = yorig;
    }

    state = (x > bound);
    code = oldstate+2*state;
	
    if (npts == 1) {
	xorig = x; yorig = y;
    	if (state) {
       	    clipr(0, x, y,bb,mode);  nout++;
	}
    } else if (npts > 1) {
       dx = x-xold; dy = y-yold;
       if (dx != 0.0) {
	  m = dy/dx;
       } else {
	  m = 1.0;
       }

       if (code == 0) {
	    ; /* do nothing */
       } else if (code == 1) { 	/* leaving */
	    clipr(0, bound, yold + m*(bound-xold),bb,mode); nout++;
       } else if (code == 2) { 	/* entering */
	    clipr(0, bound, yold + m*(bound-xold),bb,mode); nout++;
	    clipr(0, x, y,bb,mode); nout++;			
       } else if (code == 3) {	/* both points inside */
	    clipr(0, x, y,bb,mode); nout++;
       }
    } 

    if (init == 2) {		/* process closing segment */
 	clipr(init, 0.0, 0.0 ,bb,mode); 
    }

    xold=x; yold=y;
    oldstate = state;
}

void clipr(init, x, y, bb, mode) 
int init;
double x;
double y;
BOUNDS *bb;		   /* bounding box */
int mode;		   /* D_NORM=regular, D_RUBBER=rubberband, */
			   /* D_BB=bounding box, D_PICK=pick checking */
{
    static int npts=0;
    static int nout;
    static double xold=0.0;
    static double yold=0.0;
    static double xorig, yorig;
    double dx, dy, m;
    static int oldstate;
    double bound;
    int state, code;

    npts++;
	
    if (init == 1) {
    	npts = 0; nout = 0;
	xold = 0.0;
	yold = 0.0;
	/* init next in chain: l,r,t,b */
	if (debug) printf("#clipr initialized\n");
	clipt(1,0.0,0.0,bb,mode);  nout++;
	return;
    }  

    if (debug) printf("#clipr got: %g %g\n", x, y);
    bound = (double) (g_width+10); 		/* CUSTOMIZE */

    if (init == 2) {		/* process closing segment */
       x = xorig;
       y = yorig;
    }

    state  = (x < bound); 		/* CUSTOMIZE */
    code = oldstate+2*state;

    if (npts == 1) {
	xorig = x; yorig = y;
    	if (state) {
	   clipt(0, x, y,bb,mode); nout++;
	}
    } else if (npts > 1) {
       dx = x-xold;
       dy = y-yold;
       if (dx != 0.0) {
	  m = dy/dx;
       } else {
	  m = 1.0;
       }

       if (code == 0) {
	    ; /* do nothing */
       } else if (code == 1) { 	/* leaving */
	    clipt(0, bound, yold + m*(bound-xold),bb,mode); nout++;
       } else if (code == 2) { 	/* entering */
	    clipt(0, bound, yold + m*(bound-xold),bb,mode); nout++;
	    clipt(0, x, y,bb,mode); nout++;			
       } else if (code == 3) {	/* both points inside */
	    clipt(0, x, y,bb,mode); nout++;
       }
    } 

    if (init == 2) {		/* process closing segment */
 	clipt(init, 0.0, 0.0 ,bb,mode); 
    }

    xold=x; yold=y;
    oldstate = state;
}

void clipt(init, x, y, bb, mode) 
int init;
double x;
double y;
BOUNDS *bb;		   /* bounding box */
int mode;		   /* D_NORM=regular, D_RUBBER=rubberband, */
			   /* D_BB=bounding box, D_PICK=pick checking */
{
    static int npts=0;
    static int nout;
    static double xold=0.0;
    static double yold=0.0;
    static double xorig, yorig;
    double dx, dy, m;
    static int oldstate;
    double bound;
    int state, code;

    npts++;
	
    if (init == 1) {
    	npts = 0; nout = 0;
	xold = 0.0;
	yold = 0.0;
	/* init next in chain: l,r,t,b */
	if (debug) printf("#clipt initialized\n");
	clipb(1,0.0,0.0,bb,mode);  nout++;
	return;
    }  

    if (debug) printf("#clipt got: %g %g\n", x, y);
    bound = (double) (g_height + 10); 		/* CUSTOMIZE */

    if (init == 2) {		/* process closing segment */
       x = xorig;
       y = yorig;
    }

    state = (y < bound); 	/* CUSTOMIZE */
    code = oldstate+2*state;

    if (npts == 1) {
	xorig = x; yorig = y;
    	if (state) {
	   clipb(0, x, y,bb,mode); 
	}
    } else if (npts > 1) {
       dx = x-xold;
       dy = y-yold;
       if (dx != 0.0) {
	  m = dy/dx;
       } else {
	  m = 1.0;
       }

       if (code == 0) {
	    ; /* do nothing */
       } else if (code == 1) { 	/* leaving */
	    if (dx) {
		clipb(0, xold + (bound-yold)/m, bound,bb,mode);	 nout++;
	    } else {
		clipb(0, xold, bound,bb,mode); nout++;
	    }
       } else if (code == 2) { 	/* entering */
	    if (dx) {
		clipb(0, xold + (bound-yold)/m, bound,bb,mode);	nout++;
	    } else {
		clipb(0, xold, bound,bb,mode);	nout++;
	    }
	    clipb(0, x, y,bb,mode);  nout++;
       } else if (code == 3) {	/* both points inside */
	    clipb(0, x, y,bb,mode);  nout++;
       }
    } 

    if (init == 2) {		/* process closing segment */
 	clipb(init, 0.0, 0.0 ,bb,mode); 
    }

    xold=x; yold=y;
    oldstate = state;
}

void clipb(init, x, y, bb, mode) 
int init;
double x;
double y;
BOUNDS *bb;		   /* bounding box */
int mode;		   /* D_NORM=regular, D_RUBBER=rubberband, */
			   /* D_BB=bounding box, D_PICK=pick checking */
{
    static int npts=0;
    static int nout;
    static double xold=0.0;
    static double yold=0.0;
    static double xorig, yorig;
    double dx, dy, m;
    static int oldstate;
    double bound;
    int state, code;

    npts++;
	
    if (init == 1) {
    	npts = 0; nout = 0;
	xold = 0.0;
	yold = 0.0;
	/* init next in chain: l,r,t,b */
	if (debug) printf("#clipb initialized\n");
	emit(1,0.0,0.0,bb,mode,bb,mode);  nout++;
	return;
    }  

    if (debug) printf("#clipb got: %g %g\n", x, y);
    bound = -10.0; 		/* CUSTOMIZE */

    if (init == 2) {		/* process closing segment */
       x = xorig;
       y = yorig;
    }

    state = (y > bound);		/* CUSTOMIZE */
    code = oldstate+2*state;

    if (npts == 1) {
	xorig = x; yorig = y;
    	if (state) {
	   emit(0, x, y,bb,mode);  nout++;
	}
    } else if (npts > 1) {
       dx = x-xold;
       dy = y-yold;
       if (dx != 0.0) {
	  m = dy/dx;
       } else {
	  m = 1.0;
       }

       if (code == 0) {
	    ; /* do nothing */
       } else if (code == 1) { 	/* leaving */
	    if (dx) {
		emit(0, xold + (bound-yold)/m, bound,bb,mode);	nout++;
	    } else {
		emit(0, xold, bound,bb,mode); nout++;
	    }
       } else if (code == 2) { 	/* entering */
	    if (dx) {
		emit(0, xold + (bound-yold)/m, bound,bb,mode);	nout++;
	    } else {
		emit(0, xold, bound,bb,mode);	nout++;
	    }
	    emit(0, x, y,bb,mode); nout++;
       } else if (code == 3) {	/* both points inside */
	    emit(0, x, y,bb,mode); nout++;
       }
    } 

    xold=x; yold=y;
    oldstate = state;
}

void emit(init, x, y, bb, mode) 
int init;
double x, y;
BOUNDS *bb;		   /* bounding box */
int mode;		   /* D_NORM=regular, D_RUBBER=rubberband, */
			   /* D_BB=bounding box, D_PICK=pick checking */
{
    static int nseg=0; 
    static double xxold, yyold;
    double xp, yp;
    int debug=0;

    nseg++;

    if (init == 1) {
    	nseg = 0;
    } 

    if (debug) printf("#emit got: %g %g\n", x, y);

    if (nseg && mode != D_READIN) {
	if (X) {
	    if (mode == D_PICK) {   /* no drawing just pick checking */

		/* a bit of a hack, but we overload the bb structure */
		/* to transmit the pick points ... pick comes in on */
		/* xmin, ymin and a boolean 0,1 goes back on xmax */

		/* transform the pick point into screen coords */
		xp = bb->xmin;
		yp = bb->ymin;
		R_to_V(&xp, &yp);

		bb->xmax += (double) pickcheck(xxold, yyold, x, y, xp, yp, 3.0);

	    } else if (mode == D_RUBBER) { /* xor rubber band drawing */
		if (showon && drawon) {
		    if (nseg > 1) {
		        xwin_xor_line((int)xxold,(int)yyold,(int)x,(int)y);
		    }
		}
	    } else {		/* regular drawing */
		if (showon && drawon) {
		    if (nseg > 1) {
		        xwin_draw_line((int)xxold,(int)yyold,(int)x,(int)y);
		    }
		    /* save coords for filling polygons later */
		    if (filled_object && (draw_fill || layer_fill)) {
		        savepoly((int)x, (int)y);
		    }
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

void draw(x, y, bb, mode)  /* draw x,y transformed by extern global xf */
NUM x,y;		   /* location in real coordinate space */
BOUNDS *bb;		   /* bounding box */
int mode;		   /* D_NORM=regular, D_RUBBER=rubberband, */
			   /* D_BB=bounding box, D_PICK=pick checking */
{
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
    /* Cohen-Sutherland clipper completely eliminated this problem. */
    /* I couldn't find the dynamic range of Xlib documented anywhere. */
    /* I was surprised that the range was not equal to the full 2^32 bit */
    /* range of an unsigned int (RCW). Note: 12/28/2004 I did see a reference */
    /* to a 16 bit limitation for primitives in Xlib in the O'Reilly book */

    if (mode != D_READIN) {
	if (X) {
	    R_to_V(&xx, &yy);	/* convert to screen coordinates */ 
	    if (!nseg) {
	    	clipl(1, 0.0, 0.0, bb, mode);	/* initialize pipe */
	    }
	    clipl(0, xx, yy, bb, mode);	
	} else {	/* postscript output */
	    if (nseg) {
		/* autoplot output */
		/* if (showon && drawon) fprintf(PLOT_FD, 
		     "%4.6g %4.6g\n", xx,yy);*/	
		if (showon && drawon) ps_continue_line(PLOT_FD, xx, yy);
	    } else {
		if (showon && drawon) ps_start_line(PLOT_FD, xx, yy);
	    }
	}
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

int pickcheck(x1,y1,x2,y2,x3,y3,eps)
double x1,y1,x2,y2,x3,y3;
double eps;
{
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

void jump(bb, mode) 
BOUNDS *bb;
int mode;
{
    nseg=0;  
    filled_object = 0;		/* automatically close polygon */
    if (!X) {
	/* if (drawon) fprintf(PLOT_FD, "jump\n");*/   /* autoplot */
    }
}

void set_layer(lnum, comp)
int lnum;	/* layer number */
int comp;	/* component type */
{
    extern int showon;

    if (comp) {
	showon = show_check_visible(currep, comp, lnum);
    } else {
        showon=1;
    }

    layer_fill = equate_get_fill(lnum);	

    if (X) {
	xwin_set_pen(equate_get_color(lnum));	
	xwin_set_line(equate_get_linetype(lnum));
	/* xwin_set_pen((lnum%8));xwin_set_line((((int)(lnum/8))%5)); */
    } else {
        ps_set_pen(equate_get_color(lnum));	
	ps_set_line(equate_get_linetype(lnum));
        /* ps_set_line((lnum%5)+1); */
	/* if(drawon)fprintf(PLOT_FD,"pen %d\n",(lnum%8));*//* autoplot */
	/* if(drawon)fprintf(PLOT_FD,"line %d\n",(lnum%5)+1);*//* autoplot */
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

