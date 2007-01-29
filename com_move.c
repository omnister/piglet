#include <stdio.h>
#include <string.h>		/* for strchr() */
#include <ctype.h>		/* for toupper() */

#include "rlgetc.h"
#include "db.h"
#include "token.h"
#include "xwin.h" 	
#include "lex.h"
#include "rubber.h"
#include "lock.h"

#define POINT 0
#define REGION 1

static double x1, y1, x2, y2, x3, y3, x4, y4, x4old, y4old;
void draw_bbox();
void move_draw_point();
void move_draw_box();
SELPNT *selpnt, *tmp;

int getnum(LEXER *lp, char *cmd, double *px, double *py)
{
    int done=0;
    TOKEN token;
    char word[BUFSIZE];
    int state = 1;
    int debug = 0;

   printf("in getnum\n");

    while(!done) {
	token = token_look(lp,word);
	if (debug) printf("got %s: %s\n", tok2str(token), word);
	if (token==CMD) {
	    state=4;
	} 

	switch(state) {	
	case 1:		/* get pair of xy coordinates */
	    if (debug) printf("in NUM1\n");
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", px);	/* scan it in */
		state = 2;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		state = 4;	
	    } else {
		token_err(cmd, lp, "expected NUMBER", token);
		state = 4; 
	    }
	    break;
	case 2:		
	    if (debug) printf("in COM1\n");
	    if (token == EOL) {
		token_get(lp,word); /* just ignore it */
	    } else if (token == COMMA) {
		token_get(lp,word);
		state = 3;
	    } else {
		token_err(cmd, lp, "expected COMMA", token);
		state = 4;
	    }
	    break;
	case 3:
	    if (debug) printf("in NUM2\n");
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", py);	/* scan it in */
		return(1);
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		state = 4;
	    } else {
		token_err(cmd, lp, "expected NUMBER", token);
		state = 4; 
	    }
	    break;
	case 4:		
	    done++;
	}
    }
    return(0);	
}

/* 
    move a component in the current device.
    MOV <restrictor> { xysel xyref xynewref } ... <EOC>
*/

int com_move(LEXER *lp, char *arg)		
{

    enum {START,NUM1,COM1,NUM2,NUM3,COM2,NUM4,NUM5,COM3,NUM6,NUM7,COM4,NUM8,END} state = START;

    int done=0;
    TOKEN token;
    char word[BUFSIZE];
    char instname[BUFSIZE];
    char *pinst = (char *) NULL;
    int debug=1;
    int valid_comp=0;
    int i;
    int mode=POINT;

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
	token = token_look(lp,word);
	if (debug) printf("got %s: %s\n", tok2str(token), word);
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	case START:		/* get option or first xy pair */
	    db_checkpoint(lp);
	    if (debug) printf("in START\n");
	    if (token == OPT ) {
		token_get(lp,word);
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
		token_get(lp,word); 	/* just eat it up */
		state = START;
	    } else if (token == CMD) {
		state = END;
	    } else if (token == IDENT) {
		token_get(lp,word);
		/* check to see if is a valid comp descriptor */
		valid_comp=0;
		if ((comp = is_comp(toupper(word[0])))) {
		    if (strlen(word) == 1) {
			/* my_layer = default_layer(); */
			/* printf("using default layer=%d\n",my_layer); */
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
	case NUM1:		/* get pair of xy coordinates */
	    if (!getnum(lp, "MOVE", &x1, &y1)) {
	        state = END;
	    } else {
		if (mode == POINT) {
		    /* printf("calling db_ident, 
		        %g %g 1 layer:%d comp:%d\n", x1, y1, my_layer, comp); */
		    if ((selpnt=db_ident2(currep, x1,y1,1,my_layer, comp, pinst)) != NULL) {
			for (tmp = selpnt; tmp != NULL; tmp = tmp->next) {
			    if (tmp->p != NULL) {
				db_highlight(tmp->p);
			    }
			}
			state = NUM5;
		    } else {
			printf("nothing here to move... try SHO command?\n");
			state = START;
		    }
                 } else {		           /* mode == REGION */
                    rubber_set_callback(move_draw_box);
                    state = NUM3; 
		 }
	    }
	    break;
        case NUM3:              /* get pair of xy coordinates */
	    if (!getnum(lp, "MOVE", &x2, &y2)) {
	        state = END;
	    } else {
		rubber_clear_callback();
                selpnt=db_ident_region2(currep, x1,y1, x2, y2, 1, my_layer, comp, pinst);
		if (selpnt == NULL) {
		    printf("nothing here to move... try SHO command?\n");
		    state = END;
		} else {
		    tmp=selpnt;
                    while (tmp!=NULL) {
                        if (tmp->p != NULL) {
                            db_highlight(tmp->p);
                        }
                        tmp = tmp->next;
                     }
		    state = NUM5;
		}
	    }
            break;
	case NUM5:
	    if (!getnum(lp, "MOVE", &x3, &y3)) {
	        if ((token = token_look(lp,word)) == EOC) {
		    token_get(lp,word); 
		    rubber_clear_callback();
		    for (tmp = selpnt; tmp != NULL; tmp = tmp->next) {
			if (tmp->xsel != NULL) {
			   *(tmp->xsel) = tmp->xselorig;
			}
			if (tmp->ysel != NULL) {
			   *(tmp->ysel) = tmp->yselorig;
			}
			if (tmp->p != NULL) {
			    db_highlight(tmp->p);
			}
		    }
		    selpnt_clear(&selpnt);	
		    currep->modified++;
		    need_redraw++;
		    state = START;
		} else {
		    state = END;
		}
	    } else {
		x4 = x3; y4 = y3;
		if (debug) printf("got %g %g\n", x3, y3);
		rubber_set_callback(move_draw_point);
		state = NUM7;
	    }
	    break;
	case NUM7:
	    if (debug) printf("in NUM7\n");
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &x4);	/* scan it in */
		state = COM4;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC) {
		token_get(lp,word); 
		rubber_clear_callback();
		move_draw_point(x4, y4, 0);
		selpnt_clear(&selpnt);	
		currep->modified++;
		need_redraw++;
		state = START;
	    } else {
		token_err("MOVE", lp, "expected NUMBER", token);
		printf("aborting MOV\n");
		rubber_clear_callback();
		move_draw_point(x3, y3, 0);
		state = END;	
	    }
	    break;
	case COM4:		
	    if (debug) printf("in COM4\n");
	    if (token == EOL) {
		token_get(lp,word); /* just ignore it */
	    } else if (token == COMMA) {
		token_get(lp,word);
		state = NUM8;
	    } else {
		token_err("MOVE", lp, "expected COMMA", token);
		printf("aborting MOV\n");
		rubber_clear_callback();
		move_draw_point(x3, y3, 0);
		state = END;	
	    }
	    break;
	case NUM8:
	    if (debug) printf("in NUM8\n");
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &y4);	/* scan it in */
		if (debug) printf("got %g %g\n", x4, y4);
		if (x4old == x4 && y4old == y4) { /* double click */	
		    rubber_clear_callback();
		    move_draw_point(x4, y4, 0);
		    selpnt_clear(&selpnt);	
		    currep->modified++;
		    need_redraw++;
		    state = START;
		} else {
		    move_draw_point(x4, y4, 0);
		    currep->modified++;
		    need_redraw++;
		    state = NUM7;		/* RCW XX */
		}
		x4old=x4; y4old=y4;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else {
		printf("MOVE: cancelling POINT\n");
		printf("aborting MOV\n");
		rubber_clear_callback();
		move_draw_point(x3, y3, 0);
		state = END;	
	    }
	    break;
	case END:
	default:
	    if (token == CMD) {
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


void move_draw_point(double xx, double yy, int count)
{
        static double xxold, yyold;
        BOUNDS bb;
        bb.init=0;
	int debug = 0;
	SELPNT *tmp;

	if (debug) {
	   printf("in move_draw_point: %g %g\n", xx, yy);
	}
        
	/* 
	setlockpoint(x3, y3);
	lockpoint(&xx, &yy, currep->lock_angle);
	*/

        if (count == 0) {               /* first call */
	    for (tmp = selpnt; tmp != NULL; tmp = tmp->next) {
	        if (tmp->xsel != NULL) {
		   *(tmp->xsel) = tmp->xselorig + xx - x3;
		}
	        if (tmp->ysel != NULL) {
		   *(tmp->ysel) = tmp->yselorig + yy - y3;
		}
		if (tmp->p != NULL) {
		    db_highlight(tmp->p);
		}
	    }
        } else if (count > 0) {         /* intermediate calls */
	    for (tmp = selpnt; tmp != NULL; tmp = tmp->next) {
	        if (tmp->xsel != NULL) {
		   *(tmp->xsel) = tmp->xselorig + xxold - x3;
		}
	        if (tmp->ysel != NULL) {
		   *(tmp->ysel) = tmp->yselorig + yyold - y3;
		}
		if (tmp->p != NULL) {
		    db_highlight(tmp->p);
		}
	    }
	    for (tmp = selpnt; tmp != NULL; tmp = tmp->next) {
	        if (tmp->xsel != NULL) {
		   *(tmp->xsel) = tmp->xselorig + xx - x3;
		}
	        if (tmp->ysel != NULL) {
		   *(tmp->ysel) = tmp->yselorig + yy - y3;
		}
		if (tmp->p != NULL) {
		    db_highlight(tmp->p);
		}
	    }
        } else {                        /* last call, cleanup */
	    for (tmp = selpnt; tmp != NULL; tmp = tmp->next) {
	        if (tmp->xsel != NULL) {
		   *(tmp->xsel) = tmp->xselorig + xxold - x3;
		}
	        if (tmp->ysel != NULL) {
		   *(tmp->ysel) = tmp->yselorig + yyold - y3;
		}
		if (tmp->p != NULL) {
		    db_highlight(tmp->p);
		}
	    }
        }

        /* save old values */
        xxold=xx;
        yyold=yy;
        jump(&bb, D_RUBBER);
}

void move_draw_box(x2, y2, count)
double x2, y2;
int count; /* number of times called */
{
        static double x1old, x2old, y1old, y2old;
        BOUNDS bb;
        bb.init=0;
	static int called=0;

        if (count == 0) {               /* first call */
            db_drawbounds(x1,y1,x2,y2,D_RUBBER);                /* draw new shape */
	    called++;
        } else if (count > 0) {         /* intermediate calls */
            db_drawbounds(x1old,y1old,x2old,y2old,D_RUBBER);    /* erase old shape */
            db_drawbounds(x1,y1,x2,y2,D_RUBBER);                /* draw new shape */
        } else {                        /* last call, cleanup */
	    if (called) {
		db_drawbounds(x1old,y1old,x2old,y2old,D_RUBBER);    /* erase old shape */
            }
	    called=0;
        }

        /* save old values */
        x1old=x1;
        y1old=y1;
        x2old=x2;
        y2old=y2;
        jump(&bb, D_RUBBER);
}

