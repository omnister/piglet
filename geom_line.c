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
    last_pair = last_pair->next = coord_create(x,y);
}

void coord_swap_last(x,y)
double x,y;
{
    last_pair->coord.x = x;
    last_pair->coord.y = y;
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

    int done=0;
    int error=0;
    TOKEN token;
    char word[BUFSIZE];
    double x2,y2;

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


    printf("layer %d\n",*layer);
    rl_setprompt("ADD_LINE> ");

    while (!done) {
	token = token_look(word);
	printf("got %s: %s\n", tok2str(token), word);
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	    case START:		/* get option or first xy pair */
		printf("in start\n");
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
		break;
	    case NUM1:		/* get pair of xy coordinates */
		printf("in num1\n");
		if (token == NUMBER) {
		    token_get(word);
		    sscanf(word, "%lf", &x1);	/* scan it in */
		    state = COM1;
		} else if (token == EOL) {
		    token_get(word); 	/* just ignore it */
		} else if (token == EOC) {
		    state = END; 
		}
		break;
	    case COM1:		
		printf("in com1\n");
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
		printf("in num2\n");
		if (token == NUMBER) {
		    token_get(word);
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
		printf("in num3\n");
		if (token == NUMBER) {
		    token_get(word);
		    sscanf(word, "%lf", &x2);	/* scan it in */
		    state = COM2;
		} else if (token == EOL) {
		    token_get(word); 	/* just ignore it */
		} else if (token == EOC) {
		    state = END; 
		}
		break;
	    case COM2:		
		printf("in com2\n");
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
		printf("in num4\n");
		if (token == NUMBER) {
		    token_get(word);
		    sscanf(word, "%lf", &y2);	/* scan it in */

		    rubber_clear_callback();
		    printf("doing append\n");
		    coord_append(x2,y2);
		    rubber_set_callback(draw_line);
		    rubber_draw(x2,y2);

		    print_pairs(first_pair);

		    state = NUM3;	/* loop till EOC */
		} else if (token == EOL) {
		    token_get(word); /* just ignore it */
		} else if (token == EOC) {
		    state = END; 
		}
		break;
	    case END:
	    default:
		printf("in end\n");
		if (token == EOC) {
			db_add_line(currep, *layer, (OPTS *) NULL, first_pair);
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

	/* DB_TAB dbtab;  */
	/* DB_DEFLIST dbdeflist; */
	/* DB_LINE dbline; */

	/*
	typedef struct opt_list {
	    char *optstring;
	    struct opt_list *next;
	} OPTS;
	*/


        dbtab.dbhead = &dbdeflist;
        dbtab.dbtail = &dbdeflist;
        dbtab.next = NULL;
	dbtab.name = "callback";

        dbdeflist.u.l = &dbline;
        dbdeflist.type = LINE;

        dbline.layer=1;
        dbline.opts=(OPTS *)NULL;
        dbline.coords=first_pair;
	
	printf("in draw_line\n");

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

void old_draw_line(x2, y2, count) 
double x2, y2;
int count; /* number of times called */
{
	static double x1old, x2old, y1old, y2old;
	COORDS *tmp;
	int i;
	
	printf("in draw_line\n");

	if (count == 0) {		/* first call */
	    jump(); /* draw new shape */
	    tmp = first_pair;
	    while(tmp != NULL) {
		draw(tmp->coord.x, tmp->coord.y, RUBBER);
		tmp = tmp->next;
	    }
	    draw(x2, y2, RUBBER);
	} else if (count > 0) {		/* intermediate calls */

	    jump(); /* erase old shape */
	    tmp = first_pair;
	    while(tmp != NULL) {
		draw(tmp->coord.x, tmp->coord.y, RUBBER);
		tmp = tmp->next;
	    }
	    draw(x2old, y2old, RUBBER);

	    jump(); /* draw new shape */
	    tmp = first_pair;
	    while(tmp != NULL) {
		draw(tmp->coord.x, tmp->coord.y, RUBBER);
		tmp = tmp->next;
	    }
	    draw(x2, y2, RUBBER);
	
	} else {			/* last call, cleanup */
	    jump(); /* erase old shape */
	    tmp = first_pair;
	    while(tmp != NULL) {
		draw(tmp->coord.x, tmp->coord.y, RUBBER);
		tmp = tmp->next;
	    }
	    draw(x2old, y2old, RUBBER);
	}

	/* save old values */
	x1old=x1;
	y1old=y1;
	x2old=x2;
	y2old=y2;
	jump();
}

