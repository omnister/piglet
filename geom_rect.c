#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"

#define NORM   0	/* draw() modes */
#define RUBBER 1

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

int add_rect(int *layer)
{
    enum {START,NUM1,COM1,NUM2,NUM3,COM2,NUM4,END} state = START;

    int done=0;
    int error=0;
    TOKEN token;
    char word[BUFSIZE];
    double x2,y2;

    printf("layer %d\n",*layer);
    rl_setprompt("ADD_RECT> ");

    while (!done) {
	token = token_look(word);
	/* printf("got %s: %s\n", tok2str(token), word); */
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	    case START:		/* get option or first xy pair */
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
		if (token == NUMBER) {
		    token_get(word);
		    sscanf(word, "%lf", &y1);	/* scan it in */
		    rubber_set_callback(draw_box);
		    state = NUM3;
		} else if (token == EOL) {
		    token_get(word); 	/* just ignore it */
		} else if (token == EOC) {
		    state = END; 
		}
		break;
	    case NUM3:		/* get pair of xy coordinates */
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
		if (token == NUMBER) {
		    token_get(word);
		    sscanf(word, "%lf", &y2);	/* scan it in */
		    state = START;
		    modified = 1;
		    db_add_rect(currep, *layer, (OPTS *) NULL, x1, y1, x2, y2);
		    rubber_clear_callback();
		    need_redraw++;
		} else if (token == EOL) {
		    token_get(word); /* just ignore it */
		} else if (token == EOC) {
		    state = END; 
		}
		break;
	    case END:
	    default:
		if (token == EOC || token == CMD) {
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

void draw_box(x2, y2, count) 
double x2, y2;
int count; /* number of times called */
{
	static double x1old, x2old, y1old, y2old;

	if (count == 0) {		/* first call */
	    jump(); /* draw new shape */
	    draw(x1,y1, RUBBER);
	    draw(x1,y2, RUBBER);
	    draw(x2,y2, RUBBER);
	    draw(x2,y1, RUBBER);
	    draw(x1,y1, RUBBER);

	} else if (count > 0) {		/* intermediate calls */

	    jump(); /* erase old shape */
	    draw(x1old,y1old, RUBBER);
	    draw(x1old,y2old, RUBBER);
	    draw(x2old,y2old, RUBBER);
	    draw(x2old,y1old, RUBBER);
	    draw(x1old,y1old, RUBBER);

	    jump(); /* draw new shape */
	    draw(x1,y1, RUBBER);
	    draw(x1,y2, RUBBER);
	    draw(x2,y2, RUBBER);
	    draw(x2,y1, RUBBER);
	    draw(x1,y1, RUBBER);

	} else {			/* last call, cleanup */
	    jump(); /* erase old shape */
	    draw(x1old,y1old, RUBBER);
	    draw(x1old,y2old, RUBBER);
	    draw(x2old,y2old, RUBBER);
	    draw(x2old,y1old, RUBBER);
	    draw(x1old,y1old, RUBBER);
	}

	/* save old values */
	x1old=x1;
	y1old=y1;
	x2old=x2;
	y2old=y2;
	jump();

}

