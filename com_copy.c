#include <stdio.h>
#include <string.h>		/* for strchr() */
#include <ctype.h>		/* for toupper() */

#include "rlgetc.h"
#include "db.h"
#include "token.h"
#include "xwin.h" 	
#include "lex.h"
#include "rubber.h"

static double x1, y1, x2, y2, x3, y3;
static double xmin, ymin, xmax, ymax;
void draw_cbox();

/* 
    copy a component in the current device.
    COP <restrictor> { xysel xyref xynewref1 [xynewref2...] } <EOC>
*/

com_copy(LEXER *lp, char *arg)		
{

    enum {START,NUM1,COM1,NUM2,NUM3,COM2,NUM4,NUM5,COM3,NUM6,END} state = START;

    char *line;
    TOKEN token;
    char word[BUFSIZE];
    char buf[BUFSIZE];
    int debug=0;
    int done=0;
    int nnum=0;
    int retval;
    int valid_comp=0;
    int i;
    int flag;
    DB_DEFLIST *p_best;
    DB_DEFLIST *p_new = NULL;

    int my_layer=0; 	/* internal working layer */

    int comp=ALL;

    /* check that we are editing a rep */
    if (currep == NULL ) {
	printf("must do \"EDIT <name>\" before COPY\n");
	token_flush_EOL(lp);
	return(1);
    }

/* 
    To create a show set for restricting the pick, we look 
    here for and optional primitive indicator
    concatenated with an optional layer number:

	A[layer_num] ;(arc)
	C[layer_num] ;(circle)
	L[layer_num] ;(line)
	N[layer_num] ;(note)
	O[layer_num] ;(oval)
	P[layer_num] ;(poly)
	R[layer_num] ;(rectangle)
	T[layer_num] ;(text)

    or a instance name.  Instance names can be quoted, which
    allows the user to have instances which overlap the primitive
    namespace.  For example:  N7 is a note on layer seven, but
    "N7" is an instance call.

*/

    rl_saveprompt();
    rl_setprompt("COPY> ");
    while(!done) {
	token = token_look(lp,word);
	if (debug) printf("got %s: %s\n", tok2str(token), word);
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	case START:		/* get option or first xy pair */
	    if (debug) printf("in START\n");
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
	    } else if (token == IDENT) {
		token_get(lp,word);
	    	state = NUM1;
		/* check to see if is a valid comp descriptor */
		valid_comp=0;
		if (comp = is_comp(toupper(word[0]))) {
		    if (strlen(word) == 1) {
			my_layer = default_layer();
			printf("using default layer=%d\n",my_layer);
			valid_comp++;	/* no layer given */
		    } else {
			valid_comp++;
			/* check for any non-digit characters */
			/* to allow instance names like "L1234b" */
			for (i=0; i<strlen(&word[1]); i++) {
			    if (!isdigit(word[1+i])) {
				valid_comp=0;
			    }
			}
			if (valid_comp) {
			    if(sscanf(&word[1], "%d", &my_layer) == 1) {
				if (debug) printf("given layer=%d\n",my_layer);
			    } else {
				valid_comp=0;
			    }
			} 
			if (valid_comp) {
			    if (my_layer > MAX_LAYER) {
				printf("layer must be less than %d\n",
				    MAX_LAYER);
				valid_comp=0;
				done++;
			    }
			    if (!show_check_modifiable(comp, my_layer)) {
				printf("layer %d is not modifiable!\n",
				    my_layer);
				token_flush_EOL(lp);
				valid_comp=0;
				done++;
			    }
			}
		    }
		} else { 
		    /* here need to handle a valid cell name */
		    printf("looks like a descriptor to me: %s\n", word);
		}
	    } else {
		token_err("COPY", lp, "expected DESC or NUMBER", token);
		state = END;	/* error */
	    }
	    break;
	case NUM1:		/* get pair of xy coordinates */
	    if (debug) printf("in NUM1\n");
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &x1);	/* scan it in */
		state = COM1;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		state = END;	
	    } else {
		token_err("COPY", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case COM1:		
	    if (debug) printf("in COM1\n");
	    if (token == EOL) {
		token_get(lp,word); /* just ignore it */
	    } else if (token == COMMA) {
		token_get(lp,word);
		state = NUM2;
	    } else {
		token_err("COPY", lp, "expected COMMA", token);
	        state = END;
	    }
	    break;
	case NUM2:
	    if (debug) printf("in NUM2\n");
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &y1);	/* scan it in */

		if (debug) printf("got comp %d, layer %d\n", comp, my_layer);
		if ((p_best=db_ident(currep, x1,y1,1, my_layer, comp, 0)) != NULL) {
		    db_notate(p_best);	    /* print out id information */
		    db_highlight(p_best);
		    xmin=p_best->xmin;
		    xmax=p_best->xmax;
		    ymin=p_best->ymin;
		    ymax=p_best->ymax;
		    state = NUM3;
		} else {
		    printf("nothing here that is copyable... try SHO command?\n");
		    state = START;
		}
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("COPY: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("COPY", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case NUM3:
	    if (debug) printf("in NUM3\n");
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &x2);	/* scan it in */
		state = COM2;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		state = END;	
	    } else {
		token_err("COPY", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case COM2:		
	    if (debug) printf("in COM2\n");
	    if (token == EOL) {
		token_get(lp,word); /* just ignore it */
	    } else if (token == COMMA) {
		token_get(lp,word);
		state = NUM4;
	    } else {
		token_err("COPY", lp, "expected COMMA", token);
	        state = END;
	    }
	    break;
	case NUM4:
	    if (debug) printf("in NUM4\n");
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &y2);	/* scan it in */
		db_highlight(p_best);		/* unhighlight */
		rubber_set_callback(draw_cbox);
		state = NUM5;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("COPY: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("COPY", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case NUM5:
	    if (debug) printf("in NUM5\n");
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &x3);	/* scan it in */
		state = COM3;
	    } else if (token == BACK) {
		token_get(lp,word); 
		if (p_new == NULL) {
			;   /* just ignore it */
		} else {
		    db_unlink_component(currep, p_new);
		    p_new=NULL;
		    need_redraw++;
		}
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		state = END;	
	    } else {
		token_err("COPY", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case COM3:		
	    if (debug) printf("in COM3\n");
	    if (token == EOL) {
		token_get(lp,word); /* just ignore it */
	    } else if (token == COMMA) {
		token_get(lp,word);
		state = NUM6;
	    } else {
		token_err("COPY", lp, "expected COMMA", token);
	        state = END;
	    }
	    break;
	case NUM6:
	    if (debug) printf("in NUM6\n");
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &y3);	/* scan it in */
		printf("got %g %g\n", x3, y3);
		/* rubber_clear_callback(); */
		p_new=db_copy_component(p_best);
		db_move_component(p_new, x3-x2, y3-y2);
		db_insert_component(currep, p_new);
		need_redraw++;
		state = NUM5;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("COPY: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("COPY", lp, "expected NUMBER", token);
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
	    rubber_clear_callback();
	    break;
	}
    }
    rl_restoreprompt();
    return(1);
}


/*

    if (!done) {
	if (valid_comp) {
	} else { 
	    if ((strlen(word) == 1) && toupper(word[0]) == 'I') {
		if((token=token_get(lp,word)) != IDENT) {
		    printf("DEL INST: bad inst name: %s\n", word);
		    done++;
		}
	    }
	    if (debug) printf("calling del_inst with %s\n", word);
	    if (!show_check_modifiable(INST, my_layer)) {
		    printf("INST component is not modifiable!\n");
	    } else {
		;
	    }
	}
    }
} else if (token == QUOTE) {
    if (!show_check_modifiable(INST, my_layer)) {
	    printf("INST component is not modifiable!\n");
    } else {
	;
*/



void draw_cbox(x, y, count)
double x, y;  /* offset */
int count; /* number of times called */
{
	static double x1old, x2old, y1old, y2old;
	static double xx1, yy1, xx2, yy2;
	BOUNDS bb;
	bb.init=0;

	xx1=xmin+x-x2;
	yy1=ymin+y-y2;
	xx2=xmax+x-x2;
	yy2=ymax+y-y2;

	if (count == 0) {		/* first call */
	    jump(); /* draw new shape */
	    draw(xx1,yy1, &bb, D_RUBBER);
	    draw(xx1,yy2, &bb, D_RUBBER);
	    draw(xx2,yy2, &bb, D_RUBBER);
	    draw(xx2,yy1, &bb, D_RUBBER);
	    draw(xx1,yy1, &bb, D_RUBBER);
	    jump();
	    draw(xx1,yy1, &bb, D_RUBBER);
	    draw(xx2,yy2, &bb, D_RUBBER);
	    jump();
	    draw(xx1,yy2, &bb, D_RUBBER);
	    draw(xx2,yy1, &bb, D_RUBBER);

	} else if (count > 0) {		/* intermediate calls */

	    jump(); /* erase old shape */
	    draw(x1old,y1old, &bb, D_RUBBER);
	    draw(x1old,y2old, &bb, D_RUBBER);
	    draw(x2old,y2old, &bb, D_RUBBER);
	    draw(x2old,y1old, &bb, D_RUBBER);
	    draw(x1old,y1old, &bb, D_RUBBER);
	    jump();
	    draw(x1old,y1old, &bb, D_RUBBER);
	    draw(x2old,y2old, &bb, D_RUBBER);
	    jump();
	    draw(x1old,y2old, &bb, D_RUBBER);
	    draw(x2old,y1old, &bb, D_RUBBER);

	    jump(); /* draw new shape */
	    draw(xx1,yy1, &bb, D_RUBBER);
	    draw(xx1,yy2, &bb, D_RUBBER);
	    draw(xx2,yy2, &bb, D_RUBBER);
	    draw(xx2,yy1, &bb, D_RUBBER);
	    draw(xx1,yy1, &bb, D_RUBBER);
	    jump();
	    draw(xx1,yy1, &bb, D_RUBBER);
	    draw(xx2,yy2, &bb, D_RUBBER);
	    jump();
	    draw(xx1,yy2, &bb, D_RUBBER);
	    draw(xx2,yy1, &bb, D_RUBBER);

	} else {			/* last call, cleanup */
	    jump(); /* erase old shape */
	    draw(x1old,y1old, &bb, D_RUBBER);
	    draw(x1old,y2old, &bb, D_RUBBER);
	    draw(x2old,y2old, &bb, D_RUBBER);
	    draw(x2old,y1old, &bb, D_RUBBER);
	    draw(x1old,y1old, &bb, D_RUBBER);
	    jump();
	    draw(xx1,yy1, &bb, D_RUBBER);
	    draw(xx2,yy2, &bb, D_RUBBER);
	    jump();
	    draw(xx1,yy2, &bb, D_RUBBER);
	    draw(xx2,yy1, &bb, D_RUBBER);
	}

	/* save old values */
	x1old=xx1;
	y1old=yy1;
	x2old=xx2;
	y2old=yy2;
	jump();
}

