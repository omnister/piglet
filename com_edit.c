#include <stdio.h>
#include <string.h>		/* for strchr() */
#include <ctype.h>		/* for toupper */
#include <math.h>

#include <readline/readline.h> 	/* for command line editing */
#include <readline/history.h>  

#include "rlgetc.h"
#include "db.h"
#include "token.h"
#include "xwin.h" 	
#include "lex.h"

com_edit(lp, arg)		/* begin edit of an old or new device */
LEXER *lp;
char *arg;
{
    TOKEN token;
    int done=0;
    char word[128];
    char name[128];
    char buf[128];
    int error=0;
    int debug=0;
    int nfiles=0;
    FILE *fp;
    LEXER *my_lp;
    DB_TAB *save_rep;  
    DB_TAB *new_rep;  
    DB_TAB *old_rep = NULL;  
   

    if (debug) printf("    com_edit <%s>\n", arg); 

    name[0]=0;
    while(!done && (token=token_get(lp, word)) != EOF) {
	switch(token) {
	    case IDENT: 	/* identifier */
		if (nfiles == 0) {
		    strncpy(name, word, 128);
		    nfiles++;
		} else {
		    printf("EDIT: wrong number of args\n");
		    return(-1);
		}
	    	break;
	    case QUOTE: 	/* quoted string */
	    case OPT:		/* option */
	    case END:		/* end of file */
	    case NUMBER: 	/* number */
	    case COMMA:		/* comma */
		printf("EDIT: expected IDENT: got %s\n", tok2str(token));
	    	break;
	    case EOL:		/* newline or carriage return */
	    	break;	/* ignore */
	    case EOC:		/* end of command */
		done++;
		break;
	    case CMD:		/* command */
	    	token_unget(lp, token, word);
		done++;
		break;
	    default:
		eprintf("bad case in com_edi");
		break;
	}
    }

    if (lp->mode == MAIN || currep == NULL  || lp->mode == EDI) {

	if (lp->mode == EDI) {	
	    if (currep != NULL) {
		/* currep->being_edited = 0;  */

		old_rep = currep;

		/* the old policy was to just dump it if it was not modified */
		/* Now we leave being_edited */
		/* alone and use it to manage an edit stack.  When we EXIT a cell */
		/* with being_edited = N, we then re-edit the next cell in the db */
		/* with the highest being_edited < N.  We have to do a search for */
		/* the biggest one because of the possibility that a cell could be */
		/* purged along the way... */
	    }
	}

	/* check for a name provided */
	if (strlen(name) == 0) {
	    printf("    must provide a name: EDIT <name>\n");
	    return (0);
	} 

	if (debug) printf("got %s\n", name); 

	lp->mode = EDI;
    
	if ((new_rep=db_lookup(name)) == NULL) { /* Not already in memory */

	    currep = db_install(name);  	/* create blank stub */
	    show_init();
	    need_redraw++;

	    /* now check if on disk */
	    snprintf(buf, MAXFILENAME, "./cells/%s.d", name);
	    if((fp = fopen(buf, "r")) == 0) {
		/* cannot find copy on disk */
		/* so leave the user with an empty start */
		if (debug) printf("calling dowin 1\n");
		do_win(lp, 4, -100.0, -100.0, 100.0, 100.0, 1.0);
		if (old_rep == NULL ) {
		    currep->being_edited = 1;
		} else {
		    /* push the stack */
		    currep->being_edited = old_rep->being_edited+1;
		}
	    } else {
		/* read it in */
		xwin_display_set_state(D_OFF);
		my_lp = token_stream_open(fp, buf);
		my_lp->mode = EDI;
		save_rep=currep;
		printf ("reading %s from disk\n", name);
		/* FIXME should save and restore show sets or store them in currep */
		show_set_modify(ALL,0,1);
		parse(my_lp);
		token_stream_close(my_lp); 
		if (debug) printf ("done reading %s from disk\n", name);
		show_set_modify(ALL,0,0);
		show_set_visible(ALL,0,1);
		xwin_display_set_state(D_ON);
		
		currep=save_rep;
		if (debug) printf("calling dowin 2\n");
		do_win(lp, 4, currep->vp_xmin, currep->vp_ymin, currep->vp_xmax, currep->vp_ymax, 1.0); 
		currep->modified = 0;

		if (old_rep == NULL ) {
		    currep->being_edited = 1;
		} else {
		    /* push the stack */
		    currep->being_edited = old_rep->being_edited+1;
		}

		show_init();
		need_redraw++;
	    }
	} else {			/* was already in memory */
	    if (new_rep->being_edited) {
	       printf("can't edit the same cell twice!\n");
	       return(0);
	    } else {
		currep = new_rep;
		xwin_window_set(
		    currep->minx, 
		    currep->miny,
		    currep->maxx,
		    currep->maxy
		);
		if (old_rep != NULL) {
		    currep->being_edited = old_rep->being_edited+1;
		} else {
		    currep->being_edited = 1;
		}
	    } 
	}	
    } else if (lp->mode == EDI) {
	printf("    must SAVE current device before new EDIT\n");
    } else {
	printf("    must EXIT before entering EDIT subsystem\n");
    }
    if (debug) printf("leaving  com_edit <%s>\n", arg); 
    return (0);
}

