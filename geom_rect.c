#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"
#include "lex.h"

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

OPTS opts;

int add_rect(lp, layer)
LEXER *lp;
int *layer;
{
    enum {START,NUM1,COM1,NUM2,NUM3,COM2,NUM4,END} state = START;

    int done=0;
    int error=0;
    TOKEN token;
    char word[BUFSIZE];
    double x2,y2;
    int debug=0;

    if (debug) printf("layer %d\n",*layer);
    rl_setprompt("ADD_RECT> ");

    opt_set_defaults(&opts);

    while (!done) {
	token = token_look(lp,word);
	if (debug) printf("got %s: %s\n", tok2str(token), word); 
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	case START:		/* get option or first xy pair */
	    if (token == OPT ) {
		token_get(lp,word); /* ignore for now */
		state = START;
	    } else if (token == NUMBER) {
		state = NUM1;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just eat it up */
		state = START;
	    } else if (token == EOC || token == CMD) {
		 state = END;
	    } else {
		token_err("RECT", lp, "expected OPT or NUMBER", token);
		state = END;	/* error */
	    }
	    break;
	case NUM1:		/* get pair of xy coordinates */
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &x1);	/* scan it in */
		state = COM1;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("   cancelling ADD RECT\n");
		state = END;	
	    } else {
		token_err("RECT", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case COM1:		
	    if (token == EOL) {
		token_get(lp,word); /* just ignore it */
	    } else if (token == COMMA) {
		token_get(lp,word);
		state = NUM2;
	    } else {
		token_err("RECT", lp, "expected COMMA", token);
	        state = END;
	    }
	    break;
	case NUM2:
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &y1);	/* scan it in */
		rubber_set_callback(draw_box);
		state = NUM3;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("ADD RECT: cancelling ADD RECT\n");
	        state = END;
	    } else {
		token_err("RECT", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case NUM3:		/* get pair of xy coordinates */
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &x2);	/* scan it in */
		state = COM2;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("ADD RECT: cancelling ADD RECT\n");
	        state = END;
	    } else {
		token_err("RECT", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case COM2:		
	    if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == COMMA) {
		token_get(lp,word);
		state = NUM4;
	    } else if (token == EOL) {
		token_get(lp,word); /* just ignore it */
	    } else {
		token_err("RECT", lp, "expected COMMA", token);
		state = END;	
	    }
	    break;
	case NUM4:
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &y2);	/* scan it in */
		state = START;
		db_add_rect(currep, *layer, opt_copy(&opts), x1, y1, x2, y2);
		rubber_clear_callback();
		need_redraw++;
	    } else if (token == EOL) {
		token_get(lp,word); /* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("ADD RECT: cancelling ADD RECT\n");
		state = END; 
	    } else {
		token_err("RECT", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case END:
	default:
	    if (token == EOC || token == CMD) {
		;
	    } else {
		token_flush_EOL(lp);
	    }
	    done++;
	    break;
	}
    }
    rubber_clear_callback();
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

