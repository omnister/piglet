#include "db.h"
#include "lex.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"

#define NORM 0		/* drawing modes */
#define RUBBER 1	 

static COORDS *CP;

DB_TAB dbtab; 
DB_DEFLIST dbdeflist;
DB_POLY dbpoly;

/* FIXME: successive segments currently reuse the same pointer to
option block.  Need to make new one for each seg, and copy all values
Also need to make sure and release any unused opts to avoid mem leaks.*/


/*
 *
 *  "ADD"--+---"P"------+-+-------------+--|xy|-|xy|-+-|xy|-+--<CMD|EOC>-->
 *         |            v |             v            |      v 
 *         +-"P"<layer>-+ +-":W"<width>-+            +------+
 *                        |             |
 *                        +-":F"--------+
 */

OPTS opts;

static double x1, y1;

int add_poly(int *layer)
{
    enum {START,NUM1,COM1,NUM2,NUM3,COM2,NUM4,END,ERR} state = START;

    int debug=0;
    int done=0;
    int error=0;
    int nsegs;
    TOKEN token;
    char word[BUFSIZE];
    double x2,y2;
    static double xold, yold;

    opt_set_defaults(&opts);

    if (debug) {printf("in geom_poly: layer %d\n",*layer);}
    rl_setprompt("ADD_POLY> ");

    while (!done) {
	token = token_look(word);
	if (debug) printf("got %s: %s\n", tok2str(token), word);
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	    case START:		/* get option or first xy pair */
		nsegs=0;
		if (debug) printf("in start\n");
		if (token == OPT ) {
		    token_get(word); 
		    if (opt_parse(word, "W", &opts) == -1) {
		    	state = END;
		    } else {
			state = START;
		    }
		} else if (token == NUMBER) {
		    state = NUM1;
		} else if (token == EOL) {
		    token_get(word); 	/* just eat it up */
		    state = START;
		} else if (token == EOC || token == CMD) {
		    done++;
		} else {
	            printf("   expected OPTION or XYPAIR, got: %s\n", 
		        tok2str(token));
		    state = END; 
		} 
		xold=yold=0.0;
		break;
	    case NUM1:		/* get pair of xy coordinates */
		if (debug) printf("in num1\n");
		if (token == NUMBER) {
		    token_get(word);
		    xold=x1;
		    sscanf(word, "%lf", &x1);	/* scan it in */
		    state = COM1;
		} else if (token == EOL) {
		    token_get(word); 	/* just ignore it */
		} else if (token == EOC || token == CMD) {
		    printf("   cancelling ADD POLY\n");
		    done++;
		} else {
	            printf("   expected NUMBER, EOC or CMD, got: %s\n", 
		        tok2str(token));
		    state = END; 
		}
		break;
	    case COM1:		
		if (debug) printf("in com1\n");
		if (token == EOL) {
		    token_get(word); /* just ignore it */
		} else if (token == COMMA) {
		    token_get(word);
		    state = NUM2;
		} else {
		    printf("  expected COMMA, got %s\n",
		    	tok2str(token));
		    state = END;	
		}
		break;
	    case NUM2:
		if (debug) printf("in num2\n");
		if (token == NUMBER) {
		    token_get(word);

		    yold=y2;
		    sscanf(word, "%lf", &y1);	/* scan it in */
		    
		    CP = coord_new(x1,y1);
		    coord_append(CP, x1,y1);
		    nsegs++;

		    if (debug) coord_print(CP);

		    rubber_set_callback(draw_poly);
		    state = NUM3;
		} else if (token == EOL) {
		    token_get(word); 	/* just ignore it */
		} else if (token == EOC || token == CMD) {
		    printf("   cancelling ADD POLY\n");
		    done++;
		} else {
	            printf("   expected NUMBER, EOC or CMD, got: %s\n", 
		        tok2str(token));
		    state = END; 
		}
		break;
	    case NUM3:		/* get pair of xy coordinates */
		if (debug) printf("in num3\n");
		if (token == NUMBER) {
		    token_get(word);
		    xold=x2;
		    sscanf(word, "%lf", &x2);	/* scan it in */
		    state = COM2;
		} else if (token == EOL) {
		    token_get(word); 	/* just ignore it */
		} else if (token == EOC || token == CMD) {
		    state = END; 
		} else {
	            printf("  expected NUMBER, EOC or CMD, got: %s\n", 
		        tok2str(token));
		    state = END; 
		}
		break;
	    case COM2:		
		if (debug) printf("in com2\n");
		if (token == EOL) {
		    token_get(word); 	/* just ignore it */
		} else if (token == COMMA) {
		    token_get(word);
		    state = NUM4;
		} else if (token == EOL) {
		    token_get(word); /* just ignore it */
		} else if (token == EOC || token == CMD) {
		    printf("cancelling ADD POLY\n");
		    done++;
		} else {
		    printf("  expected COMMA, EOC or CMD, got:%s\n",
		    	tok2str(token));
		    state = END;	
		}
		break;
	    case NUM4:
		if (debug) printf("in num4\n");
		if (token == NUMBER) {
		    token_get(word);
		    yold=y2;
		    sscanf(word, "%lf", &y2);	/* scan it in */

		    nsegs++;

		    /* two identical clicks terminates this poly */
		    /* but keeps the ADD L command in effect */

		    if (nsegs==1 && x1==x2 && y1==y2) {
	    		rubber_clear_callback();
			printf("POLY: seg must have finite length\n");
			state = START;
		    } else if (x2==xold && y2==yold) {

		    	if (debug) coord_print(CP);
			if (debug) printf("dropping coord\n");
			coord_drop(CP);  /* drop last coord */
		    	if (debug) coord_print(CP);
			if (debug) printf("adding poly1 layer %d\n", *layer);

			db_add_poly(currep, *layer, opt_copy(&opts), CP);
			modified = 1;
			need_redraw++;
			rubber_clear_callback();
			state = START;
		    } else {
			rubber_clear_callback();
			if (debug) printf("doing append\n");
			coord_swap_last(CP, x2, y2);
			coord_append(CP, x2,y2);
			nsegs++;
			rubber_set_callback(draw_poly);
			rubber_draw(x2,y2);
			state = NUM3;	/* loop till EOC */
		    }

		} else if (token == EOL) {
		    token_get(word); /* just ignore it */
		} else if (token == EOC || token == CMD) {
		    printf("    cancelling ADD POLY\n");
		    done++;
		} else {
	            printf("  expected NUMBER, EOC or CMD, got: %s\n", 
		        tok2str(token));
		    state = END; 
		}
		break;
	    case END:
		if (debug) printf("in end\n");
		if (token == EOC) {
		    if (debug) printf("adding poly2 layer %d\n", *layer);
		    if (nsegs >= 3) {
			db_add_poly(currep, *layer, opt_copy( & opts), CP);
			modified = 1;
			need_redraw++;
		    } else {
			printf("   POLY requires at least three points\n");
		    }
		} else if (token == CMD) {
		    printf("   cancelling ADD POLY\n");
		    done++;
		} else {
		    token_flush();
		}
		done++;
		break;
	    default:
	    	break;
	}
    }
    rubber_clear_callback();
    return(1);
}

void draw_poly(x2, y2, count) 
double x2, y2;
int count; /* number of times called */
{
	static double x1old, x2old, y1old, y2old;
	int i;

	int debug=0;

	/* DB_TAB dbtab;  */
	/* DB_DEFLIST dbdeflist; */
	/* DB_POLY dbpoly; */


        dbtab.dbhead = &dbdeflist;
        dbtab.dbtail = &dbdeflist;
        dbtab.next = NULL;
	dbtab.name = "callback";

        dbdeflist.u.p = &dbpoly;
        dbdeflist.type = POLY;

        dbpoly.layer=1;
        dbpoly.opts=&opts;
        dbpoly.coords=CP;
	
	if (debug) {printf("in draw_poly\n");}

	if (count == 0) {		/* first call */
	    jump(); /* draw new shape */
	    do_poly(&dbdeflist, 1);

	} else if (count > 0) {		/* intermediate calls */
	    jump(); /* erase old shape */
	    do_poly(&dbdeflist, 1);
	    jump(); /* draw new shape */
	    coord_swap_last(CP, x2, y2);
	    do_poly(&dbdeflist, 1);
	} else {			/* last call, cleanup */
	    jump(); /* erase old shape */
	    do_poly(&dbdeflist, 1);
	}

	/* save old values */
	x1old=x1;
	y1old=y1;
	x2old=x2;
	y2old=y2;
	jump();
}

