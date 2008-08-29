#include <ctype.h>
#include <math.h>
#include <string.h>

#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"
#include "lex.h"
#include "rlgetc.h"

#define POINT  0
#define REGION 1

/*
 *
 * SMASH [<device_name>] {<coord>} EOC
 * 
 *     Select instance at <coord>.  Optionally restrict selection to one
 *     of <device_name>.  First coord simply highlights the instance. 
 *     The SMAsh is only performed after a double-click on the same
 *     <coord>.  A semi-colon (;) terminates the SMAsh command.  Using a
 *     double-click allows multiple SMAshes to be performed with one
 *     command. 
 * 
 * 
 *     Abort command if <device> is scaled, rotated, or sheared. 
 * 
 * 
 *     walk the DB_TAB for selected <device> copying each DB_DEFLIST to
 *     currep with appropriate offset. 
 * 
 * 
 *     Delete original <device>. 
 *
 *
 */

STACK *stack;
static double x1, yy1;		/* y1 name conflicts with <math.h> */
static double x2, y2;
static double lastx1, lasty1;
void smash_draw_box();

void smashrep(DB_DEFLIST *p_best) {

    DB_TAB *smashrep;
    DB_DEFLIST *p;
    DB_DEFLIST *dp;

    if ( p_best != NULL &&   /* test for skew */
	(p_best->u.i->opts->aspect_ratio == 1.0) &&
	(p_best->u.i->opts->slant == 0.0)) {

	/* (p_best->u.i->opts->mirror == 0) &&  
	(fmod(p_best->u.i->opts->rotation, 90.0) == 0.0 ) &&
	(p_best->u.i->opts->scale == 1.0) && */

	smashrep=db_lookup(p_best->u.i->name);

	for (p=smashrep->dbhead; p!=(struct db_deflist *)0; p=p->next) {
	    /* printf("   type = %d\n", p->type); */
	    dp=db_copy_component(p, p_best);
	    /* db_move_component(dp, p_best->u.i->x, p_best->u.i->y); */
	    db_insert_component(currep, dp);
	}

	db_unlink_component(currep, p_best);
	currep->modified++;
	need_redraw++;

    } else {
	if (p_best != NULL) { 
	    if (fmod(p_best->u.i->opts->rotation, 90.0) != 0.0 ) {
	       printf("can't smash instances with non-orthogonal rotations\n");
	    } else if (p_best->u.i->opts->aspect_ratio != 1.0) {
	       printf("can't smash instances with non-unity aspect ratios\n");
	    } else if (p_best->u.i->opts->slant != 0.0) {
	       printf("can't smash slanted instances\n");
	    } else {
	       printf("can't smash this instance\n");
	    }
	} else {
	   printf("nothing here to smash, try SHO #e command?\n");
	}

    }
}


int com_smash(LEXER *lp, char *arg)
{
    enum {START,NUM1,NUM2,END} state = START;

    int done=0;
    TOKEN token;
    char *word;
    int debug=0;
    DB_DEFLIST *p_best = NULL;
    DB_DEFLIST *p_prev = NULL;
    char instname[BUFSIZE];
    char *pinst = NULL;
    int mode = POINT;
    int ncoords=0;
    
    while (!done) {
	token = token_look(lp,&word);
	if (debug) printf("got %s: %s\n", tok2str(token), word); 
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	case START:		/* get option or first xy pair */
	    db_checkpoint(lp);
	    if (token == OPT ) {
		token_get(lp,&word);  
		if (word[0]==':') {
		    switch (toupper(word[1])) {
			case 'R':
			    mode = REGION;
			    break;
			case 'P':
			    mode = POINT;
			    break;
			default:
			    printf("SMASH: bad option: %s\n", word);
			    state=END;
			    break;
		    } 
		} else {
		    printf("SMASH: bad option: %s\n", word);
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
		if (db_lookup(word)) {
                   strncpy(instname, word, BUFSIZE);
		   pinst = instname;
                } else {
                    printf("not a valid instance name: %s\n", word);
                    state = START;
                }
		state = NUM1;
	    } else {
		token_err("SMASH", lp, "expected NUMBER", token);
		state = END;	/* error */
	    }
	    break;
	case NUM1:		/* get pair of xy coordinates */
            if (ncoords) {
	       lastx1=x1;
	       lasty1=yy1;
	    }
            if (getnum(lp, "SMASH", &x1, &yy1)) {
		if (mode == POINT) {
		    if (p_prev != NULL) {
			db_highlight(p_prev);	/* unhighlight it */
			p_prev = NULL;
		    }
		    if (ncoords && lastx1 == x1 && lasty1 == yy1) {	/* double click */
			smashrep(p_best);
			p_best=NULL;
			ncoords=0;
		    } else {
			if ((p_best=db_ident(currep, x1,yy1, 1, 0, INST, pinst)) != NULL) {
			    db_highlight(p_best);	
			    db_notate(p_best);	/* print information */
			    p_prev=p_best;
			}
		    }
		    ncoords++;
		    state = START;
		} else {			/* mode == REGION */
		    rubber_set_callback(smash_draw_box);
		    state = NUM2;
		}
	    } else if (token == EOL) {
		token_get(lp,&word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("SMASH: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("SMASH", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;

	case NUM2:		/* get pair of xy coordinates */
	    if (getnum(lp, "SMASH", &x2, &y2)) {
		state = START;
		rubber_clear_callback();

		printf("SMASH: got %g,%g %g,%g\n", x1, yy1, x2, y2);
		stack=db_ident_region(currep, x1,yy1, x2, y2, 1, 0, INST, pinst);
		while ((p_best = (DB_DEFLIST *) stack_pop(&stack))!=NULL) {
		    printdef(stdout, p_best, NULL);
		    db_notate(p_best);          /* print information */
		    smashrep(p_best);
		}
		need_redraw++;
	    } else if (token == EOL) {
		token_get(lp,&word);     /* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("SMASH: cancelling POINT\n");
		state = END;
	    } else {
		token_err("SMASH", lp, "expected NUMBER", token);
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
    if (p_prev != NULL) {
	db_highlight(p_prev);	/* unhighlight any remaining component */
    }
    return(1);
}

void smash_draw_box(x2, y2, count)
double x2, y2;
int count; /* number of times called */
{
        static double x1old, x2old, y1old, y2old;
        BOUNDS bb;
        bb.init=0;

        if (count == 0) {               /* first call */
            db_drawbounds(x1,yy1,x2,y2,D_RUBBER);                /* draw new shape */
        } else if (count > 0) {         /* intermediate calls */
            db_drawbounds(x1old,y1old,x2old,y2old,D_RUBBER);    /* erase old shape */
            db_drawbounds(x1,yy1,x2,y2,D_RUBBER);                /* draw new shape */
        } else {                        /* last call, cleanup */
            db_drawbounds(x1old,y1old,x2old,y2old,D_RUBBER);    /* erase old shape */
        }

        /* save old values */
        x1old=x1;
        y1old=yy1;
        x2old=x2;
        y2old=y2;
        jump(&bb, D_RUBBER);
}

