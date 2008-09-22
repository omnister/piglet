#include <stdio.h>
#include <string.h>		/* for strchr() */
#include <ctype.h>		/* for toupper() */

#include "rlgetc.h"
#include "db.h"
#include "token.h"
#include "xwin.h" 	
#include "lex.h"
#include "rubber.h"

#define POINT  0
#define REGION 1

/* 
    delete a component from the current device.
*/

static double x1, y1;
static double x2, y2;
void delete_draw_box();
SELPNT *selpnt;
STACK *stack;

int com_delete(LEXER *lp, char *arg)		
{

    enum {START,NUM1,NUM2,END} state = START;

    TOKEN token;
    char *word;
    int valid_comp=0;
    int i;
    DB_DEFLIST *p_best;
    char instname[BUFSIZE];
    char *pinst = (char *) NULL;
    int debug=0;
    int done=0;
    int mode=POINT;

    int my_layer=0; 	/* internal working layer */

    int comp=ALL;

    /* check that we are editing a rep */
    if (currep == NULL ) {
	printf("must do \"EDIT <name>\" before DEL\n");
	token_flush_EOL(lp);
	return(1);
    }

/* 
    To create a show set for restricting the pick, we look 
    here for either a primitive indicator
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
	/* if (debug) printf("got %s: %s\n", tok2str(token), word); */
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	case START:		/* get option or first xy pair */
	    db_checkpoint(lp);
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
                            printf("DELETE: bad option: %s\n", word);
                            state=END;
                            break;
                    }
                } else {
                    printf("DELETE: bad option: %s\n", word);
                    state = END;
                }
		state = START;
	    } else if (token == NUMBER) {
		state = NUM1;
	    } else if (token == EOL) {
		token_get(lp,&word); 	/* just eat it up */
		state = START;
	    } else if (token == EOC || token == CMD) {
		state = END;
	    } else if (token == IDENT) {
		token_get(lp,&word);
	    	state = START;
		/* check to see if is a valid comp descriptor */
		valid_comp=0;
		if ((comp = is_comp(toupper(word[0])))) {
		    if (strlen(word) == 1) {
			/* my_layer = default_layer();
			printf("using default layer=%d\n",my_layer); */
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
		token_err("DEL", lp, "expected DESC or NUMBER", token);
		state = END;	/* error */
	    }
	    break;
	case NUM1:
            if (getnum(lp, "DELETE", &x1, &y1)) {
		if (mode == POINT) {
		    if (debug) printf("got comp %d, layer %d\n", comp, my_layer);
		    if ((p_best=db_ident(currep, x1,y1,1,my_layer, comp, pinst)) != NULL) {
			db_notate(p_best);	    /* print out id information */
			db_unlink_component(currep, p_best); 
			need_redraw++;
		    } else {
			printf("nothing here to delete...try SHO command?\n");
		    }
		    state = START;
		} else {
                    rubber_set_callback(delete_draw_box);
                    state = NUM2;
		}
	    } else if ((token=token_look(lp, &word)) == EOL) {
		token_get(lp,&word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("DEL: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("DEL", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
        case NUM2:
            if (getnum(lp, "DELETE", &x2, &y2)) {	// getnum may destoy lookahead
                state = START;
                rubber_clear_callback();
                need_redraw++;

                printf("DELETE: got %g,%g %g,%g\n", x1, y1, x2, y2);
                stack=db_ident_region(currep, x1,y1, x2, y2, 0, my_layer, comp, pinst);
		if (stack == NULL) {
                    printf("nothing to delete, try SHO #E?\n");
                } else {
		    while ((p_best = (DB_DEFLIST *) stack_pop(&stack))!=NULL) {
			db_notate(p_best);	    /* print out id information */
			db_unlink_component(currep, p_best); 
			need_redraw++;
		    }
                }
	    } else if ((token=token_look(lp, &word)) == EOL) {
                token_get(lp,&word);     /* just ignore it */
            } else if (token == EOC || token == CMD) {
                printf("DELETE: cancelling POINT\n");
                state = END;
            } else {
                token_err("DELETE", lp, "expected NUMBER", token);
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
    return(1);
}

void delete_draw_box(x2, y2, count)
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


