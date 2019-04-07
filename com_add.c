#include <stdio.h>
#include <string.h>		/* for strchr() */
#include <ctype.h>		/* for toupper */

#include "rlgetc.h"
#include "db.h"
#include "token.h"
#include "xwin.h" 	

#define UNUSED(x) (void)(x)

/* 
    Add a component to the current device.
    This routine checks for <component[layer]> or <instance_name>
    and dispatches control the the appropriate add_<comp> subroutine
*/

int com_add(LEXER *lp, char *arg)		
{
    UNUSED(arg);
    TOKEN token;
    char *word;
    int done=0;
    int valid_comp=0;
    int i;
    int layer;

    int comp;
    int debug=0;

    /* check that we are editing a rep */
    if (currep == NULL ) {
	printf("must do \"EDIT <name>\" before ADD\n");
	token_flush_EOL(lp);
	return(1);
    }

/* 
    To dispatch to the proper add_<comp> routine, we look 
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
	token = token_get(lp, &word);
	if (token == IDENT) { 	

	    /* check to see if is a valid comp descriptor */
	    valid_comp=0;
	    if ((comp = is_comp(toupper((unsigned char)word[0])))) {
	    	if (strlen(word) == 1) {
		    layer=default_layer();
		    printf("using default layer=%d\n",layer);
		    valid_comp++;	/* no layer given */
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
			if(sscanf(&word[1], "%d", &layer) == 1 && 
			    layer >= 0 && layer <= MAX_LAYER) {
			    if (debug) printf("given layer=%d\n",layer);
			} else {
			    valid_comp=0;
			}
		    } 
		    if (valid_comp) {
		        if (layer > MAX_LAYER) {
			    printf("layer must be less than %d\n",
				MAX_LAYER);
			    valid_comp=0;
			    done++;
			}
			if (!show_check_modifiable(currep, comp, layer)) {
			    printf("layer %d is not modifiable!\n",
				layer);
			    token_flush_EOL(lp);
			    valid_comp=0;
			    done++;
			}
		    }
		}
	    } 

	    if (!done) {
		if (valid_comp) {
		    switch (comp) {
			case ARC:
			    add_arc(lp, &layer);
			    break;
			case CIRC:
			    add_circ(lp, &layer);
			    break;
			case LINE:
			    add_line(lp, &layer);
			    break;
			case NOTE:
			    add_note(lp, &layer);  
			    break;
			case OVAL:	/* synonym for oval */
			    add_oval(lp, &layer);
			    break;
			case POLY:
			    add_poly(lp, &layer);
			    break;
			case RECT:
			    add_rect(lp, &layer);
			    break;
			case TEXT:
			    add_text(lp, &layer);
			    break;
			default:
			    printf("invalid comp name\n");
			    break;
		    }
		} else {  /* must be a identifier */
		    /* check to see if "ADD I <name>" */
		    if ((strlen(word) == 1) && toupper((unsigned char)word[0]) == 'I') {
			if((token=token_get(lp,&word)) != IDENT) {
			    printf("ADD INST: bad inst name: %s\n", word);
			    done++;
			}
		    }
		    if (debug) printf("calling add_inst with %s\n", word);
		    if (!show_check_modifiable(currep, INST, 0)) {
			    printf("INST component is not modifiable!\n");
		    } else {
			add_inst(lp, word);
		    }
		}
	    }
	} else if (token == QUOTE) {
	    if (!show_check_modifiable(currep, INST, 0)) {
		    printf("INST component is not modifiable!\n");
	    } else {
		add_inst(lp, word);
	    }
	} else if (token == CMD) { /* return control back to top */
	    token_unget(lp, token, word);
	    done++;
	} else if (token == EOL) {
	    ; /* ignore */
	} else if (token == EOC) {
	    done++;
	} else if (token == EOF) {
	    done++;
	} else {
	    printf("ADD: expected COMP/LAYER or INST name: %d\n", token);
	    token_flush_EOL(lp);
	    done++;
	}
    }
    return(0);
}

