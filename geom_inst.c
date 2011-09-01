#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"
#include "opt_parse.h"
#include "rlgetc.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

/* :Mmirror :Rrot :Xscale :Yyxratio :Zslant */


static double bb_xmin, bb_ymin, bb_xmax, bb_ymax;
void draw_inst_bb();

int loadrep(char *inst_name)
{
    char buf[BUFSIZE];
    FILE *fp;
    extern XFORM *global_transform;
    XFORM *save_transform;

    DB_TAB *ed_rep;
    int retval=1;
    char *save_rep;

    BOUNDS bb;

    if ((ed_rep = db_lookup(inst_name)) == NULL) {	/* not in memory */

	snprintf(buf, MAXFILENAME, "./cells/%s.d", inst_name);
	if((fp = fopen(buf, "r")) == 0) { 		/* cannot find copy on disk */	
	    retval=0;
	} else { 					/* found it on disk, read it in */	

	    if (currep != NULL) {
		save_rep=strsave(currep->name);
	    } else {
		save_rep=NULL;
	    }

	    currep =  db_install(inst_name);  		/* create blank stub */
	    readin(buf, 1, EDI, NULL);
	    currep->modified = 0;

    	    bb.init=0;
	    save_transform = global_transform;
	    db_render(currep, 0, &bb, D_READIN); 	/* set boundbox, etc */
	    global_transform = save_transform;

	    if (save_rep != NULL) {
		currep=db_lookup(save_rep);
		free(save_rep);
	    } else {
		currep=NULL;
	    }
	}
    }
    return(retval);
}

int add_inst(LEXER *lp, char *inst_name)
{
    enum {START,NUM1,NUM2,NUM3,NUM4,END} state = START;

    double x1, y1;	// pick value
    double x2, y2;	// pick value
    double x3, y3;	// pick value
    double xold, yold;	// previous pick value to suppress double pics
    int numpicks=0;	// number of picks
    int done=0;
    TOKEN token;
    OPTS opts;
    char *word;
    int debug=0;
    struct db_inst *ip;

    DB_TAB *ed_rep;

    XFORM *xp;

    double xx, yy;

    opt_set_defaults(&opts);

    // rl_saveprompt();
    rl_setprompt("ADD_INST> ");

    if (debug) printf("currep = %s\n", currep->name);
    if (debug) printf("adding inst %s\n", inst_name);

    /* don't destroy it if it's already in memory */
    if (debug) printf("calling db_lookup with %s\n", inst_name);

    if (loadrep(inst_name) == 0) {
	printf("warning: you can't add a null instance: %s\n", inst_name);
	token_flush_EOL(lp);
	done++;	
    }

    if ((ed_rep = db_lookup(inst_name)) == 0) {
    	printf("ADD INST: instance not found: %s\n", inst_name );
	return(-1);
    }

    bb_xmin=ed_rep->minx;
    bb_xmax=ed_rep->maxx;
    bb_ymin=ed_rep->miny;
    bb_ymax=ed_rep->maxy;

    if (debug) printf("currep = %s\n", currep->name);

    if (strcmp(currep->name, inst_name) == 0 || db_contains(inst_name, currep->name)) {
    	printf("ADD INST: Sorry, to do that would cause a recursive definition\n" );
	return(-1);
    }

    while (!done) {
	token = token_look(lp, &word);
	if (debug) printf("got %s: %s state: %d\n", tok2str(token), word, state);
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	    case START:		/* get option or first xy pair */
	        db_checkpoint(lp);
		rubber_set_callback(draw_inst_bb);
		if (token == OPT ) {
		    token_get(lp, &word); 
		    if (opt_parse(word, INST_OPTS, &opts) == -1) {
			state = END;
		    } else {
			/* an option may have scaled the bounding box */
			/* clear callback, recompute and then restart */

			rubber_clear_callback(draw_inst_bb);

			xp = matrix_from_opts(&opts);
			bb_xmin = bb_xmax = bb_ymin = bb_ymax = 0.0;

			xx = ed_rep->minx;
			yy = ed_rep->miny;
			xform_point(xp, &xx, &yy); 

			if (xx < bb_xmin) bb_xmin = xx;
			if (yy < bb_ymin) bb_ymin = yy;
			if (xx > bb_xmax) bb_xmax = xx;
			if (yy > bb_ymax) bb_ymax = yy;

			xx = ed_rep->maxx;
			yy = ed_rep->maxy;
			xform_point(xp, &xx, &yy); 

			if (xx < bb_xmin) bb_xmin = xx;
			if (yy < bb_ymin) bb_ymin = yy;
			if (xx > bb_xmax) bb_xmax = xx;
			if (yy > bb_ymax) bb_ymax = yy;

			xx = ed_rep->maxx;
			yy = ed_rep->miny;
			xform_point(xp, &xx, &yy); 

			if (xx < bb_xmin) bb_xmin = xx;
			if (yy < bb_ymin) bb_ymin = yy;
			if (xx > bb_xmax) bb_xmax = xx;
			if (yy > bb_ymax) bb_ymax = yy;

			xx = ed_rep->minx;
			yy = ed_rep->maxy;
			xform_point(xp, &xx, &yy); 

			if (xx < bb_xmin) bb_xmin = xx;
			if (yy < bb_ymin) bb_ymin = yy;
			if (xx > bb_xmax) bb_xmax = xx;
			if (yy > bb_ymax) bb_ymax = yy;

			free(xp);

			state = START;
			rubber_set_callback(draw_inst_bb);
		    	if (opts.stepflag) {
			    rl_setprompt("ARRAY ORIGIN> ");
			    rubber_clear_callback();
			}
		    }
		} else if (token == NUMBER) {
		    if (opts.stepflag) {
			state = NUM2;	
		    } else {
			state = NUM1;	
		    }
		} else if (token == EOL) {
		    token_get(lp, &word); 	/* just eat it up */
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
		    if (getnum(lp, "INST", &x1, &y1)) {
			// supress double clicks
			if (debug) printf("%d %g %g %g %g\n", numpicks, x1, y1, xold, yold);
			if (numpicks==0 || ((xold != x1) || (yold != y1))) {  
			    db_add_inst(currep, ed_rep, opt_copy(&opts), x1, y1);
			    rubber_clear_callback();
			    need_redraw++;
			    rubber_set_callback(draw_inst_bb);
			}
		        numpicks++; xold=x1; yold=y1;
			state = START;
	            } else {
			state = END;
		    }
		} else if (token == EOL) {
		    token_get(lp, &word);
		} else if (token == EOC  || token == CMD) {
		    state = END; 
		} else {
		    token_err("INST", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case NUM2:		/* get column end coordinate */
		rubber_clear_callback();
		if (token == NUMBER) {
		    if (getnum(lp, "INST", &x1, &y1)) {
    			rl_setprompt("COLUMN EXTENT> ");
			state = NUM3;
	            } else {
			state = END;
		    }
		} else if (token == EOL) {
		    token_get(lp, &word);
		} else if (token == EOC  || token == CMD) {
		    state = END; 
		} else {
		    token_err("INST", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case NUM3:		/* get column end coordinate */
		if (token == NUMBER) {
		    if (getnum(lp, "INST", &x3, &y3)) {
    			rl_setprompt("ROW EXTENT> ");
			state = NUM4;
	            } else {
			state = END;
		    }
		} else if (token == EOL) {
		    token_get(lp, &word);
		} else if (token == EOC  || token == CMD) {
		    state = END; 
		} else {
		    token_err("INST", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case NUM4:		/* get column end coordinate */
		if (token == NUMBER) {
		    if (getnum(lp, "INST", &x2, &y2)) {
		        if (debug) printf("%g %g %g %g %g %g\n", x1, y1, x2, y2, x3, y3);
			ip = db_add_inst(currep, ed_rep, opt_copy(&opts), x1, y1);
			ip->colx = x3;
			ip->coly = y3;
			ip->rowx = x2;
			ip->rowy = y2;
			rubber_clear_callback();
			need_redraw++;
			state = END;
	            } else {
			state = END;
		    }
		} else if (token == EOL) {
		    token_get(lp, &word);
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
    rubber_clear_callback();
    rl_restoreprompt();
    return(1);
}

void draw_inst_bb(double x, double y, int count) 
{
	static double xold, yold;
	BOUNDS bb;
	bb.init=0;

	if (count == 0) {		/* first call */
	    db_drawbounds(bb_xmin+x, bb_ymin+y, bb_xmax+x, bb_ymax+y, D_RUBBER);
	} else if (count > 0) {		/* intermediate calls */
	    db_drawbounds(bb_xmin+xold, bb_ymin+yold, bb_xmax+xold, bb_ymax+yold, D_RUBBER);
	    db_drawbounds(bb_xmin+x, bb_ymin+y, bb_xmax+x, bb_ymax+y, D_RUBBER);
	} else {			/* last call, cleanup */
	    db_drawbounds(bb_xmin+xold, bb_ymin+yold, bb_xmax+xold, bb_ymax+yold, D_RUBBER);
	}

	/* save old values */
	xold=x;
	yold=y;
	jump(&bb, D_RUBBER);
}

