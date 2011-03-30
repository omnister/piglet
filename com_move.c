#include <stdio.h>
#include <string.h>		/* for strchr() */
#include <ctype.h>		/* for toupper() */

#include "rlgetc.h"
#include "db.h"
#include "token.h"
#include "xwin.h" 	
#include "rubber.h"

#define POINT 0
#define REGION 1

static double x1, y1, x2, y2, x3, y3, x4, y4;
static double lastx, lasty;
static double xmin, ymin, xmax, ymax;
void draw_mbox();
void move_draw_box();
STACK *stack;
STACK *tmp;

/* 
    move a component in the current device.
    MOV <restrictor> { xysel xyref xynewref1 [xynewref2...] } <EOC>
*/

int com_move(LEXER *lp, char *arg)		
{

    enum {START,NUM1,NUM2,NUM3,NUM4,END} state = START;

    TOKEN token;
    char *word;
    int debug=0;
    int done=0;
    int valid_comp=0;
    int i;
    DB_DEFLIST *p_best;
    DB_DEFLIST *p_new = NULL;
    int mode=POINT;
    char instname[BUFSIZE];
    char *pinst = (char *) NULL;
    int num_moves=0;

    int my_layer=0; 	/* internal working layer */

    int comp=ALL;

    /* check that we are editing a rep */
    if (currep == NULL ) {
	printf("must do \"EDIT <name>\" before MOVE\n");
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

    while(!done) {
	token = token_look(lp,&word);
	if (debug) printf("got %s: %s\n", tok2str(token), word);
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	case START:		/* get option or first xy pair */
	    num_moves=0;
	    db_checkpoint(lp);
	    if (debug) printf("in START\n");
	    if (token == OPT ) {
		token_get(lp,&word); /* ignore for now */
		if (word[0]==':') {
                    switch (toupper(word[1])) {
                        case 'R':
                            mode = REGION;
                            break;
                        case 'P':
                            mode = POINT;
                            break;
                        default:
                            printf("MOVE: bad option: %s\n", word);
                            state=END;
                            break;
                    }
                } else {
                    printf("MOVE: bad option: %s\n", word);
                    state = END;
                }
                state = START;
	    } else if (token == NUMBER) {
		state = NUM1;
	    } else if (token == EOL || token == EOC) {
	    	rubber_clear_callback();
		token_get(lp,&word); 	/* just eat it up */
		state = START;
	    } else if (token == CMD) {
		state = END;
	    } else if (token == IDENT) {
		token_get(lp,&word);
		/* check to see if is a valid comp descriptor */
		valid_comp=0;
		if ((comp = is_comp(toupper(word[0])))) {
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
			    if (!show_check_modifiable(currep, comp, my_layer)) {
				printf("layer %d is not modifiable!\n",
				    my_layer);
				token_flush_EOL(lp);
				valid_comp=0;
				done++;
			    }
			}
		    }
		} else { 
		    if (db_lookup(word)) {
		        strncpy(instname, word, BUFSIZE);
			pinst = instname;
		    } else {
			printf("not a valid instance name: %s\n", word);
			state = START;
		    }
		}
	    } else {
		token_err("MOVE", lp, "expected DESC or NUMBER", token);
		state = END;	/* error */
	    }
	    break;
	case NUM1:
	    if (debug) printf("in NUM1\n");
	    if (token == NUMBER) {
		if (getnum(lp, "MOVE", &x1, &y1)) {
		    if (mode == POINT) {
			if ((p_best=db_ident(currep, x1,y1,1,my_layer, comp, pinst)) != NULL) {
			    db_notate(p_best);	    /* print out id information */
			    db_highlight(p_best);
			    xmin=p_best->xmin;
			    xmax=p_best->xmax;
			    ymin=p_best->ymin;
			    ymax=p_best->ymax;
			    state = NUM3;
			} else {
			    printf("nothing here that is moveable... try SHO command?\n");
			    state = START;
			}
		    } else {
			rubber_set_callback(move_draw_box);
			state = NUM2;
		    }
		} else {
		    state = END;
		} 
	    } else if (token == EOL) {
		token_get(lp,&word); 	/* just ignore it */
	    } else if (token == EOC) {
		printf("MOVE: cancelling POINT\n");
	    	rubber_clear_callback();
	        state = START;
	    } else if (token == CMD) {
		printf("MOVE: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("MOVE", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
        case NUM2:
            if (token == NUMBER) {
		if (getnum(lp, "MOVE", &x2, &y2)) {
		    rubber_clear_callback();
		    xmin=x1;
		    xmax=x2;
		    ymin=y1;
		    ymax=y2;
		    stack=db_ident_region(currep, x1,y1, x2, y2, 1, my_layer, comp, pinst);
		    state = NUM3;
		} else {
		    state = END;
		} 
	    } else if (token == EOL) {
		token_get(lp,&word);     /* just ignore it */
	    } else if (token == EOC) {
		rubber_clear_callback();
		printf("MOVE: cancelling POINT\n");
		state = START;
	    } else if (token == CMD) {
		printf("MOVE: cancelling POINT\n");
		state = END;
	    } else {
		rubber_clear_callback();
		token_err("IDENT", lp, "expected NUMBER", token);
		state = END;
	    }
            break;
	case NUM3:
	    if (debug) printf("in NUM3\n");
	    if (token == NUMBER) {
		if (getnum(lp, "MOVE", &x3, &y3)) {
		    if (mode == POINT) {
			db_highlight(p_best);
		    }
		    rubber_set_callback(draw_mbox);
		    state = NUM4;
		} else {
		    state = END;
		}
	    } else if (token == EOL) {
		token_get(lp,&word); 	/* just ignore it */
	    } else if (token == EOC) {
		if (mode==POINT) db_highlight(p_best);	/* unhighlight */
		printf("MOVE: cancelling POINT\n");
	        state = START;
	    } else if (token == CMD) {
		if (mode==POINT) db_highlight(p_best);	/* unhighlight */
		printf("MOVE: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("MOVE", lp, "expected NUMBER", token);
		if (mode==POINT) db_highlight(p_best);	/* unhighlight */
		state = END; 
	    }
	    break;
	case NUM4:
	    if (debug) printf("in NUM4\n");
	    if (token == NUMBER) {
		if (getnum(lp, "MOVE", &x4, &y4)) {
		    if (mode == POINT) {
			db_highlight(p_best);
		    }
		    rubber_set_callback(draw_mbox);
		    state = NUM4;
		    printf("got %g %g, last: %g %g\n", x4, y4, lastx, lasty);
		    /* if double click then go to state 1 */
		    
		    if (!num_moves) {
		        lastx=x3;
			lasty=y3;
		    }

		    if (num_moves && lastx == x4 && lasty == y4) {
			num_moves = 0;
			rubber_clear_callback();
			need_redraw++;
			state = START;
		    } else {
			rubber_clear_callback();
			if (mode == POINT) {
			    db_move_component(p_best, x4-lastx, y4-lasty);
			    currep->modified++;
			} else {
			    tmp = stack;
			    while (tmp!=NULL) {
				p_best = (DB_DEFLIST *) stack_walk(&tmp);
				db_move_component(p_best, x4-lastx, y4-lasty);
				currep->modified++;
			    }
			}
			rubber_set_callback(draw_mbox);
			need_redraw++;
			num_moves++;
			lastx = x4;
			lasty = y4;
			state = NUM4;
		    } 
		} else {
		    state = END;
		}
	    } else if (token == EOL) {
		token_get(lp,&word); 	/* just ignore it */
	    } else if (token == EOC) {
	    	rubber_clear_callback();
		printf("MOVE: cancelling POINT\n");
	        state = START;
	    } else if (token == CMD) {
		printf("MOVE: cancelling POINT\n");
	        state = END;
	    } else if (token == BACK) {
		token_get(lp,&word); 
		if (p_new == NULL) {
			;   /* just ignore it */
		} else {
		    db_unlink_component(currep, p_new);
		    p_new=NULL;
		    need_redraw++;
		}
	    } else {
		token_err("MOVE", lp, "expected NUMBER", token);
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
    return(1);
}


void draw_mbox(x, y, count)
double x, y;  /* offset */
int count; /* number of times called */
{
	static double x1old, x2old, y1old, y2old;
	static double xx1, yy1, xx2, yy2;
	BOUNDS bb;
	bb.init=0;

	xx1=xmin+x-x3;
	yy1=ymin+y-y3;
	xx2=xmax+x-x3;
	yy2=ymax+y-y3;

	if (count == 0) {		/* first call */
	    db_drawbounds(xx1, yy1, xx2, yy2, D_RUBBER);
	} else if (count > 0) {		/* intermediate calls */
	    db_drawbounds(x1old, y1old, x2old, y2old, D_RUBBER); 	/* erase old shape */
	    db_drawbounds(xx1, yy1, xx2, yy2, D_RUBBER);
	} else {			/* last call, cleanup */
	    db_drawbounds(x1old, y1old, x2old, y2old, D_RUBBER); 	/* erase old shape */
	}

	/* save old values */
	x1old=xx1;
	y1old=yy1;
	x2old=xx2;
	y2old=yy2;
	jump(&bb, D_RUBBER);
}


void move_draw_box(x2, y2, count)
double x2, y2;
int count; /* number of times called */
{
        static double x1old, x2old, y1old, y2old;
        BOUNDS bb;
        bb.init=0;

        if (count == 0) {               /* first call */
            db_drawbounds(x1,y1,x2,y2,D_RUBBER);                /* draw new shape */
        } else if (count > 0) {         /* intermediate calls */
            db_drawbounds(x1old,y1old,x2old,y2old,D_RUBBER);    /* erase old shape */
            db_drawbounds(x1,y1,x2,y2,D_RUBBER);                /* draw new shape */
        } else {                        /* last call, cleanup */
            db_drawbounds(x1old,y1old,x2old,y2old,D_RUBBER);    /* erase old shape */
        }

        /* save old values */
        x1old=x1;
        y1old=y1;
        x2old=x2;
        y2old=y2;
        jump(&bb, D_RUBBER);
}
