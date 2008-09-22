#include <stdio.h>
#include <string.h>		/* for strchr() */
#include <ctype.h>		/* for toupper() */
#include <math.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "rlgetc.h"
#include "db.h"
#include "token.h"
#include "xwin.h" 	
#include "lex.h"
#include "eprintf.h"
#include "opt_parse.h"

#define MAXBUF 256

/* 
    change options for a component in the current device.
*/

/* FIXME: add way to change note to text, text to note */
/* perhaps by using :N option on text, :T option on note */

int com_change(LEXER *lp, char *arg)		
{

    enum {START,NUM1,COM1,NUM2,OPT,NUM3,COM2,NUM4,END} state = START;

    TOKEN token;
    char *word;
    char buf[MAXBUF];
    int debug=0;
    int done=0;
    int retval;
    int valid_comp=0;
    double optval;
    int i;
    DB_DEFLIST *p_best = NULL;
    DB_TAB *p_tab;
    char instname[BUFSIZE];
    char *pinst = (char *) NULL;

    int my_layer = 0; 	/* internal working layer */
    int new_layer;	 

    int comp=ALL;
    static double x1,y1;
    double saverotation;

    /* check that we are editing a rep */
    if (currep == NULL ) {
	printf("must do \"EDIT <name>\" before CHA\n");
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
	    if (token == OPT ) {
		token_get(lp,&word); /* ignore for now */
		printf("CHA: ignoring option, please select component first\n");
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
		token_err("CHA", lp, "expected DESC or NUMBER", token);
		state = END;	/* error */
	    }
	    break;
	case NUM1:		/* get pair of xy coordinates */
	    if (getnum(lp, "CHA", &x1, &y1)) {		// getnum may destroy lookahead
		if (debug) printf("got comp %d, layer %d\n", comp, my_layer);
		if (p_best != NULL) {
		    db_highlight(p_best); 	/* unhighlight it */
		    p_best = NULL; 
		}

		if ((p_best=db_ident(currep, x1,y1,1,my_layer, comp, pinst)) != NULL) {
		    db_notate(p_best);	    /* print out id information */
		    db_highlight(p_best);
		    switch (p_best->type) {	/* put text in command buffer */
		        case NOTE:
			    snprintf(buf, MAXBUF, "\"%s\"", p_best->u.n->text);
			    add_history(buf);
		            /* token_set_mode(lp, 1);	set raw for testing */
			    break;
			case TEXT:
			    snprintf(buf, MAXBUF, "\"%s\"", p_best->u.t->text);
			    add_history(buf);
			    break;
			default:
			    break;
		    }
		} else {
		    printf("nothing here to change...try SHO command?\n");
		}
		state = OPT;
	    } else if ((token=token_look(lp, &word)) == EOL) {
		token_get(lp,&word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		state = END;	
	    } else {
		token_err("CHA", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case OPT:
	    if (token == OPT ) {
		token_get(lp, &word); 

		new_layer=0;
		if (word[0] == ':' && (index("L", toupper(word[1])) != NULL)) { /* parse some options locally */
		    switch (toupper(word[1])) {	
			case 'L': 		/* :L(layer) */
                            if (p_best->type == INST) {
				weprintf("cannot change layer for instances: %s\n", word); 
                                state=END;
			    }
			    if(sscanf(word+2, "%lf", &optval) != 1) {
				weprintf("invalid option argument: %s\n", word+2); 
				state=END;
			    }
			    if (optval < 0.0 ) {
				weprintf("layer must be positive: %s\n", word+2); 
				state=END;
			    }
			    if (optval > MAX_LAYER ) {
				weprintf("layer must be less than %d: %s\n",
				    MAX_LAYER, word+2); 
				state=END;
			    }
			    if ((optval-floor(optval)) != 0.0 ) {
				weprintf("layer must be an integer: %s\n", word+2); 
				state=END;
			    }
			    if (state != END) {
				new_layer = (int) optval;
				db_set_layer(p_best, new_layer);
				currep->modified++;
				need_redraw++;
			    }
			    break;
			default:
			    break;
		    }
		} else {	/* let opt_parse do the rest */
		    if (p_best != NULL) {
			switch (p_best->type) {
			    case ARC:
				retval=opt_parse(word, ARC_OPTS, (p_best->u.a->opts));
				break;
			    case CIRC:
				retval=opt_parse(word, CIRC_OPTS, (p_best->u.c->opts));
				break;
			    case INST:
				p_tab = db_lookup(p_best->u.i->name);
				saverotation = p_best->u.i->opts->rotation;
				if (p_tab->is_tmp_rep) {
				    retval=opt_parse(word, INST_OPTS, (p_best->u.i->opts));
				    if (0 && fmod(p_best->u.i->opts->rotation, 90.0) != 0.0) {
					printf("NONAMEs only rotatable by 90 deg. multiples\n");
					p_best->u.i->opts->rotation = saverotation;
				    } 
				    if (p_best->u.i->opts->aspect_ratio != 1.0) {
					printf("NONAMEs may not be aspected\n");
					p_best->u.i->opts->aspect_ratio = 1.0;
				    }
				    if (p_best->u.i->opts->slant != 0.0) {
					printf("NONAMEs may not be slanted\n");
					p_best->u.i->opts->slant = 0.0;
				    }
				} else {
				    retval=opt_parse(word, INST_OPTS, (p_best->u.i->opts));
				}
				break;
			    case LINE:
				retval=opt_parse(word, LINE_OPTS, (p_best->u.l->opts));
				break;
			    case NOTE:
				retval=opt_parse(word, NOTE_OPTS, (p_best->u.n->opts));
				break;
			    case OVAL:
				retval=opt_parse(word, OVAL_OPTS, (p_best->u.o->opts));
				break;
			    case POLY:
				retval=opt_parse(word, POLY_OPTS, (p_best->u.p->opts));
				break;
			    case RECT:
				retval=opt_parse(word, RECT_OPTS, (p_best->u.r->opts));
				break;
			    case TEXT:
				retval=opt_parse(word, TEXT_OPTS, (p_best->u.t->opts));
				break;
			    default:
				printf("unknown case in change routine\n");
				break;
			}
		    }
		    if (retval != -1) {
			state = OPT;
			currep->modified++;
			need_redraw++;
		    } else {
			state = END;
		    }
		}
	    } else if (token == EOL) {
		token_get(lp,&word); 	/* just ignore it */
	    } else if (token == QUOTE) {
		token_get(lp,&word); 	/* just ignore it */
		if (p_best->type == NOTE) {
		    free(p_best->u.n->text);
		    p_best->u.n->text = strsave(word);
		    currep->modified++;
		    need_redraw++;
		} else if(p_best->type == TEXT ) {
		    free(p_best->u.t->text);
		    p_best->u.t->text = strsave(word);
		    currep->modified++;
		    need_redraw++;
		} else {
		    printf("can't add text to this kind of component\n");
		    state = END;
		}
	    } else if (token == RAW) {	/* FIXME: a stub for testing RAW mode */
	        /* token_get(lp,&word);
		printf("RAW: %s <%02x>\n",word, (unsigned int) word[0]);
		fflush(stdout);
		if (word[0]=='x') {
		    token_set_mode(lp, 0);
		}
		*/
		token_set_mode(lp, 0);
	    } else {
		    state = START;
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


