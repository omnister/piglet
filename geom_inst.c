#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"
#include "opt_parse.h"

#define NORM   0	/* draw() modes */
#define RUBBER 1

static double x1, y1;

/* :Mmirror :Rrot :Xscale :Yyxratio :Zslant */

int add_inst(LEXER *lp, char *inst_name)
{
    enum {START,NUM1,COM1,NUM2,END} state = START;

    int done=0;
    int error=0;
    TOKEN token;
    OPTS opts;
    char word[BUFSIZE];
    char buf[BUFSIZE];
    double x2,y2;
    int debug=0;
    FILE *fp;
    LEXER *my_lp;

    DB_TAB *ed_rep;
    DB_TAB *save_rep;

    opt_set_defaults(&opts);

    rl_setprompt("ADD_INST> ");

    if (debug) printf("currep = %s\n", currep->name);
    if (debug) printf("adding inst %s\n", inst_name);

    /* don't destroy it if it's already in memory */
    if (debug) printf("calling db_lookup with %s\n", inst_name);
    if ((ed_rep = db_lookup(inst_name)) == NULL) {


	/* now check if on disk */
	snprintf(buf, MAXFILENAME, "./cells/%s.d", inst_name);
	if((fp = fopen(buf, "r")) == 0) {
	    printf("can't add a null instance: %s\n", inst_name);
	    token_flush_EOL(lp);
	    done++;
	    /* cannot find copy on disk */
	} else {
	    ed_rep = db_install(inst_name);  /* create blank stub */
	    /* read it in */
	    printf("reading %s from disk\n", buf);
	    my_lp = token_stream_open(fp, buf);
	    save_rep = currep;

	    /* xwin_display_set_state(D_OFF); */
	    currep = ed_rep;
	    parse(my_lp);
	    /* xwin_display_set_state(D_ON); */

	    token_stream_close(my_lp); 
	    currep = save_rep;
	}
    }

    if ((ed_rep = db_lookup(inst_name)) == 0) {
    	printf("ADD INST: instance not found: %s\n", inst_name );
	return(-1);
    }

    if (debug) printf("currep = %s\n", currep->name);

    if (strcmp(inst_name, currep->name) == 0) {
    	printf("ADD INST: can't recursively add instance\n" );
	return(-1);
    }

    while (!done) {
	token = token_look(lp, word);
	/* printf("got %s: %s\n", tok2str(token), word); */
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	    case START:		/* get option or first xy pair */
		if (token == OPT ) {
		    token_get(lp, word); 
		    if (opt_parse(word, "MRXYZ", &opts) == -1) {
			state = END;
		    } else {
			state = START;
		    }
		} else if (token == NUMBER) {
		    state = NUM1;
		} else if (token == EOL) {
		    token_get(lp, word); 	/* just eat it up */
		    state = START;
		} else if (token == EOC  || token == CMD) {
		    state = END; 
		} else {
		    token_err("INST", lp, "expected OPT or NUMBER", token);
		    state = END; 
		}
		break;
	    case NUM1:		/* get pair of xy coordinates */
		if (token == NUMBER) {
		    token_get(lp, word); 
		    sscanf(word, "%lf", &x1);	/* scan it in */
		    state = COM1;
		} else if (token == EOL) {
		    token_get(lp, word); /* just ignore it */
		} else if (token == EOC  || token == CMD) {
		    state = END; 
		} else {
		    token_err("INST", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case COM1:		
		if (token == EOL) {
		    token_get(lp, word); /* just ignore it */
		} else if (token == COMMA) {
		    token_get(lp, word);
		    state = NUM2;
		} else {
		    token_err("INST", lp, "expected COMMA", token);
		    state = END;	
		}
		break;
	    case NUM2:
		if (token == NUMBER) {
		    token_get(lp, word);
		    sscanf(word, "%lf", &y1);	/* scan it in */
		    
		    db_add_inst(currep, ed_rep, opt_copy(&opts), x1, y1);
		    need_redraw++;
		    state = START;
		} else if (token == EOL) {
		    token_get(lp, word);
		} else if (token == EOC  || token == CMD) {
		    state = END; 
		} else {
		    token_err("INST", lp, "expected NUMBER", token);
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
    return(1);
}

