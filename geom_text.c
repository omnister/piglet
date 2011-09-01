#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"
#include "opt_parse.h"
#include "rlgetc.h"
#include <string.h>

DB_TAB dbtab; 
DB_DEFLIST dbdeflist;
DB_TEXT dbtext;

static double x1, y1;
char str[BUFSIZE];
void draw_text(); 

OPTS opts;

int add_annotation();

/* [:Mmirror] [:Rrot] [:Yyxratio] [:Zslant] [:Fsize] "string" xy EOC" */

void add_note(LEXER *lp, int *layer) {
    add_annotation(lp, layer, 0);
}

void add_text(LEXER *lp, int *layer) {
    add_annotation(lp, layer, 1);
}

int add_annotation(lp, layer, mode)
LEXER *lp;
int *layer;
int mode;
{
    enum {START,NUM1,END} state = START;

    char *word;
    int done=0;
    TOKEN token;
    int debug=0;
    int nargs=0;
    char *type;
    int numadd = 0;
    double xold, yold;

    opt_set_defaults( &opts );
    opts.font_size = db_get_font_size(); 	/* get default from FSIze command */
    opts.slant = db_get_text_slant(); 		/* get default from TSLant command */
    opts.font_num = mode;			/* default note font=0, text=1 */

    str[0] = '\0';

    while (!done) {

	rl_saveprompt();

	if (mode) {
	    rl_setprompt("ADD_TEXT> ");
	    type="TEXT";
	} else {
	    rl_setprompt("ADD_NOTE> ");
	    type="NOTE";
	}

	token = token_look(lp, &word);

	if (debug) printf("got %s: %s\n", tok2str(token), word); 

	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	    case START:		/* get option or first xy pair */
	        db_checkpoint(lp);
		if (token == OPT ) {
		    token_get(lp, &word); 
		    if (nargs > 1) {
			rubber_clear_callback();
		    }
		    if (mode && opt_parse(word, TEXT_OPTS, &opts) == -1) {
			state = END;
		    } else if (!mode && opt_parse(word, NOTE_OPTS, &opts) == -1) {
			state = END;
		    } else {
			state = START;
		    }
		    if (nargs > 1) {
			rubber_set_callback(draw_text);
			nargs++;
		    }
		} else if (token == NUMBER) {
		    state = NUM1;
		} else if (token == QUOTE) {
		    nargs++;
		    if (debug) printf("nargs = %d\n", nargs);
		    if (nargs > 1) {
		    	rubber_clear_callback();
		    }
		    token_get(lp,&word); 
		    strncpy(str,word,BUFSIZE);
		    rubber_set_callback(draw_text);
		    state = START;
		} else if (token == EOL) {
		    token_get(lp, &word); 	/* just eat it up */
		    state = START;
		} else if (token == EOC || token == CMD) {
		    state = END;	/* error */
		} else {
		    token_err(type, lp, "expected OPT or NUMBER", token);
		    state = END; 
		}
		break;
	    case NUM1:		/* get pair of xy coordinates */
		if (token == NUMBER) {
		    if (getnum(lp, "TEXT", &x1, &y1)) {
			if (strlen(str) == 0) {
			    printf("ADD %s: no string given\n", type);
			    state = START;
			} else {
			    if (numadd && xold == x1 && yold == y1) {
				printf("   suppressing double click\n");
			    } else {
				rubber_clear_callback();
				if (mode) {
				    db_add_text(currep, *layer, opt_copy(&opts),
					strsave(str), x1, y1);
				} else {
				    db_add_note(currep, *layer, opt_copy(&opts),
					strsave(str), x1, y1);
				}
				rubber_set_callback(draw_text);
				need_redraw++;
			    }
			    xold=x1; yold=y1;
			    numadd++;
			    state = START;
			}
		    } else {
		  	state = END;
		    }
		} else if (token == EOL) {
		    token_get(lp, &word); 	/* just ignore it */
		} else if (token == EOC || token == CMD) {
		    printf(" cancelling ADD %s\n", type);
		    state = END; 
		} else {
		    token_err(type, lp, "expected NUMBER", token);
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

		/* FIXME, maybe I need a rubber_cleanup() which does */
		/* a clear_callback only if needed?  Bailing out from*/
		/* a bad option leaves a "ghost" on the screen */

		rubber_clear_callback();
		done++;
		break;
	}
    }
    rl_restoreprompt();
    return(1);
}


void draw_text(double x2, double y2, int count) 
{
	static double xold, yold;
	int debug=0;
	static BOUNDS bb;

	bb.init=0;

	/* DB_TAB dbtab;  */
	/* DB_DEFLIST dbdeflist; */
	/* DB_text dbtext; */


        dbtab.dbhead = &dbdeflist;
        dbtab.next = NULL;
	dbtab.name = "callback";

        dbdeflist.u.t = &dbtext;
        dbdeflist.type = TEXT;

        dbtext.layer=1;
        dbtext.opts=&opts;
	dbtext.text=str;
	
	if (debug) {printf("in draw_text\n");}

	if (count == 0) {		/* first call */
	    jump(&bb, D_RUBBER); /* draw new shape */
	    dbtext.x=x2;
	    dbtext.y=y2;
	    do_text(&dbdeflist, &bb, D_RUBBER);

	} else if (count > 0) {		/* intermediate calls */
	    jump(&bb, D_RUBBER); /* erase old shape */
	    dbtext.x=xold;
	    dbtext.y=yold;
	    do_text(&dbdeflist, &bb, D_RUBBER);
	    jump(&bb, D_RUBBER); /* draw new shape */
	    dbtext.x=x2;
	    dbtext.y=y2;
	    do_text(&dbdeflist, &bb, D_RUBBER);
	} else {			/* last call, cleanup */
	    jump(&bb, D_RUBBER); /* erase old shape */
	    dbtext.x=xold;
	    dbtext.y=yold;
	    do_text(&dbdeflist, &bb, D_RUBBER);
	}

	/* save old values */
	xold=x2;
	yold=y2;
	jump(&bb, D_RUBBER);
}

