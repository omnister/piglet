#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"

#define NORM 0		/* drawing modes */
#define RUBBER 1	 

PAIR tmp_pair;
static COORDS *first_pair, *last_pair; 

DB_TAB dbtab; 
DB_DEFLIST dbdeflist;
DB_LINE dbline;


COORDS *coord_create(x,y)
double x,y;
{
    COORDS *tmp;
    tmp = (COORDS *) emalloc(sizeof(COORDS));
    tmp->coord.x = x;
    tmp->coord.y = y;
    tmp->next = NULL;
    tmp->prev = NULL;
    return(tmp);
}

void coord_new(x,y)
double x,y;
{
    COORDS *temp;
    first_pair = last_pair = coord_create(x,y);
}


void coord_append(x,y)
double x,y;
{
    COORDS *tmp;
    tmp = last_pair;	/* save pointer to last coord */
    last_pair->next = coord_create(x,y);
    last_pair = last_pair->next;
    last_pair->prev = tmp;
}

void coord_swap_last(x,y)
double x,y;
{
    last_pair->coord.x = x;
    last_pair->coord.y = y;
}

void coord_drop()
{
    COORDS *tmp;
    tmp = last_pair;
    last_pair = last_pair->prev;
    last_pair->next = NULL;
    free((COORDS *) tmp);
}


void print_pairs()
{	
    COORDS *tmp;
    int i;

    i=1;
    tmp = first_pair;
    while(tmp != NULL) {
	printf(" %g,%g", tmp->coord.x, tmp->coord.y);
	if ((tmp = tmp->next) != NULL && (++i)%4 == 0) {
	    printf("\n");
	}
    }
    printf(";\n");
}		

/* test harness */
/*
main() 
{
	coord_new(1.0,2.0);
	coord_append(3.0, 4.0);
	coord_append(5.0, 6.0);
	coord_append(7.0, 8.0);
	print_pairs();
	coord_swap_last(9.0, 10.0);
	print_pairs();
}
*/
/*
 *
 *        +--------------+---------<--+---------------+
 *        v              v            ^               |
 * (ADD)--0---(R)------+-1------------+--|xy|---|xy|----|prim|->
 *        |            | |            |                  |cmd |
 *        +-(R)<layer>-+ +-(:W<width>-+
 *                       |            |
 *                       +-(:FILL)----+
 */

static double x1, y1;

int add_line(int *layer)
{
    enum {START,NUM1,COM1,NUM2,NUM3,COM2,NUM4,END} state = START;

    int debug=0;
    int done=0;
    int error=0;
    int nsegs;
    TOKEN token;
    char word[BUFSIZE];
    double x2,y2;
    static double xold, yold;
    OPTS *opts;

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

    opts = opt_create();
    opts->width = 10.0;

    if (debug) {printf("layer %d\n",*layer);}
    rl_setprompt("ADD_LINE> ");

    while (!done) {
	token = token_look(word);
	if (debug) {printf("got %s: %s\n", tok2str(token), word);}
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	    case START:		/* get option or first xy pair */
		nsegs=0;
		if (debug) printf("in start\n");
		if (token == OPT ) {
		    token_get(word); /* ignore for now */
		    state = START;
		} else if (token == NUMBER) {
		    state = NUM1;
		} else if (token == EOL) {
		    token_get(word); 	/* just eat it up */
		    state = START;
		} else {
		    state = END;	/* error */
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
		} else if (token == EOC) {
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
		    printf("    1: expected a comma!\n");
		    state = END;	
		}
		break;
	    case NUM2:
		if (debug) printf("in num2\n");
		if (token == NUMBER) {
		    token_get(word);

		    yold=y2;
		    sscanf(word, "%lf", &y1);	/* scan it in */
		    
		    coord_new(x1,y1);
		    coord_append(x1,y1);

		    print_pairs();

		    rubber_set_callback(draw_line);
		    state = NUM3;
		} else if (token == EOL) {
		    token_get(word); 	/* just ignore it */
		} else if (token == EOC) {
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
		} else if (token == EOC) {
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
		} else {
		    printf("  2: expected a comma!, got:%s\n", tok2str(token));
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

		    /* two identical clicks terminates this line */
		    /* but keeps the ADD L command in effect */

		    if (nsegs==1 && x1==x2 && y1==y2) {
	    		rubber_clear_callback();
			printf("error: a line must have finite length\n");
			state = START;
		    } else if (x2==xold && y2==yold) {
		    	print_pairs();
			printf("dropping coord\n");
			coord_drop();  /* drop last coord */
		    	print_pairs();
			db_add_line(currep, *layer, opts, first_pair);
			modified = 1;
			need_redraw++;
			rubber_clear_callback();
			state = START;
		    } else {
			rubber_clear_callback();
			if (debug) printf("doing append\n");
			coord_append(x2,y2);
			rubber_set_callback(draw_line);
			rubber_draw(x2,y2);
			state = NUM3;	/* loop till EOC */
		    }

		} else if (token == EOL) {
		    token_get(word); /* just ignore it */
		} else if (token == EOC) {
		    state = END; 
		}
		break;
	    case END:
	    default:
		if (debug) printf("in end\n");
		if (token == EOC) {
			db_add_line(currep, *layer, opts, first_pair);
			modified = 1;
			need_redraw++;
		    	; /* add geom */
		} else if (token == CMD) {
			;
		} else {
		    token_flush();
		}
	    	rubber_clear_callback();
		done++;
		break;
	}
    }
    return(1);
}

void draw_line(x2, y2, count) 
double x2, y2;
int count; /* number of times called */
{
	static double x1old, x2old, y1old, y2old;
	int i;

	int debug=0;

	/* DB_TAB dbtab;  */
	/* DB_DEFLIST dbdeflist; */
	/* DB_LINE dbline; */


        dbtab.dbhead = &dbdeflist;
        dbtab.dbtail = &dbdeflist;
        dbtab.next = NULL;
	dbtab.name = "callback";

        dbdeflist.u.l = &dbline;
        dbdeflist.type = LINE;

        dbline.layer=1;
        dbline.opts=opt_create();
	dbline.opts->width = 10.0;
        dbline.coords=first_pair;
	
	if (debug) {printf("in draw_line\n");}

	if (count == 0) {		/* first call */
	    jump(); /* draw new shape */
	    do_line(&dbdeflist, 1);

	} else if (count > 0) {		/* intermediate calls */
	    jump(); /* erase old shape */
	    do_line(&dbdeflist, 1);
	    jump(); /* draw new shape */
	    coord_swap_last(x2, y2);
	    do_line(&dbdeflist, 1);
	} else {			/* last call, cleanup */
	    jump(); /* erase old shape */
	    do_line(&dbdeflist, 1);
	}

	/* save old values */
	x1old=x1;
	y1old=y1;
	x2old=x2;
	y2old=y2;
	jump();
}

