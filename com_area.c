#include <stdio.h>
#include <string.h>		/* for strchr() */
#include <ctype.h>		/* for toupper() */

#include "rlgetc.h"
#include "db.h"
#include "token.h"
#include "xwin.h" 	
#include "lex.h"
#include "rubber.h"

/* 
    measure the area of a component in the current device.
    AREA <restrictor> { xysel } ... <EOC>
*/

int com_area(LEXER *lp, char *arg)		
{

    enum {START,NUM1,END} state = START;

    TOKEN token;
    char *word;
    int debug=1;
    int done=0;
    int valid_comp=0;
    int i;
    DB_DEFLIST *p_best;
    DB_DEFLIST *p_prev = NULL;
    double x1, y1;
    double area;
    double cum_area=0.0;
    char instname[BUFSIZE];
    char *pinst = (char *) NULL;

    int my_layer=0; 	/* internal working layer */

    int comp=ALL;

    /* check that we are editing a rep */
    if (currep == NULL ) {
	printf("must do \"EDIT <name>\" before AREA\n");
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
	    if (debug) printf("in START\n");
	    if (token == OPT ) {
		token_get(lp,&word); /* ignore for now */
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
		token_err("AREA", lp, "expected DESC or NUMBER", token);
		state = END;	/* error */
	    }
	    break;
	case NUM1:		/* get pair of xy coordinates */
	    if (debug) printf("in NUM1\n");
            if (getnum(lp, "AREA", &x1, &y1)) {
		if (debug) printf("got comp %d, layer %d\n", comp, my_layer);

		if (p_prev != NULL) {
		    db_highlight(p_prev);	/* unhighlight it */
		    p_prev = NULL;
		}

		if ((p_best=db_ident(currep, x1,y1,0,my_layer, comp, pinst)) != NULL) {
		    db_notate(p_best);	    /* print out id information */
		    db_highlight(p_best);
		    area = db_area(p_best);
		    cum_area += area;
		    if (area >= 0) {
			printf("   area = %g, total = %g\n", area, cum_area );
		    }
		    p_prev=p_best;
		    state = NUM1;
		} else {
		    printf("nothing here to measure... try SHO command?\n");
		    state = START;
		}
	    } else if ((token=token_look(lp, &word)) == EOL) {
		token_get(lp,&word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("AREA: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("AREA", lp, "expected NUMBER", token);
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


