#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"
#include "opt_parse.h"

DB_TAB dbtab; 
DB_DEFLIST dbdeflist;
DB_TEXT dbtext;

static double x1, y1;
char str[BUFSIZE];
void draw_text(); 

OPTS opts;


/* [:Mmirror] [:Rrot] [:Yyxratio] [:Zslant] [:Fsize] "string" xy EOC" */

int add_text(lp, layer, font_num)
LEXER *lp;
int *layer;
int font_num;
{
    enum {START,NUM1,COM1,NUM2,END} state = START;

    char word[BUFSIZE];
    int done=0;
    int error=0;
    TOKEN token;
    double x2,y2;
    int debug=0;
    int nargs=0;
    char *type;

    DB_TAB *ed_rep;

    opt_set_defaults( &opts );
    opts.font_num = font_num;

    str[0] = 0;

    while (!done) {

	if ((opts.font_num)%2) {
	    rl_setprompt("ADD_TEXT> ");
	    type="TEXT";
	} else {
	    rl_setprompt("ADD_NOTE> ");
	    type="NOTE";
	}

	token = token_look(lp, word);

	if (debug) printf("got %s: %s\n", tok2str(token), word); 

	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	    case START:		/* get option or first xy pair */
		if (token == OPT ) {
		    token_get(lp, word); 
		    if (nargs > 1) {
			rubber_clear_callback();
		    }
		    if (opt_parse(word, "MNRYZF", &opts) == -1) {
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
		    if (debug) printf("nargs = %d\n");
		    if (nargs > 1) {
		    	rubber_clear_callback();
		    }
		    token_get(lp,str); 
		    rubber_set_callback(draw_text);
		    state = START;
		} else if (token == EOL) {
		    token_get(lp, word); 	/* just eat it up */
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
		    token_get(lp, word);
		    sscanf(word, "%lf", &x1);	/* scan it in */
		    state = COM1;
		} else if (token == EOL) {
		    token_get(lp, word); 	/* just ignore it */
		} else if (token == EOC || token == CMD) {
		    printf(" cancelling ADD %s\n", type);
		    state = END; 
		} else {
		    token_err(type, lp, "expected NUMBER", token);
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
		    token_err(type, lp, "expected COMMA", token);
		    state = END;	
		}
		break;
	    case NUM2:
		if (token == NUMBER) {
		    token_get(lp, word);
		    sscanf(word, "%lf", &y1);	/* scan it in */

		    if (strlen(str) == 0) {
		    	printf("ADD %s: no string given\n", type);
			state = START;
		    } else {

			rubber_clear_callback();
			db_add_text(currep, *layer, opt_copy(&opts),
			    strsave(str), x1, y1);
			rubber_set_callback(draw_text);
			need_redraw++;
			state = START;
	            }
		} else if (token == EOL) {
		    token_get(lp, word); 	/* just ignore it */
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
    return(1);
}


void draw_text(x2, y2, count) 
double x2, y2;
int count; /* number of times called */
{
	static double xold, yold;
	int i;
	static BOUNDS bb;

	bb.init=0;
	int debug=0;

	/* DB_TAB dbtab;  */
	/* DB_DEFLIST dbdeflist; */
	/* DB_text dbtext; */


        dbtab.dbhead = &dbdeflist;
        dbtab.dbtail = &dbdeflist;
        dbtab.next = NULL;
	dbtab.name = "callback";

        dbdeflist.u.t = &dbtext;
        dbdeflist.type = TEXT;

        dbtext.layer=1;
        dbtext.opts=&opts;
	dbtext.text=str;
	
	if (debug) {printf("in draw_text\n");}

	if (count == 0) {		/* first call */
	    jump(); /* draw new shape */
	    dbtext.x=x2;
	    dbtext.y=y2;
	    do_text(&dbdeflist, &bb, D_RUBBER);

	} else if (count > 0) {		/* intermediate calls */
	    jump(); /* erase old shape */
	    dbtext.x=xold;
	    dbtext.y=yold;
	    do_text(&dbdeflist, &bb, D_RUBBER);
	    jump(); /* draw new shape */
	    dbtext.x=x2;
	    dbtext.y=y2;
	    do_text(&dbdeflist, &bb, D_RUBBER);
	} else {			/* last call, cleanup */
	    jump(); /* erase old shape */
	    dbtext.x=xold;
	    dbtext.y=yold;
	    do_text(&dbdeflist, &bb, D_RUBBER);
	}

	/* save old values */
	xold=x2;
	yold=y2;
	jump();
}

