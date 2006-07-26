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
void wrap_draw_box();
STACK *stack;

/* 
    wrap a set of components in the current device.
    WRAP [<restrictor>] [<devicename>] { xyorig coord1 coord2 } ... <EOC>
*/

int com_wrap(LEXER *lp, char *arg)		
{

    enum {START,NUM1,COM1,NUM2,NUM3,COM2,NUM4,NUM5,COM3,NUM6,NUM7,COM4,NUM8,END} state = START;

    int done=0;
    TOKEN token;
    char word[BUFSIZE];
    int debug=0;
    int valid_comp=0;
    int i;
    DB_DEFLIST *p_best;
    DB_DEFLIST *p_new;
    DB_TAB *newrep;
    OPTS *opts;
    char *wrap_inst_name = NULL;
    double tmp;

    int my_layer=0; 	/* internal working layer */

    int comp=ALL;

    /* check that we are editing a rep */
    if (currep == NULL ) {
	printf("must do \"EDIT <name>\" before WRAP\n");
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
    rl_setprompt("WRAP> ");
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
		token_get(lp,word); 
                if (word[0]==':') {
                    switch (toupper(word[1])) {
                        default:
                            printf("WRAP: bad option: %s\n", word);
                            state=END;
                            break;
                    }
                } else {
                    printf("WRAP: bad option: %s\n", word);
                    state = END;
                }
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
	    	state = START;
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
		} 

		if (valid_comp==0) {
		    /* must be the new name of the wrapped instance */ 
		    if (db_exists(word)) {
		        printf("can't wrap onto an existing cell!\n");
		        state = END;
		    } else {
			wrap_inst_name = strsave(word);
			printf("got %s wrap_inst_name\n", wrap_inst_name);
		    }
		}
	    } else {
		token_err("WRAP", lp, "expected DESC or NUMBER", token);
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
		token_err("WRAP", lp, "expected NUMBER", token);
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
		token_err("WRAP", lp, "expected COMMA", token);
	        state = END;
	    }
	    break;
	case NUM2:
	    if (debug) printf("in NUM2\n");
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &y1);	/* scan it in */
		state = NUM3; 
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("WRAP: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("WRAP", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
        case NUM3:              /* get pair of xy coordinates */
            if (token == NUMBER) {
                token_get(lp,word);
                sscanf(word, "%lf", &x2);       /* scan it in */
                state = COM2;
            } else if (token == EOL) {
                token_get(lp,word);     /* just ignore it */
            } else if (token == EOC || token == CMD) {
                state = END;
            } else {
                token_err("IDENT", lp, "expected NUMBER", token);
                state = END;
            }
            break;
        case COM2:
            if (token == EOL) {
                token_get(lp,word); /* just ignore it */
            } else if (token == COMMA) {
                token_get(lp,word);
                state = NUM4;
            } else {
                token_err("IDENT", lp, "expected COMMA", token);
                state = END;
            }
            break;
        case NUM4:
            if (token == NUMBER) {
                token_get(lp,word);
                sscanf(word, "%lf", &y2);       /* scan it in */
		rubber_set_callback(wrap_draw_box);
                state = NUM5;
            } else if (token == EOL) {
                token_get(lp,word);     /* just ignore it */
            } else if (token == EOC || token == CMD) {
                printf("IDENT: cancelling POINT\n");
                state = END;
            } else {
                token_err("IDENT", lp, "expected NUMBER", token);
                state = END;
            }
            break;

	case NUM5:
	    if (debug) printf("in NUM5\n");
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &x3);	/* scan it in */
		state = COM3;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		state = END;	
	    } else {
		token_err("WRAP", lp, "expected NUMBER", token);
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
		token_err("WRAP", lp, "expected COMMA", token);
	        state = END;
	    }
	    break;
	case NUM6:
	    if (debug) printf("in NUM6\n");
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &y3);	/* scan it in */
		printf("got %g %g\n", x3, y3);
		rubber_clear_callback();
		
		if (x2 > x3) {
		   tmp = x3; x3 = x2; x2 = tmp;
		}
		if (y2 > y3) {
		   tmp = y3; y3 = y2; y2 = tmp;
		}

                stack=db_ident_region(currep, x2, y2, x3, y3, 1, my_layer, comp, 0);

		if (wrap_inst_name != NULL) {
		    newrep = (DB_TAB *) db_install(wrap_inst_name);
		} else {
		    newrep = (DB_TAB *) db_install("dummy");
		}

		while ((p_best = (DB_DEFLIST *) stack_pop(&stack))!=NULL) {
		    p_new = db_copy_component(p_best);
		    db_unlink_component(currep, p_best);
		    db_move_component(p_new, -x1, -y1);
		    db_insert_component(newrep, p_new);
		}
		opts = opt_create();
		opt_set_defaults(opts);
		show_init(newrep);
		newrep->minx = min(x2-x1, x3-x1);
		newrep->maxx = max(x2-x1, x3-x1);
		newrep->miny = min(y2-y1, y3-y1);
		newrep->maxy = max(y2-y1, y3-y1);
		newrep->modified++;
		db_add_inst(currep, newrep, opts, x1, y1);

		currep->modified++;
		need_redraw++;
		state = END;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("WRAP: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("WRAP", lp, "expected NUMBER", token);
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
	    if (!show_check_modifiable(currep, INST, my_layer)) {
		    printf("INST component is not modifiable!\n");
	    } else {
		;
	    }
	}
    }
} else if (token == QUOTE) {
    if (!show_check_modifiable(currep, INST, my_layer)) {
	    printf("INST component is not modifiable!\n");
    } else {
	;
*/

void wrap_draw_box(x3, y3, count)
double x3, y3;
int count; /* number of times called */
{
        static double x2old, x3old, y2old, y3old;
        BOUNDS bb;
        bb.init=0;

        if (count == 0) {               /* first call */
            db_drawbounds(x2,y2,x3,y3,D_RUBBER);                /* draw new shape */
        } else if (count > 0) {         /* intermediate calls */
            db_drawbounds(x2old,y2old,x3old,y3old,D_RUBBER);    /* erase old shape */
            db_drawbounds(x2,y2,x3,y3,D_RUBBER);                /* draw new shape */
        } else {                        /* last call, cleanup */
            db_drawbounds(x2old,y2old,x3old,y3old,D_RUBBER);    /* erase old shape */
        }

        /* save old values */
        x2old=x2;	/* global */
        y2old=y2;
        x3old=x3;
        y3old=y3;
        jump(&bb, D_RUBBER);
}

