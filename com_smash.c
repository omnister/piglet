#include <ctype.h>
#include <math.h>

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
 *     Select instance at <coord>. Optionally restrict selection to one of <device_name>.
 *     First coord simply highlights the instance.  The SMAsh is only performed after
 *     a double-click on the same <coord>.  A semi-colon (;) terminates the SMAsh command.
 *     Using  a double-click allows multiple SMAshes to be performed with one command.
 * 
 *     Abort command if <device> is scaled, rotated, or sheared.
 * 
 *     walk the DB_TAB for selected <device> copying each DB_DEFLIST to currep
 *     with appropriate offset.
 * 
 *     Delete original <device>.
 *
 */

static double x1, yy1;		/* funny name for yy1 to avoid conflict with <math.h> */
static double lastx1, lasty1;
STACK *stack;

int com_smash(LEXER *lp, char *arg)
{
    enum {START,NUM1,COM1,NUM2,NUM3,COM2,NUM4,END} state = START;

    int done=0;
    TOKEN token;
    char word[BUFSIZE];
    int debug=0;
    DB_DEFLIST *p_best = NULL;
    DB_DEFLIST *p_prev = NULL;
    DB_DEFLIST *p;
    DB_DEFLIST *dp;
    char *instname=NULL;
    int ncoords=0;
    DB_TAB *smashrep;
    
    while (!done) {
	token = token_look(lp,word);
	if (debug) printf("got %s: %s\n", tok2str(token), word); 
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	case START:		/* get option or first xy pair */
	    db_checkpoint(lp);
	    if (token == OPT ) {
		token_get(lp,word);  
		if (word[0]==':') {
		    switch (toupper(word[1])) {
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
		token_get(lp,word); 	/* just eat it up */
		state = START;
	    } else if (token == EOC || token == CMD) {
		 state = END;
	    } else if (token == IDENT) {
		token_get(lp,word);  
		instname = strsave(word);
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

	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &x1);	/* scan it in */
		state = COM1;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		state = END;	
	    } else {
		token_err("SMASH", lp, "expected NUMBER", token);
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
		token_err("SMASH", lp, "expected COMMA", token);
	        state = END;
	    }
	    break;
	case NUM2:
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &yy1);	/* scan it in */

		if (p_prev != NULL) {
		    db_highlight(p_prev);	/* unhighlight it */
		    p_prev = NULL;
		}

		if (ncoords && lastx1 == x1 && lasty1 == yy1) {	/* double click */

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
		    p_best=NULL;
		    ncoords=0;

		} else {
		    if ((p_best=db_ident(currep, x1,yy1, 1, 0, INST, instname)) != NULL) {
			db_highlight(p_best);	
			db_notate(p_best);	/* print information */
			p_prev=p_best;
		    }
		}
		ncoords++;
		state = START;

	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
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

