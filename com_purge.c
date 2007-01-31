#include <stdio.h>
#include <string.h>		/* for strchr() */
#include <ctype.h>		/* for toupper() */

#include "rlgetc.h"
#include "db.h"
#include "token.h"
#include "xwin.h" 	
#include "lex.h"
#include "rubber.h"

int com_purge(lp, arg)		/* remove device from memory and disk */
LEXER *lp;
char *arg;
{
    TOKEN token;
    int done=0;
    char word[BUFSIZE];
    int editlevel;

    while(!done && (token=token_get(lp, word)) != EOF) {
	switch(token) {
	    case IDENT: 	/* identifier */
                if (currep != NULL && strncmp(word, currep->name, BUFSIZE) == 0) {
		    editlevel=currep->being_edited;
		    currep->being_edited = 0;
		    if ((currep = db_edit_pop(editlevel)) != NULL) {
			;
		    } else {
			lp->mode = MAIN;
		    }
		    db_purge(lp, word);
		    need_redraw++;
		} else {
		    db_purge(lp, word);
		}
		done++;
	    	break;
	    case CMD:		/* command */
		token_unget(lp, token, word);
		done++;
		break;
	    case EOC:		/* end of command */
		done++;
		break;
	    default:
		; /* eat em up! */
	    	break;
	}
    }
    return (0);
}

