#include <stdio.h>
#include <string.h>		/* for strchr() */
#include <ctype.h>		/* for toupper */
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <readline/readline.h> 	/* for command line editing */
#include <readline/history.h>  

#include "rlgetc.h"
#include "db.h"
#include "token.h"
#include "xwin.h" 	
#include "ev.h"

extern void do_win();		/* found in com_window */
extern int readin();
extern int com_show();
extern int com_echo();

int com_edit(LEXER *lp, char *arg)		/* begin edit of an old or new device */
{
    TOKEN token;
    int done=0;
    char *word;
    char *p;
    char name[128];
    char buf[128];
    char buf2[128];
    int debug=0;
    int nfiles=0;
    DB_TAB *new_rep;  
    DB_TAB *old_rep = NULL;  
    double x1, y1;
    DB_DEFLIST *p_best;
    // double xmin, ymin, xmax, ymax;
    int code;

    enum {START,NUM1,DOIT,END} state = START;

/* an edi parse loop that will allow picking of instances for EIP */

/* current thinking is to add flag bits in the db_deflist so that the picking */
/* is managed in the rendering routine in-situ.  Setting and clearing the flags */
/* may also be able to manage EIP'ing in and out in a stack-like fashion */
/* need to make EDI <coord> do a pick on instances only to select EIP rep */

    if (debug) printf("    com_edit <%s>\n", arg); 

    switch (lp->mode) {
        case PRO:
	    printf("PRO: can't EDIT in PROCESS mode\n");
            token_flush_EOL(lp);
	    return(1);
	case SEA:
	    printf("SEA: can't EDIT in SEARCH mode\n");
            token_flush_EOL(lp);
	    return(1);
	case MAC:
	    printf("MAC: can't EDIT in MACRO mode\n");
            token_flush_EOL(lp);
	    return(1);
	default:
	    break;
    }

    name[0]=0;

    while(!done) {
	token = token_look(lp,&word);
	if (debug) printf("got %s: %s\n", tok2str(token), word);
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	case START:		/* get cellname or xy pick point */
	    if (debug) printf("in START\n");
	    if (token == OPT ) {
		token_get(lp,&word); /* ignore for now */
		state = START;
	    } else if (token == NUMBER) {
		state = NUM1;
	    } else if (token == EOL) {
		token_get(lp,&word); 	/* just eat it up */
		state = START;
	    } else if (token == EOC || token == CMD) {
		printf("EDIT: requires a name or a selection point\n");
		state = END;
	    } else if (token == QUOTE) {	// RCW
		token_get(lp,&word);
		if (nfiles == 0) {
		    strncpy(name, word, 128);
		    nfiles++;
		} else {
		    printf("EDIT: wrong number of args\n");
		    return(-1);
		}
		state = DOIT;
	    } else if (token == IDENT) {
		token_get(lp,&word);
		if (nfiles == 0) {
		    strncpy(name, word, 128);
		    nfiles++;
		} else {
		    printf("EDIT: wrong number of args\n");
		    return(-1);
		}
		state = DOIT;
	    } else {
		token_err("EDIT", lp, "expected DESC or NUMBER", token);
		state = END;	/* error */
	    }
	    break;
	case NUM1:
	    if (debug) printf("in NUM2\n");
	    if (token == NUMBER ) {
		if (getnum(lp, "EDIT", &x1, &y1)) {
		    if ((p_best=db_ident(currep, x1,y1,1, 0, INST, 0)) != NULL) {
			db_notate(p_best);	    /* print out id information */
			db_highlight(p_best);
			// xmin=p_best->xmin;
			// xmax=p_best->xmax;
			// ymin=p_best->ymin;
			// ymax=p_best->ymax;
			printf("%s\n", p_best->u.i->name);
			strncpy(name, p_best->u.i->name, 128);
			state = DOIT; 
		    } else {
			printf("nothing here to EDIT... try SHO command?\n");
			state = START;
		    }
	        } else {
		    state = END;
		}
	    } else if (token == EOL) {
		token_get(lp,&word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("EDIT: cancelling EDIT\n");
	        state = END;
	    } else {
		token_err("EDIT", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case DOIT:
	    if (lp->mode == MAIN || currep == NULL  || lp->mode == EDI) {
		if (lp->mode == EDI) {	
		    if (currep != NULL) {
			/* currep->being_edited = 0;  */

			old_rep = currep;

			/* the old policy was to just dump it if it was */
			/* not modified  Now we leave being_edited */
			/* alone and use it to manage an edit stack.  */
			/* When we EXIT a cell with being_edited = N, */
			/* we then re-edit the next cell in the db */
			/* with the highest being_edited < N. We have to */
			/* do a search for the biggest one because of */
			/* the possibility that a cell could be */
			/* purged along the way...  */
		    }
		}

		/* check for a name provided */
		if (strlen(name) == 0) {
		    printf("    must provide a name: EDIT <name>\n");
		    state = END;
		} 

		if (debug) printf("got %s\n", name); 

		lp->mode = EDI;

		if ((new_rep=db_lookup(name)) != NULL && new_rep->modified && strncmp(name,"NONAME_",7)) { 
		    if (ask(lp, "modified cell already in memory, read in a new copy?")) {
		        db_unlink_cell(new_rep);
		    }
		}

		// FIXME:  should remember file mtime when originally reading in a file
		// then, any subsequent reads should ask to reload memory if the file
		// is newer than the remembered read-in date.  This allows easy textual
		// modification of a cell by exiting, editing and re-editing.
	    
		if ((new_rep=db_lookup(name)) == NULL) { /* Not already in memory */

		    currep = db_install(name);  	 /* create blank stub */
		    show_init(currep);
		    need_redraw++;

		    /* 
		       FIXME:
		       should decompose this into routines
		       FILE *search(char *cellname);
		       readin(FILE *fp); 
		    */

		    snprintf(buf, MAXFILENAME, "./cells/%s.d", name);
		    snprintf(buf2, MAXFILENAME, "./cells/#%s.d", name);

		    code = 0;
		    if (access(buf, R_OK) == 0) code=1;
		    if (access(buf2, R_OK) == 0) code+=2;

		    struct stat sb1;
		    struct stat sb2;

		    switch (code) {
		        case 0:	// no files exist
			    break;
			case 1:	// only the proper .d file exists
			    break;
			case 2:	// only the crash file exists
			    printf("couldn't find %s, reading %s instead\n", buf, buf2);
			    strcpy(buf, buf2);
			    break;
			case 3: // both files exist
			    stat(buf,  &sb1);
			    stat(buf2, &sb2);
			    if (sb2.st_mtime > sb1.st_mtime) {
				printf("\tAn autosave file was found that is newer than the device file.\n");
				printf("\tdevice file:    %s, %d bytes, %s", buf, 
					(int) sb1.st_size, ctime(&sb1.st_mtime));
				printf("\tautosave file: %s, %d bytes, %s", buf2, 
					(int) sb2.st_size, ctime(&sb2.st_mtime));
			    }
		            if (ask(lp, "read autosave file instead of .d file?")) {
				strcpy(buf, buf2);
		            }
			    break;
			default:
			    break;
		    }

		    if (readin(buf,1,EDI) == 0) {   /* cannot find copy on disk */
			if (debug) printf("calling dowin 1\n");
			do_win(lp, 4, -150.0, -150.0, 150.0, 150.0, 1.0);
			if (old_rep == NULL ) {
			    currep->being_edited = 1;
			} else {
			    /* push the stack */
			    currep->being_edited = old_rep->being_edited+1;
			}
		    } else { 	/* found copy and read it in */
			if (debug) printf("calling dowin 2: %g %g %g %g\n", 
				currep->vp_xmin, currep->vp_ymin, 
				currep->vp_xmax, currep->vp_ymax);

			do_win(lp, 4, currep->vp_xmin, currep->vp_ymin, 
			        currep->vp_xmax, currep->vp_ymax, 1.05); 
			currep->modified = 0;

			if (old_rep == NULL ) {
			    currep->being_edited = 1;
			} else {
			    /* push the stack */
			    currep->being_edited = old_rep->being_edited+1;
			}

			show_init(currep);
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
	        state = END;
	    } else if (lp->mode == EDI) {
		printf("    must SAVE current device before new EDIT\n");
		state = END;
	    } else {
		printf("    must EXIT before entering EDIT subsystem\n");
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
	    if ((p=EVget("PIG_SHOW_DEFAULT")) != NULL) {
		sprintf(buf, "SHOW %s;", p);
		// printf("Evaluating $PIG_SHOW_DEFAULT: \"SHOW %s\"\n", p);
		rl_ungets(lp, buf);
	    }
	    done++;
	    break;
	}
    }

    return(1);
}

