#include <stdio.h>
#include "db.h"
#include "xwin.h"
#include "token.h"
#include "lex.h"
#include "rlgetc.h"

/*
SHOW {+|-|#} [EACILN0PRT]<layer>

   + : make it visible
   - : turn it off
   # : visible and modifiable

   E : everything
   A : arc
   C : circle
   I : instances
   L : lines
   N : notes
   O : oval
   P : polygons
   R : rectangles
   T : text

   layer : layer number, "0" or omitted for all layers.
*/

com_show(lp, arg)		/* define which kinds of things to display */
LEXER *lp;
char *arg;
{
    TOKEN token;
    int done=0;
    int level;
    char word[128];
    int nnums=0;
    double tmp;
    int debug=0;

    int visible=0;
    int modifiable=0;
    int comp=0;
    int layer=0;

    if (lp->mode != EDI) {
    	printf("No cell currently being edited!\n");
	token_flush_EOL(lp);
	return(1);
    }

    rl_saveprompt();
    rl_setprompt("SHOW> ");

    while(!done && (token=token_get(lp, word)) != EOF) {
	switch(token) {
	    case OPT:		/* option */
		switch(toupper(word[0])) {
		    case '-':
		        visible=0;
		        modifiable=0;
		        break;
		    case '+':
		        visible=1;
		        modifiable=0;
		        break;
		    case '#':
		        visible=1;
		        modifiable=1;
		        break;
		    default:
		    	printf("SHOW: options start with one of {+-#}: %s\n", word);
			done++;
			token_flush_EOL(lp);
			break;
		    }

		    if (!done && strlen(word) > 1) {
			switch(toupper(word[1])) {
			   case 'E':
				comp=ALL;
				break;
			   case 'A':
				comp=ARC;
				break;
			   case 'C':
				comp=CIRC;
				break;
			   case 'I':
				comp=INST;
				break;
			   case 'L':
				comp=LINE;
				break;
			   case 'N':
				comp=NOTE;
				break;
			   case 'O':
				comp=OVAL;
				break;
			   case 'P':
				comp=POLY;
				break;
			   case 'R':
				comp=RECT;
				break;
			   case 'T':
				comp=TEXT;
				break;
			   default:
				printf("SHOW: bad component designator: %s\n", word);
				done++;
				token_flush_EOL(lp);
				break;
			}
		    }

		    if (!done) {
			if ((strlen(word) >= 3)) {
			    if(sscanf(&word[2], "%d", &layer) != 1 || 
				    layer > MAX_LAYER) {
				printf("SHOW: bad layer number: %s\n", word);
				done++;
				token_flush_EOL(lp);
			    } 
			} else {
			    layer=0;
			}
		    }
		    if (!done) {
			show_set_visible(comp, layer, visible);
			show_set_modify(comp, layer, modifiable);
		    }
	        break;

	    case CMD:		/* command */
		token_unget(lp, token, word);
		done++;
		break;
	    case EOC:		/* end of command */
		done++;
		break;
	    case EOL:		/* newline or carriage return */
	        break;
	    case NUMBER: 	/* number */
	    case IDENT: 	/* identifier */
	    case COMMA:		/* comma */
	    case QUOTE: 	/* quoted string */
	    case END:		/* end of file */
	    default:
		printf("SHOW: expected OPT, got %s\n", tok2str(token), word);
		done++;
	    	break;
	}
    }
    rl_restoreprompt();
    return (0);
}

