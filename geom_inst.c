#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"
#include "opt_parse.h"

#define NORM   0	/* draw() modes */
#define RUBBER 1

static double x1, y1;

/* :Mmirror :Rrot :Xscale :Yyxratio :Z */

int add_inst(inst_name)
char *inst_name;
{
    enum {START,NUM1,COM1,NUM2,END} state = START;

    int done=0;
    int error=0;
    TOKEN token;
    OPTS *opts;
    char word[BUFSIZE];
    double x2,y2;
    int debug=0;

    DB_TAB *ed_rep;

    rl_setprompt("ADD_INST> ");

    if ((ed_rep = db_lookup(inst_name)) == 0) {
    	printf("ADD INST: instance not found: %s\n", inst_name );
	return(-1);
    }

    if (strcmp(inst_name, currep->name) == 0) {
    	printf("ADD INST: can't recursively add instance\n" );
	return(-1);
    }

    opts = opt_create();

    while (!done) {
	token = token_look(word);
	/* printf("got %s: %s\n", tok2str(token), word); */
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	    case START:		/* get option or first xy pair */
		if (token == OPT ) {
		    token_get(word); 
		    if (opt_parse(word, "MRXYZ", opts) == -1) {
			state = END;
		    } else {
			state = START;
		    }
		} else if (token == NUMBER) {
		    state = NUM1;
		} else if (token == EOL) {
		    token_get(word); 	/* just eat it up */
		    state = START;
		} else {
		    state = END;	/* error */
		}
		break;
	    case NUM1:		/* get pair of xy coordinates */
		if (token == NUMBER) {
		    token_get(word);
		    sscanf(word, "%lf", &x1);	/* scan it in */
		    state = COM1;
		} else if (token == EOL) {
		    token_get(word); 	/* just ignore it */
		} else if (token == EOC) {
		    state = END; 
		}
		break;
	    case COM1:		
		if (token == EOL) {
		    token_get(word); /* just ignore it */
		} else if (token == COMMA) {
		    token_get(word);
		    state = NUM2;
		} else {
		    printf("    1: expected a comma!\n");
		    state = END;	
		}
		break;
	    case NUM2:
		if (token == NUMBER) {
		    token_get(word);
		    sscanf(word, "%lf", &y1);	/* scan it in */
		    db_add_inst(currep, ed_rep, opts, x1, y1);
		    need_redraw++;
		    state = START;
		} else if (token == EOL) {
		    token_get(word); 	/* just ignore it */
		} else if (token == EOC) {
		    state = END; 
		}
		break;
	    case END:
	    default:
		if (token == EOC || token == CMD) {
			;
		} else {
		    token_flush();
		}
		done++;
		break;
	}
    }
    return(1);
}

