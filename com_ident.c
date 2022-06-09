#include <stddef.h>
#include <ctype.h>
#include <string.h>

#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"
#include "rlgetc.h"

#define UNUSED(x) (void)(x)

#define POINT  0
#define REGION 1

/*
 *
 * IDE [:R xy1 xy2] | [[:p] xypnt] ... EOC
 *	highlight and print out details of rep under xypnt.
 *	  no refresh is done.
 * OPTIONS:
 *      :R identify by region (requires 2 points to define a region)
 *      :P identify by point (default mode: requires single coordinate) 
 *
 * NOTE: default is pick by point.  However, if you change to pick
 *       by region with :R, you can later go back to picking by point with
 *       the :P option.
 *
 */

static double x1, y1;
static double x2, y2;
void ident_draw_box();

int com_identify(LEXER *lp, char *arg)
{
    UNUSED(arg);
    enum {START,NUM1,NUM2,END} state = START;

    TOKEN token;
    char *word;
    char instname[BUFSIZE];
    DB_DEFLIST *p_best;
    double area;
    DB_DEFLIST *p_prev = NULL;
    char *pinst = (char *) NULL;
    size_t i;
    int debug=0;
    int mode=POINT;
    int done=0;
    int my_layer=0;
    int comp=ALL;
    int valid_comp=0;
    STACK *stack;
    
    while (!done) {
	token = token_look(lp,&word);
	if (debug) printf("got %s: %s\n", tok2str(token), word); 
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	case START:		/* get option or first xy pair */
	    if (token == OPT ) {
		token_get(lp,&word); 
		if (word[0]==':') {
		    switch (toupper((unsigned char)word[1])) {
		        case 'R':
			    mode = REGION;
			    break;
			case 'P':
			    mode = POINT;
			    break;
			default:
			    printf("IDENT: bad option: %s\n", word);
			    state=END;
			    break;
		    } 
		} else {
		    printf("IDENT: bad option: %s\n", word);
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
                state = NUM1;
                /* check to see if is a valid comp descriptor */
                valid_comp=0;
                if ((comp = is_comp(toupper((unsigned char)word[0])))) {
                    if (strlen(word) == 1) {
                        /* my_layer = default_layer();
                        printf("using default layer=%d\n",my_layer); */
                        valid_comp++;   /* no layer given */
                    } else {
                        valid_comp++;
                        /* check for any non-digit characters */
                        /* to allow instance names like "L1234b" */
                        for (i=0; i<strlen(&word[1]); i++) {
                            if (!isdigit((unsigned char)word[1+i])) {
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
		token_err("IDENT", lp, "expected NUMBER", token);
		state = END;	/* error */
	    }
	    break;
	case NUM1:
	    if (token == NUMBER) {
		if (getnum(lp, "IDENT", &x1, &y1)) {
		    if (mode==POINT) {
			if (p_prev != NULL) {
			    db_highlight(p_prev);	/* unhighlight it */
			    p_prev = NULL;
			}
			if ((p_best=db_ident(currep, x1,y1,0, my_layer, comp , pinst)) != NULL) {
			    db_highlight(p_best);	
			    printdef(stdout, p_best, NULL);
			    db_notate(p_best);	/* print information */
			    area = db_area(p_best);
			    if (area >= 0) {
				printf("   area = %g\n", area);
			    }
			    p_prev=p_best;
			}
			state = START;
		    } else {			/* mode == REGION */
			rubber_set_callback(ident_draw_box);
			state = NUM2;
		    }
	        } else {
		    state = END;
		}
	    } else if (token == EOL) {
		token_get(lp,&word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("IDENT: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("IDENT", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case NUM2:
	    if (token == NUMBER) {
		if (getnum(lp, "IDENT", &x2, &y2)) {
		    state = START;
		    rubber_clear_callback();

		    printf("IDENT: got %g,%g %g,%g\n", x1, y1, x2, y2);
		    stack=db_ident_region(currep, x1,y1, x2, y2, 0, my_layer, comp, pinst);
		    while ((p_best = (DB_DEFLIST *) stack_pop(&stack))!=NULL) {
			db_highlight(p_best);	
			printdef(stdout, p_best, NULL);
			db_notate(p_best);		/* print information */
			area = db_area(p_best);
			if (area >= 0) {
			    printf("   area = %g\n", area);
			}
		    }
		    need_redraw++;
	        } else {
		    state = END;
		}
	    } else if (token == EOL) {
		token_get(lp,&word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("IDENT: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("IDENT", lp, "expected NUMBER", token);
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
    if (p_prev != NULL) {
	db_highlight(p_prev);	/* unhighlight any remaining component */
    }
    return(1);
}

void ident_draw_box(double x2, double y2, int count)
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

