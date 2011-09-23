#include <sys/stat.h>	/* for mkdir() */
#include <sys/types.h>	/* for mkdir() */
#include <stdio.h>
#include <math.h>
#include <string.h>	/* for strnlen... */
#include <ctype.h> 	/* for toupper() */
#include <unistd.h>     /* for access() */
#include <time.h>

#include "db.h"		/* hierarchical database routines */
#include "eprintf.h"	/* error reporting functions */
#include "rlgetc.h"
#include "token.h"
#include "xwin.h"       /* for snapxy() */
#include "readfont.h"	/* for writestring() */
#include "rubber.h"
#include "rlgetc.h"
#include "ev.h"

#define EPS 1e-6

#define CELL 0		/* modes for db_def_print() */
#define ARCHIVE 1

#define BB 2		/* draw bounding box and don't do xform */


char *PIG_PATH="./cells:.:./.pigrc:~/.pigrc:/usr/local/lib/piglet:/usr/lib/piglet";


/* master symbol table pointers */
static DB_TAB *HEAD = 0;

/* global drawing variables */
XFORM unity_transform;
XFORM *global_transform = &unity_transform;  /* global xform matrix */

STACK *stack = NULL;

void db_free_component(); 		/* recycle memory for component */
int getbits();
int db_def_arch_recurse();
int db_def_files_recurse(); 
void db_contains_body(); 
void db_def_print();

/********************************************************/

DB_TAB * db_edit_pop(int level)
{

    /* simply run through the db and return a pointer */
    /* to the rep with the highest <being_edited> variable */
    /* that is less than <level>.  Print an error if we  */
    /* find a <being_edited> equal or higher than <level> */
    /* The editor logic should ensure that this never can */
    /* happen... */

    int debug=0;

    DB_TAB *sp;
    DB_TAB *sp_best;
    int maxlevel=0;

    if (debug) printf("db_edit_pop: called with level %d\n", level);

    if (HEAD != (DB_TAB *)0) {
	for (sp=HEAD; sp!=(DB_TAB *)0; sp=sp->next) {
	    if (sp->being_edited > 0) {
		if (debug) printf("db_edit_pop: %s is level %d\n",
			sp->name, sp->being_edited);
		if (sp->being_edited > maxlevel) {
		    maxlevel = sp->being_edited;
		    sp_best = sp;
		}
	    }
	}
    }
    if (maxlevel > level) {
	printf("fatal error in db_edit_pop()\n"); 
	printf("db_edit_pop(): returning pointer to %s %d %d\n",
	    sp_best->name, maxlevel, level); 
	return(NULL);
    } else if (maxlevel > 0) {
	if (debug) printf("db_edit_pop(): returning pointer to %s\n",
	    sp_best->name); 
        return (sp_best);	
    } else {
	if (debug) printf("db_edit_pop(): returning NULL\n"); 
	return (NULL);
    }
}

int db_exists(char *name) { 		/* return 0 if not in memory or on disk */

    char buf[MAXFILENAME];

    snprintf(buf, MAXFILENAME, "./cells/%s.d", name );


    if (((access(buf,F_OK)) == 0)  || (db_lookup(name) != NULL)) {
	printf("checking access on %s = 1\n", buf);
        return 1;
    } else {
	printf("checking access on %s = 0\n", buf);
        return 0;
    }
}


DB_TAB *db_lookup(char *name)           /* find name in db */
{
    DB_TAB *sp;
    int debug=0;

    if (debug) printf("calling db_lookup with %s: ", name);
    if (name==NULL) {
       return(NULL);
    } 
    if (HEAD != (DB_TAB *)0) {
	for (sp=HEAD; sp!=(DB_TAB *)0; sp=sp->next) {
	    if (strcmp(sp->name, name)==0) {
		if (debug) printf("found!\n");
		return(sp);
	    }
	}
    }
    if (debug) printf("not found!\n");
    return(NULL);              	/* 0 ==> not found */
} 

DB_TAB *new_dbtab() {	/* return a new dbtab set to default values */
    DB_TAB *sp;
    char *p;
    int d;

    sp = (DB_TAB *) emalloc(sizeof(struct db_tab));
    sp->name = NULL;

    sp->next   = (DB_TAB *) 0; 
    sp->prev   = (DB_TAB *) 0; 
    sp->dbhead = (struct db_deflist *) 0; 
    sp->undo = (STACK *) 0; 
    sp->redo = (STACK *) 0; 
    sp->chksum = 0;
    sp->background = (char *) 0;
    sp->prims = 0;

    sp->vp_xmin = -100.0; 
    sp->vp_ymin = -100.0; 
    sp->vp_xmax = 100.0; 
    sp->vp_ymax = 100.0; 

    sp->old_xmin = -100.0; 
    sp->old_ymin = -100.0; 
    sp->old_xmax = 100.0; 
    sp->old_ymax = 100.0; 

    sp->minx = -100.0; 
    sp->miny = -100.0; 
    sp->maxx = 100.0; 
    sp->maxy = 100.0; 

    sp->grid_xd = 10.0;
    sp->grid_yd = 10.0;
    sp->grid_xs = 1.0;
    sp->grid_ys = 1.0;
    sp->grid_xo = 0.0;
    sp->grid_yo = 0.0;

    /* FIXME: good place for an ENV variables to let */
    /* user set the default grid color and step sizes */

    if ((p=EVget("PIG_GRID")) != NULL) {
    	if ((d=sscanf(p,"%lg %lg %lg %lg %lg %lg", 
	    &sp->grid_xd, &sp->grid_yd, &sp->grid_xs,
    	    &sp->grid_ys, &sp->grid_xo, &sp->grid_yo)) <= 0) {
		printf("bad PIG_GRID=\"%s\"\n",p);
	} 
    } 

    sp->grid_color = 3;		
    if( (p=EVget("PIG_GRID_COLOR")) != NULL) {
        sp->grid_color=atoi(p);
    }

    sp->display_state=G_ON;
    sp->logical_level=1;
    sp->lock_angle=0.0;
    sp->modified = 0;
    sp->being_edited = 0;
    sp->is_tmp_rep = 0;
    sp->flag = 0;
    sp->seqflag = 0;

    return(sp);
}

DB_TAB *db_install(char *s)		/* install s in db */
{
    DB_TAB *sp;

    /* this is written to ensure that a db_print is */
    /* not in reversed order...  New structures are */
    /* added at the end of the list using HEAD->prev */

    sp = new_dbtab();
    sp->name = emalloc(strlen(s)+1); /* +1 for '\0' */
    strcpy(sp->name, s);

    if (HEAD ==(DB_TAB *) 0) {	/* first element */
	HEAD = sp;
	sp->prev = sp;
    } else {
	sp->prev = HEAD->prev;
	HEAD->prev->next = sp;
	HEAD->prev = sp;
    }

    return (sp);
}

int ask(LEXER *lp, char *s) /* ask a y/n question */
{
    TOKEN token;
    char *word;
    char buf[128];
    int done=0;

    rl_saveprompt();

    sprintf(buf,"%s: (y/n)? ", s);
    rl_setprompt(buf);

    while(!done && (token=token_get(lp, &word)) != EOF) {
	switch(token) {
	    case IDENT: 	/* identifier */
		if (strncasecmp(word, "Y", 1) == 0) {
    		    rl_restoreprompt();
		    return(1);
		} else if (strncasecmp(word, "N", 1) == 0) {
    		    rl_restoreprompt();
		    return(0);
		} else {
	    	     printf("please answer by typing \"yes\" or \"no\": ");
		} 
		break;
	    case EOL:
	    case EOC:
	    	break; 
	    default:		/* command */
		 printf("please answer by typing \"yes\" or \"no\": ");
		 break;
        }
    }
    return(-1);
}

void db_list_db() 			/* print names of all cells in memory */
{
    DB_TAB *sp;
    int i=0;

    if (HEAD != (DB_TAB *)0) {
	for (sp=HEAD; sp!=(DB_TAB *)0; sp=sp->next) {
	    printf("    %d: %s ", i++, sp->name);
	    if (sp->modified) {
		printf("\t(modified)");
	    }
	    printf("\n");
	}
    }
}

//          ++++++++++++++++++++++++
//          |                      v
//   H-->[ f ]-->[ n ]-->[ s ]-->[ l ]--->/
//        ^       | ^      |^      |
//        +-------- +------ +-------

void db_fsck(DB_DEFLIST *dp) {		/* do a logical scan of a deflist */
    DB_DEFLIST *p;
    DB_DEFLIST *pold;
    DB_DEFLIST *next;
    DB_DEFLIST *nextold;
    DB_DEFLIST *prev;
    // DB_DEFLIST *prevold;
    DB_DEFLIST *final;
    int count=0;

    final = dp->prev; 	/* prev of first should point at last */
    nextold=NULL;
    // prevold=NULL;

    for (p=dp; p!=(struct db_deflist *)0; p=p->next) {
	next = p->next;
	prev = p->prev;
       
        if (count) {
	   if (p != nextold) printf("err1\n"); 
	   if (prev != pold) printf("err2\n"); 
	}

	if (next == NULL && p != final) {
	    printf("db_fsck: bad tail pointer\n");
	}

        printf("D:%ld T:%ld, P: %ld, N:%ld\n",
	(long int) p, (long int) p->type, (long int) p->prev, (long int) p->next);

	count++;
	pold=p;
	nextold=next;
	// prevold=prev;
    }
}

void db_free(DB_DEFLIST *dp) {		/* free an entire definition list */
    DB_DEFLIST *p;
    DB_DEFLIST *pn;
    for (p=dp; p!=(struct db_deflist *)0; p=pn) {
	pn=p->next;
	db_free_component(p);
    }
}

void db_unlink_cell(DB_TAB *sp) {

    DB_DEFLIST *p;
    DB_DEFLIST *pn;

    if (currep == sp) {
	currep = NULL;
	need_redraw++;
    }

    if (HEAD==sp) {				/* first in chain */
	HEAD=sp->next;
    } else if (HEAD!=NULL && HEAD->prev==sp) {	/* last in chain */
	HEAD->prev=sp->prev;
    }

    if (sp->prev != (DB_TAB *) 0) {
	sp->prev->next = sp->next;
    }
    if (sp->next != (DB_TAB *) 0) {
	sp->next->prev = sp->prev;
    }

    for (p=sp->dbhead; p!=(struct db_deflist *)0; p=pn) {
	pn=p->next;
	db_free_component(p);
    }

    /* free sp->undo and sp->redo stacks to prevent memory leak */

    while ((p = (DB_DEFLIST *) stack_pop(&(sp->undo)))!=NULL) {              /* clear out undo stack */
	 db_free(p);
    }
    while ((p = (DB_DEFLIST *) stack_pop(&(sp->redo)))!=NULL) {              /* clear out redo stack */
	 db_free(p);
    }

    free(sp->name);
    free(sp->background);
    free(sp);
}

void db_purge(LEXER *lp, char *s)			/* remove all definitions for s */
{
    DB_TAB *dp;
    DB_TAB *sp;
    DB_DEFLIST *p;
    int debug=0;
    char buf[MAXFILENAME];
    char buf2[MAXFILENAME];
    int flag=0;

    if ((sp=db_lookup(s))) { 	/* in memory */
        snprintf(buf, MAXFILENAME, "delete %s from memory", s);
        if (strcmp(lp->name, "STDIN") !=0 || ask(lp, buf)) {
	    if (debug) printf("db_purge: unlinking %s from db\n", s);
	    db_unlink_cell(sp);
	    flag=1;
	}
    } 

    /* FIXME: later on this will get extended to include a search */
    /* path  instead of just a hard-wired "cells" subdirectory */

    /* now delete copy on disk */

    snprintf(buf, MAXFILENAME, "./cells/%s.d", s);
    if ((access(buf,F_OK)) == 0)  {	/* exists? */
        if (debug) printf("inside exists\n");
        sprintf(buf2, "delete %s from disk", buf);
        if (debug) printf("lexer name = %s", lp->name);
        if (strcmp(lp->name, "STDIN") !=0 || ask(lp, buf2)) {
	    if (debug) printf("removing %s\n", buf);
	    flag+=2;
	    remove(buf);
	}
    }

    /* we wiped out both memory and disk versions of the cell */
    /* so we'd better scan the DB and wipe out all calls to it */

    if (flag==3) {
    	for (dp=HEAD; dp!=(DB_TAB *)0; dp=dp->next) {
	    for (p=dp->dbhead; p!=(struct db_deflist *)0; p=p->next) {
		if (p->type == INST && strcmp(p->u.i->name, s)==0) {
		     /* this one has to go! */
		     printf("unresolvable references to %s in %s\n",
		     	s, dp->name);
			/* p->u.i->name); */
		     	
		}
     	    }
	}
    }
}


/* create a copy of a component with coordinates optionally transformed */
/* by instance options.  If pinstdef is null, no transform done */
/* We allow arbitrary mirroring, rotation and scaling */
/* We assume here that there is no shearing or aspecting. */
/* Rectangles rotated by non-multiples of 90 degrees */
/* are converted into equivalent polygons */
// DB_DEFLIST *p;	       /* component to be copied */
// DB_DEFLIST *pinstdef;  /* parent instance with transform for smashing */

DB_DEFLIST *db_copy_component(DB_DEFLIST *p, DB_DEFLIST *pinstdef) 		/* create a copy of a component */
{
    struct db_deflist *dp;
    XFORM *xp;
    double x,y;

    if (p==NULL) {
    	printf("db_copy_component: can't copy a null component\n");
	return(NULL);
    }

    if (pinstdef != NULL) {
        xp = matrix_from_opts(pinstdef->u.i->opts);
	xp->dx = pinstdef->u.i->x;
	xp->dy = pinstdef->u.i->y;
    } else {
        xp = matrix_from_opts(NULL);	/* returns unit xform */
    }

    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;
    dp->prev = NULL;
    dp->type = p->type;

    dp->xmin = p->xmin;
    dp->ymin = p->ymin;
    dp->xmax = p->xmax;
    dp->ymax = p->ymax;

    switch (p->type) {
    case ARC:  /* arc definition */
	dp->u.a = (struct db_arc *) emalloc(sizeof(struct db_arc));
	dp->u.a->layer=p->u.a->layer;
	dp->u.a->opts=opt_copy(p->u.a->opts);

    	if (pinstdef != NULL) {
	    dp->u.a->opts->width *= pinstdef->u.i->opts->scale;
	} 

	dp->u.a->x1 = p->u.a->x1; dp->u.a->y1 = p->u.a->y1;
	xform_point(xp, &(dp->u.a->x1), &(dp->u.a->y1));

	dp->u.a->x2 = p->u.a->x2; dp->u.a->y2 = p->u.a->y2;
	xform_point(xp, &(dp->u.a->x2), &(dp->u.a->y2));

	dp->u.a->x3 = p->u.a->x3; dp->u.a->y3 = p->u.a->y3;
	xform_point(xp, &(dp->u.a->x3), &(dp->u.a->y3));
	break;

    case CIRC:  /* circle definition */
	dp->u.c = (struct db_circ *) emalloc(sizeof(struct db_circ));
	dp->u.c->layer=p->u.c->layer;
	dp->u.c->opts=opt_copy(p->u.c->opts);

    	if (pinstdef != NULL) {
	    dp->u.c->opts->width *= pinstdef->u.i->opts->scale;
	} 

	dp->u.c->x1 = p->u.c->x1; dp->u.c->y1 = p->u.c->y1;
	xform_point(xp, &(dp->u.c->x1), &(dp->u.c->y1));

	dp->u.c->x2 = p->u.c->x2; dp->u.c->y2 = p->u.c->y2;
	xform_point(xp, &(dp->u.c->x2), &(dp->u.c->y2));
	break;

    case LINE:  /* line definition */
	dp->u.l = (struct db_line *) emalloc(sizeof(struct db_line));
	dp->u.l->layer=p->u.l->layer;
	dp->u.l->opts=opt_copy(p->u.l->opts);

    	if (pinstdef != NULL) {
	    dp->u.l->opts->width *= pinstdef->u.i->opts->scale;
	} 

	dp->u.l->coords=coord_copy(xp, p->u.l->coords); 
	break;

    case NOTE:  /* note definition */
	dp->u.n = (struct db_note *) emalloc(sizeof(struct db_note));
	dp->u.n->layer=p->u.n->layer;
	dp->u.n->opts=opt_copy(p->u.n->opts);

    	if (pinstdef != NULL) {
	    dp->u.n->opts->rotation += pinstdef->u.i->opts->rotation;
	    dp->u.n->opts->font_size *= pinstdef->u.i->opts->scale;
	    dp->u.n->opts->mirror ^= pinstdef->u.i->opts->mirror;
	} 

	dp->u.n->text=strsave(p->u.n->text);

	dp->u.n->x = p->u.n->x; dp->u.n->y = p->u.n->y;
	xform_point(xp, &(dp->u.n->x), &(dp->u.n->y));
    	break;
    case OVAL:  /* oval definition */
	dp->u.o = (struct db_oval *) emalloc(sizeof(struct db_oval));
	dp->u.o->layer=p->u.o->layer;
	dp->u.o->opts=opt_copy(p->u.o->opts);

    	if (pinstdef != NULL) {
	    dp->u.o->opts->width *= pinstdef->u.i->opts->scale;
	} 

	dp->u.o->x1 = p->u.o->x1; dp->u.o->y1 = p->u.o->y1;  	/* first foci */
	xform_point(xp, &(dp->u.o->x1), &(dp->u.o->y1));

	dp->u.o->x2 = p->u.o->x2; dp->u.o->y2 = p->u.o->y2;  	/* second foci */
	xform_point(xp, &(dp->u.o->x2), &(dp->u.o->y2));

	dp->u.o->x3 = p->u.o->x3; dp->u.o->y3 = p->u.o->y3;  	/* point on curve */
	xform_point(xp, &(dp->u.o->x3), &(dp->u.o->y3));
	break;

    case POLY:  /* polygon definition */
	dp->u.p = (struct db_poly *) emalloc(sizeof(struct db_poly));
	dp->u.p->layer=p->u.p->layer;
	dp->u.p->opts=opt_copy(p->u.p->opts);

    	if (pinstdef != NULL) {
	    dp->u.p->opts->width *= pinstdef->u.i->opts->scale;
	} 

	dp->u.p->coords=coord_copy(xp, p->u.p->coords);
	break;

    case RECT:  /* rectangle definition */

	if (pinstdef != NULL && fmod(pinstdef->u.i->opts->rotation, 90.0) != 0) {

	    /* non-ortho rotation, convert rectangle to polygon */

	    dp->u.p = (struct db_poly *) emalloc(sizeof(struct db_poly));
	    dp->u.p->layer=p->u.r->layer;
	    dp->u.p->opts=opt_copy(p->u.r->opts);
	    dp->u.p->opts->width *= pinstdef->u.i->opts->scale;

	    dp->type = POLY;	/* override default */

	    x = p->u.r->x1; y = p->u.r->y1; xform_point(xp, &x, &y);
	    dp->u.p->coords = coord_new(x, y);

	    x = p->u.r->x1; y = p->u.r->y2; xform_point(xp, &x, &y);
	    coord_append(dp->u.p->coords, x, y);

	    x = p->u.r->x2; y = p->u.r->y2; xform_point(xp, &x, &y);
	    coord_append(dp->u.p->coords, x, y);

	    x = p->u.r->x2; y = p->u.r->y1; xform_point(xp, &x, &y);
	    coord_append(dp->u.p->coords, x, y);

	} else {

	    dp->u.r = (struct db_rect *) emalloc(sizeof(struct db_rect));
	    dp->u.r->layer=p->u.r->layer;
	    dp->u.r->opts=opt_copy(p->u.r->opts);

	    if (pinstdef != NULL) {
		dp->u.r->opts->width *= pinstdef->u.i->opts->scale;
	    } 

	    dp->u.r->x1 = p->u.r->x1; dp->u.r->y1 = p->u.r->y1;
	    xform_point(xp, &(dp->u.r->x1), &(dp->u.r->y1));

	    dp->u.r->x2 = p->u.r->x2; dp->u.r->y2 = p->u.r->y2;
	    xform_point(xp, &(dp->u.r->x2), &(dp->u.r->y2));
	}
    	break;

    case TEXT:  /* text definition */
	dp->u.t = (struct db_text *) emalloc(sizeof(struct db_text));
	dp->u.t->layer=p->u.t->layer;
	dp->u.t->opts=opt_copy(p->u.t->opts);

    	if (pinstdef != NULL) {
	    dp->u.t->opts->rotation += pinstdef->u.i->opts->rotation;
	    dp->u.t->opts->font_size *= pinstdef->u.i->opts->scale;
	    dp->u.t->opts->mirror ^= pinstdef->u.i->opts->mirror;
	} 

	dp->u.t->text=strsave(p->u.t->text);

	dp->u.t->x = p->u.t->x; dp->u.t->y = p->u.t->y;
	xform_point(xp, &(dp->u.t->x), &(dp->u.t->y));
    	break;
    case INST:  /* instance call */

	dp->u.i = (struct db_inst *) emalloc(sizeof(struct db_inst));
	dp->u.i->opts=opt_copy(p->u.i->opts);

    	if (pinstdef != NULL) {
	    dp->u.i->opts->rotation += pinstdef->u.i->opts->rotation;
	    dp->u.i->opts->scale *= pinstdef->u.i->opts->scale;
	    dp->u.i->opts->mirror ^= pinstdef->u.i->opts->mirror;
	} 

	dp->u.i->name=strsave(p->u.i->name);
	dp->u.i->x = p->u.i->x; dp->u.i->y = p->u.i->y;
	dp->u.i->colx = p->u.i->colx; dp->u.i->coly = p->u.i->coly;
	dp->u.i->rowx = p->u.i->rowx; dp->u.i->rowy = p->u.i->rowy;

	xform_point(xp, &(dp->u.i->x), &(dp->u.i->y));
	break;
    default:
    	printf("bad case in db_copy_component\n"); 
	break;
    }
    return (dp);
}

/* push copy of currep if changed w.r.t. old copy */

int db_checkpoint(LEXER *lp) {	
    int cknew;
    int ckold;
    DB_DEFLIST *copy;
    int debug = 0;
    char buf[128];

    if (currep == NULL || strcmp(lp->name, "STDIN") != 0) {
       return(0);
    }

    cknew = db_cksum(currep->dbhead);
    ckold = db_cksum((DB_DEFLIST *) stack_top(&(currep->undo)));

    if (cknew != ckold) {
	if (debug) printf("inside db_checkpoint cknew = %d, ckold = %d\n", cknew, ckold);
        copy = db_copy_deflist(currep->dbhead); /* copy db_deflist chain */

	if (debug) printf("pushing save copy onto undo stack new:%x old:%x\n", cknew, ckold);
	stack_push(&(currep->undo), (char *) copy);

        while ((copy = (DB_DEFLIST *) stack_pop(&(currep->redo)))!=NULL) { 
	    db_free(copy); 			/* clear out redo stack */
	}
	
	// simple autosave function 
	// after every change, write a copy to "cells/#<cellname>.d"

	if (debug) printf("inside db_checkpoint currepcksum = %d, cknew = %d\n", currep->chksum, cknew);
	if (currep->chksum != cknew && currep->modified) {
	    strcpy(buf,"#");		    
	    strncat(buf,currep->name, 128);		    
	    db_save(lp, currep, buf);
	    currep->chksum = cknew;
	    if (debug) printf("saved\n");
	}

	return(1);
    }
    return(0);
}

DB_DEFLIST *db_copy_deflist(DB_DEFLIST *head) {	/* copy db_deflist chain */

    DB_DEFLIST *p;
    DB_DEFLIST *tmp;
    DB_DEFLIST *copy=NULL;

    for (p=head; p!=(DB_DEFLIST *)0; p=p->next) {
	tmp=db_copy_component(p, NULL);
        if (copy == NULL) {
           copy=tmp;
           copy->prev = tmp;
        } else {
           copy->prev->next = tmp;
           tmp->prev = copy->prev;
           copy->prev = tmp;
        }
    } 
    return(copy);
}

void db_free_component(DB_DEFLIST *p) 		/* recycle memory for component */
{
    int debug=0;
    COORDS *coords;
    COORDS *ncoords;

    if (p == NULL) {
    	printf("db_free_component: can't free a NULL component\n");
    } else {
	switch (p->type) {

	case ARC:  /* arc definition */
	    if (debug) printf("db_purge: freeing arc\n");
	    free(p->u.a->opts->cname);
	    free(p->u.a->opts->sname);
	    free(p->u.a->opts);
	    free(p->u.a);
	    free(p);
	    break;

	case CIRC:  /* circle definition */
	    if (debug) printf("db_purge: freeing circle\n");
	    free(p->u.c->opts->cname);
	    free(p->u.c->opts->sname);
	    free(p->u.c->opts);
	    free(p->u.c);
	    free(p);
	    break;

	case LINE:  /* line definition */

	    if (debug) printf("db_purge: freeing line\n");
	    coords = p->u.l->coords;
	    while(coords != NULL) {
		ncoords = coords->next;
		free(coords);	
		coords=ncoords;
	    }
	    free(p->u.l->opts->cname);
	    free(p->u.l->opts->sname);
	    free(p->u.l->opts);
	    free(p->u.l);
	    free(p);
	    break;

	case NOTE:  /* note definition */

	    if (debug) printf("db_purge: freeing note\n");
	    free(p->u.n->opts->cname);
	    free(p->u.n->opts->sname);
	    free(p->u.n->opts);
	    free(p->u.n->text);
	    free(p->u.n);
	    free(p);
	    break;

	case OVAL:  /* oval definition */

	    if (debug) printf("db_purge: freeing oval\n");
	    free(p->u.o->opts->cname);
	    free(p->u.o->opts->sname);
	    free(p->u.o->opts);
	    free(p->u.o);
	    free(p);
	    break;

	case POLY:  /* polygon definition */

	    if (debug) printf("db_purge: freeing poly\n");
	    coords = p->u.p->coords;
	    while(coords != NULL) {
		ncoords = coords->next;
		free(coords);
		coords=ncoords;
	    }
	    free(p->u.p->opts->cname);
	    free(p->u.p->opts->sname);
	    free(p->u.p->opts);
	    free(p->u.p);
	    free(p);
	    break;

	case RECT:  /* rectangle definition */

	    if (debug) printf("db_purge: freeing rect\n");
	    free(p->u.r->opts->cname);
	    free(p->u.r->opts->sname);
	    free(p->u.r->opts);
	    free(p->u.r);
	    free(p);
	    break;

	case TEXT:  /* text definition */

	    if (debug) printf("db_purge: freeing text\n");
	    free(p->u.t->opts->cname);
	    free(p->u.t->opts->sname);
	    free(p->u.t->opts);
	    free(p->u.t->text);
	    free(p->u.t);
	    free(p);
	    break;

	case INST:  /* instance call */

	    if (debug) printf("db_purge: freeing instance call: %s\n",
		p->u.i->name);
	    free(p->u.i->name);
	    free(p->u.i->opts->cname);
	    free(p->u.i->opts->sname);
	    free(p->u.i->opts);
	    free(p->u.i);
	    free(p);
	    break;

	default:
	    eprintf("unknown record type (%d) in db_free_component\n", p->type );
	    break;
	}
    }
}

/* save a non-recursive archive of single cell to a file called "cell.d" */

int db_save(LEXER *lp, DB_TAB *sp, char *name)           	/* print db */
{

    FILE *fp;
    char buf[MAXFILENAME];
    char buf2[MAXFILENAME];

    int debug=0;

    int err=0;

    mkdir("./cells", 0777);
    snprintf(buf, MAXFILENAME, "./cells/%s.d", name);

    /* check to see if would overwrite ask permission before doing so */
    /* but don't ask if device name matches save name */

    /* autosave names start with "#", so don't ask for those either */

    if (debug) {
	printf("in db_save, lp=%s\n", lp->name); 
	printf("device name=%s, save name=%s\n", sp->name, name);
	printf("access =%d\n", access(buf, F_OK));
    }

    if (  (strcmp(sp->name, name) != 0) &&
       		(strcmp(lp->name, "STDIN") == 0) &&
		(name[0] != '#') &&
       		(access(buf, F_OK) == 0)) {
        snprintf(buf2, MAXFILENAME, "overwrite %s on disk", name);
        if (ask(lp, buf2) == 0) {
	    printf("aborting save.\n");
	    return(1);
	}
    } 
    
    err+=((fp = fopen(buf, "w+")) == 0); 
    if (!err) {
	db_def_print(fp, sp, CELL); 
	err+=(fclose(fp) != 0);
    } else {
        printf("couldn't open %s for saving!\n", buf);
    }
   
    // if save was successful, remove autosave file
    if (!err && name[0] != '#') {
    	snprintf(buf, MAXFILENAME, "./cells/#%s.d", name);
	printf("unlinking %s\n", buf);
	unlink(buf);
    }

    return(err);
}

/* call it with unity xform in xp, ortho == 1 */
// int ortho; 	/* smash mode, 1 = ortho, 0=non-ortho */

int db_arc_smash(DB_TAB *cell, XFORM *xform, int ortho)
{
    // extern XFORM unity_transform;

    DB_DEFLIST *p;
    XFORM *xp;
    XFORM *child_xform;
    int child_ortho;

    if (cell == NULL) {
        printf("bad reference in db_arc_smash\n");
    	return(0);
    }

    for (p=cell->dbhead; p!=(DB_DEFLIST *)0; p=p->next) {
	switch (p->type) {
        case ARC:   /* arc definition */
        case CIRC:  /* circle definition */
        case LINE:  /* line definition */
        case NOTE:  /* note definition */
        case OVAL:  /* oval definition */
        case POLY:  /* polygon definition */
	case RECT:  /* rectangle definition */
        case TEXT:  /* text definition */
	    /* print_smash_def(p, child_xform, ortho); */
	    /* if ortho, then just emit primitive based on xform */
	    /* otherwise convert primitive to polygon */
	    break;
        case INST:  /* recursive instance call */

	    /* create transformation matrix from options */
	    xp = matrix_from_opts(p->u.i->opts);

	    xp->dx += p->u.i->x;
	    xp->dy += p->u.i->y;

	    child_xform = compose(xp,xform);
            free(xp);

	    child_ortho = ortho && is_ortho(p->u.i->opts);

	    /* instances are called by name, not by pointer */
	    /* otherwise, it is possible to PURGE a cell in */
	    /* memory and then reference a bad pointer, causing */
	    /* a crash... so instances are always accessed */
	    /* with a db_lookup() call */

	    if (db_lookup(p->u.i->name) == NULL) {

		printf("skipping ref to %s, no longer in memory\n", p->u.i->name);

	    	/* FIXME: try to reread the definition from disk */
		/* this can only happen when a memory copy has */
		/* been purged */

	    } else {
		/* printf("calling %s, ortho=%d\n", p->u.i->name, child_ortho); */
		db_arc_smash(db_lookup(p->u.i->name), child_xform, child_ortho);
	    }

	    free(child_xform);

	    break;
	default:
	    eprintf("unknown record type: %d in db_arc_smash\n", p->type);
	    return(1);
	    break;
	}
    }
    return(1);
}

/* write out archive file for sp, smash it if smash !=0 */
/* FIXME: include process file if process !=0 */

int db_def_archive(DB_TAB *sp, int smash, int process) 
{
    FILE *fp;
    char buf[MAXFILENAME];
    int err=0;
    DB_TAB *dp;
    char *s;

    mkdir("./cells", 0777);
    snprintf(buf, MAXFILENAME, "./cells/%s_I", sp->name);
    err+=((fp = fopen(buf, "w+")) == 0); 

    fp = fopen(buf, "w+"); 
    
    for (dp=HEAD; dp!=(DB_TAB *)0; dp=dp->next) {
	dp->flag=0;  /* clear recursion flag */
    }

    fprintf(fp,"$FILES\n%s\n",sp->name);	// create $FILES preamble
    stack=NULL;
    db_def_files_recurse(fp,sp);
    while ((s=stack_pop(&stack))!=NULL) {
	fprintf(fp, "%s\n",s);
    }
    fprintf(fp,";\n\n");

    /* clear out all flag bits in cell definition headers */
    for (dp=HEAD; dp!=(DB_TAB *)0; dp=dp->next) {
	dp->flag=0;
    }

    /* now print out definitions of every cell from bottom to top */

    db_def_arch_recurse(fp,sp,smash);	// subcells
    db_def_print(fp, sp, ARCHIVE);	// current cell

    err+=(fclose(fp) != 0);

    return(err);
}

/* When we retrieve an archive, it is important to first purge
   all the files that will be redefined by the archive.  If we've done
   something like: "EDIT FOO; ARC FOO; RET FOO;", then the ARCHIVE
   process will need to purge all the subcells of FOO in hierarchical
   order from top to bottom to avoid creating unreferenced stubs in the
   database at any moment.  It then needs to redefine all the cells from
   bottom to top to avoid using any cell before it has been defined. 
   The db_def_arch_recurse() function computes the bottom-to-top
   ordering by walking the definition tree and using a flag.  It prints
   names starting at the leaves of the tree and the flag bit prevents
   double definitions.  To do the initial purge properly, each archive
   file contains a "$FILE <top_cell> ...  <bottom_cell>" preamble which
   is generated by the following db_def_files_recurse() function.  This
   is a duplicate of the db_def_arch_recurse() function except that it
   uses the stack_push() and stack_pop() routines to reverse the order
   of the cell references. 
*/

int db_def_files_recurse(FILE *fp, DB_TAB *sp) 
{
    DB_DEFLIST *p; 
    DB_TAB *dp;

    for (p=sp->dbhead; p!=(struct db_deflist *)0; p=p->next) {
	if (p->type == INST) {
	    dp = db_lookup(p->u.i->name);
	    if (!(dp->flag) && !(dp->is_tmp_rep)) {	// do not archive tmp reps
		db_def_files_recurse(fp, db_lookup(p->u.i->name)); 	/* recurse */
		((db_lookup(p->u.i->name))->flag)++;	// prevent duplicates
		stack_push(&stack, p->u.i->name);	// save for $FILES
	    }
	}
    }
    return(0); 	
}

int db_def_arch_recurse(FILE *fp, DB_TAB *sp, int smash) 
{
    DB_DEFLIST *p; 
    DB_TAB *dp;

    for (p=sp->dbhead; p!=(struct db_deflist *)0; p=p->next) {
	if (p->type == INST) {
	    dp = db_lookup(p->u.i->name);
	    if (!(dp->flag) && !(dp->is_tmp_rep)) {	// do not archive tmp reps
		db_def_arch_recurse(fp, db_lookup(p->u.i->name), smash); 	/* RCW recurse */
		((db_lookup(p->u.i->name))->flag)++;	// prevent duplicates
		db_def_print(fp, db_lookup(p->u.i->name), ARCHIVE);  // print def
	    }
	}
    }
    return(0); 	
}


// list any unsaved files at BYE.  If user says "BYE; BYE;"
// then db_remove_autosaved() files will be called to remove
// all edits in progress...

int db_list_unsaved()
{
    DB_TAB *dp;
    int numunsaved=0;
    for (dp=HEAD; dp!=(DB_TAB *)0; dp=dp->next) {

	/* do not complain about NONAME reps */
    	if (dp->modified && !dp->is_tmp_rep) {
	    printf("    device <%s> is modified!\n", dp->name);
	    numunsaved++;
	}

    }
    return(numunsaved);
}

// FIXME: there are numerous instances of creating the autosave file
// with sprintf as below.  These should all be changed to call
// a single subroutine:
// const char * make_autosavefilename( char *repname );
//

int db_remove_autosavefiles()
{
    DB_TAB *dp;
    int numremoved=0;
    char buf[MAXFILENAME];

    for (dp=HEAD; dp!=(DB_TAB *)0; dp=dp->next) {

	/* do not remove NONAME reps */
    	if (dp->modified && !dp->is_tmp_rep) {
    	    snprintf(buf, MAXFILENAME, "./cells/#%s.d", dp->name);
	    printf("unlinking autosave file: %s\n", buf);
	    unlink(buf);
	    numremoved++;
	}

    }
    return(numremoved);
}


int db_contains(char *name1, char *name2) /* return 0 if sp contains no reference to "name" */
{

    DB_TAB *sp;
    DB_TAB *dp;
    int retval = 0;
    int debug=0;
	 
    if ((sp=db_lookup(name1)) == NULL) {
    	printf("error in db_contains\n");
    }

    /* clear out all flag bits in cell definition headers */
    for (dp=HEAD; dp!=(DB_TAB *)0; dp=dp->next) {
	dp->flag=0;
    }

    if (debug) printf("db_contains called with %s %s\n", name1, name2);

    db_contains_body(sp, name2, &retval); /* recurse */
    return (retval);
}

void db_contains_body(DB_TAB *sp, char *name, int *retval) 
{
    DB_DEFLIST *p; 
    int debug=0;

    /* FIXME protect against db_lookup returning NULL */
    /* which can happen if something is purged during an edit */
    /* perhaps it would also be good to have PURGE delete all references */
    /* to any PURGED cell? Also need to think about the policy for */
    /* what happens when a cell is edited that references non-existant */
    /* instances... */

    for (p=sp->dbhead; p!=(struct db_deflist *)0; p=p->next) {
	if (p->type == INST && 
	    db_lookup(p->u.i->name) != NULL &&
	    !(db_lookup(p->u.i->name)->flag)) {

		db_contains_body(db_lookup(p->u.i->name), name, retval);
		((db_lookup(p->u.i->name))->flag)++;

		if (debug) printf("%s %s %s\n", sp->name, p->u.i->name, name); 

		if (strcmp(p->u.i->name, name) == 0) {
		    (*retval)++;
		}
	}
    }
}

void printcoord(FILE *fp, double x) {
    double xint, xfrac;
    char format[16];
    static char buf[32];
    int i;
    double sign;

    if (x<0) {
       sign=-1.0;
    } else {
       sign=1.0;
    }

    x=fabs(x);

    xfrac = fmod(x,1.0);
    xint = x-xfrac;
    xfrac = (double)((int)((xfrac*pow(10.0,RES))+0.5));
    if (xfrac == (double)((int)pow(10.0,RES))) {
       xint+=1.0;
       xfrac=0.0;
    }

    if ((int)xfrac != 0) {
        if (sign<0.0) {
            sprintf(format,"-%%d.%%0%dd", RES);
        } else {
            sprintf(format,"%%d.%%0%dd", RES);
        }
        sprintf(buf, format, (int)(xint), (int)(xfrac+0.5));
        for (i=strlen(buf)-1; i>=0; i--) {
           if (buf[i]!='0') {
              break;
           } else {
              buf[i]='\0';
           }
        }
    } else {
	sprintf(format,"%%d");
	sprintf(buf, format, (int)(sign*xint));
    }

    fprintf(fp, "%s", buf);
}

void printcoords(FILE *fp, XFORM *xp, double x, double y) {
    xform_point(xp, &x, &y);
    fprintf(fp," ");
    printcoord(fp,x);
    fprintf(fp,",");
    printcoord(fp,y);
}

void oldprintcoords(FILE *fp, XFORM *xp, double x, double y) {
    double xx, yy;
    xform_point(xp, &x, &y);
    // xx = ((double)(int)(x*pow(10.0,RES)+0.5))/pow(10.0,RES);
    // yy = ((double)(int)(y*pow(10.0,RES)+0.5))/pow(10.0,RES);
    xx = x-fmod(x,1.0/pow(10.0,RES));
    yy = y-fmod(y,1.0/pow(10.0,RES));
    // printf("called with %g %g %g %g\n", x,y,xx,yy);
    fprintf(fp, " %.10g,%.10g", xx,yy);
}

/* printdef writes the definition of a single component *p to the */
/* the FILE *fp.  If *pinstdef is not NULL, then printdef will */
/* transform the definitions by the offset and transform pointed */
/* to by *pinstdef.  This is used when auto-smashing NONAME instances */
/* during SAVes.  *p points to one of the sub components of the NONAME */
/* instance, and *pinstdef is the pointer to NONAME's definition */

void printdef(FILE *fp, DB_DEFLIST *p, DB_DEFLIST *pinstdef) {

    COORDS *coords;
    DB_TAB *sp;
    DB_DEFLIST *tp;
    int i;
    XFORM *xp;
    OPTS *opts;
    double xold, yold;

    if (pinstdef != NULL) {
        xp = matrix_from_opts(pinstdef->u.i->opts);
	xp->dx = pinstdef->u.i->x;
	xp->dy = pinstdef->u.i->y;
    } else {
        xp = matrix_from_opts(NULL);	/* returns unit xform */
    }

    switch (p->type) {

    case ARC:  /* arc definition */

	fprintf(fp, "ADD A%d ", p->u.a->layer);

    	if (pinstdef != NULL) {
	    opts = opt_copy(p->u.a->opts);
	    opts->width *= pinstdef->u.i->opts->scale;
	    db_print_opts(fp, opts, ARC_OPTS);
	    free(opts);
	} else {
	    db_print_opts(fp, p->u.a->opts, ARC_OPTS);
	}

	printcoords(fp, xp, p->u.a->x1, p->u.a->y1);
	printcoords(fp, xp, p->u.a->x2, p->u.a->y2);
	printcoords(fp, xp, p->u.a->x3, p->u.a->y3);
	fprintf(fp, ";\n");

	break;

    case CIRC:  /* circle definition */

	fprintf(fp, "ADD C%d ", p->u.c->layer);

    	if (pinstdef != NULL) {
	    opts = opt_copy(p->u.c->opts);
	    opts->width *= pinstdef->u.i->opts->scale;
	    db_print_opts(fp, opts, CIRC_OPTS);
	    free(opts);
	} else {
	    db_print_opts(fp, p->u.c->opts, CIRC_OPTS);
	}

	printcoords(fp, xp, p->u.c->x1, p->u.c->y1);
	printcoords(fp, xp, p->u.c->x2, p->u.c->y2);
	fprintf(fp, ";\n");
	break;

    case LINE:  /* line definition */
	fprintf(fp, "ADD L%d ", p->u.l->layer);

    	if (pinstdef != NULL) {
	    opts = opt_copy(p->u.l->opts);
	    opts->width *= pinstdef->u.i->opts->scale;
	    db_print_opts(fp, opts, LINE_OPTS);
	    free(opts);
	} else {
	    db_print_opts(fp, p->u.l->opts, LINE_OPTS);
	}

	i=1;
	coords = p->u.l->coords;
	while(coords != NULL) {
	    fprintf(fp, " ");
	    printcoords(fp, xp,  coords->coord.x, coords->coord.y);
	    if ((coords = coords->next) != NULL && (++i)%7 == 0) {
		fprintf(fp,"\n    ");
	    }
	}
	fprintf(fp, ";\n");

	break;

    case NOTE:  /* note definition */

	fprintf(fp, "ADD N%d ", p->u.n->layer);

    	if (pinstdef != NULL) {
	    opts = opt_copy(p->u.n->opts);
	    opts->rotation += pinstdef->u.i->opts->rotation;
	    opts->font_size *= pinstdef->u.i->opts->scale;
	    opts->mirror = (opts->mirror)^(pinstdef->u.i->opts->mirror);
	    db_print_opts(fp, opts, NOTE_OPTS);
	    free(opts);
	} else {
	    db_print_opts(fp, p->u.n->opts, NOTE_OPTS);
	}

	fprintf(fp, "\"%s\" ", p->u.n->text);
	printcoords(fp, xp,  p->u.n->x, p->u.n->y);
	fprintf(fp, ";\n");
	break;

    case OVAL:  /* oval definition */

	fprintf(fp, "#add OVAL not implemented yet\n");
	break;

    case POLY:  /* polygon definition */

	fprintf(fp, "ADD P%d", p->u.p->layer);

    	if (pinstdef != NULL) {
	    opts = opt_copy(p->u.p->opts);
	    opts->width *= pinstdef->u.i->opts->scale;
	    db_print_opts(fp, opts, POLY_OPTS);
	    free(opts);
	} else {
	    db_print_opts(fp, p->u.p->opts, POLY_OPTS);
	}

	i=1;
	coords = p->u.p->coords;
	xold = coords->coord.x-1.0;	// make it different
	yold = coords->coord.y-1.0;
	while(coords != NULL) {
	    fprintf(fp, " ");
	    if (coords->coord.x != xold || coords->coord.y != yold) {
		// supress cooincident points created by stretching
		printcoords(fp, xp,  coords->coord.x, coords->coord.y);
	    }
	    xold = coords->coord.x;
	    yold = coords->coord.y;
	    if ((coords = coords->next) != NULL && (++i)%7 == 0) {
		fprintf(fp,"\n    ");
	    }
	}
	fprintf(fp, ";\n");

	break;

    case RECT:  /* rectangle definition */

	if (pinstdef != NULL && fmod(pinstdef->u.i->opts->rotation, 90.0) != 0) {

	    /* non-ortho rotation so convert to a polygon */

	    fprintf(fp, "ADD P%d", p->u.p->layer);
	    opts = opt_copy(p->u.r->opts);
	    opts->width *= pinstdef->u.i->opts->scale;
	    db_print_opts(fp, opts, POLY_OPTS);
	    free(opts);
	    fprintf(fp, " ");
	    printcoords(fp, xp,  p->u.r->x1, p->u.r->y1);
	    printcoords(fp, xp,  p->u.r->x1, p->u.r->y2);
	    printcoords(fp, xp,  p->u.r->x2, p->u.r->y2);
	    printcoords(fp, xp,  p->u.r->x2, p->u.r->y1);
	    fprintf(fp, ";\n");
	
	} else {

	    fprintf(fp, "ADD R%d ", p->u.r->layer);

	    if (pinstdef != NULL) {
		opts = opt_copy(p->u.r->opts);
		opts->width *= pinstdef->u.i->opts->scale;
		db_print_opts(fp, opts, RECT_OPTS);
		free(opts);
	    } else {
		db_print_opts(fp, p->u.r->opts, RECT_OPTS);
	    }

	    printcoords(fp, xp,  p->u.r->x1, p->u.r->y1);
	    printcoords(fp, xp,  p->u.r->x2, p->u.r->y2);
	    fprintf(fp, ";\n");
	}

	break;

    case TEXT:  /* text definition */

	fprintf(fp, "ADD T%d ", p->u.t->layer); 

    	if (pinstdef != NULL) {
	    opts = opt_copy(p->u.t->opts);
	    opts->rotation += pinstdef->u.i->opts->rotation;
	    opts->font_size *= pinstdef->u.i->opts->scale;
	    opts->mirror = (opts->mirror)^(pinstdef->u.i->opts->mirror);
	    db_print_opts(fp, opts, TEXT_OPTS);
	    free(opts);
	} else {
	    db_print_opts(fp, p->u.t->opts, TEXT_OPTS);
	}

	fprintf(fp, "\"%s\" ", p->u.t->text);
	printcoords(fp, xp,  p->u.t->x, p->u.t->y);
	fprintf(fp, ";\n");
	break;
		
    case INST:  /* instance call */
	if ((sp=db_lookup(p->u.i->name)) && sp->is_tmp_rep) { 	/* a tmp rep */
	    if (sp->is_tmp_rep) {
		printf("    Smashing anonymous instance %s to disk\n", sp->name);
		for (tp=sp->dbhead; tp!=(struct db_deflist *)0; tp=tp->next) {
		    printdef(fp, tp, p);
		}
	    }
	} else {

	    // we need to quote the instance name, otherwise something like
	    // add p3 0,0; will fail because "p3" is a polygon on layer 3
	    // instead of an inst name.  With quotes, the parser will retrieve
	    // the cell correctly.  The user, however will still have to use
	    // quotes while editing if they want to add a copy of "p3" to a 
	    // new device...

	    fprintf(fp, "ADD \"%s\" ", p->u.i->name);
	    if (pinstdef != NULL) {
		opts = opt_copy(p->u.i->opts);
		opts->rotation += pinstdef->u.i->opts->rotation;
	        opts->scale *= pinstdef->u.i->opts->scale;
	        opts->mirror = (opts->mirror)^(pinstdef->u.i->opts->mirror);
	        db_print_opts(fp, opts, INST_OPTS);
		free(opts);
	    } else {
	        db_print_opts(fp, p->u.i->opts, INST_OPTS);
	    }
	    printcoords(fp, xp,  p->u.i->x, p->u.i->y);
	    if (p->u.i->opts->stepflag) {
		printcoords(fp, xp,  p->u.i->colx, p->u.i->coly);
		printcoords(fp, xp,  p->u.i->rowx, p->u.i->rowy);
	    }
	    fprintf(fp, ";\n");
	}
	break;

    default:
	eprintf("unknown record type (%d) in printdef\n", p->type );
	break;
    }

    free(xp);
}

/* ------------------------------------------- */

static int digestvalue;

void digestinit(void) {
    extern int digestvalue;
    digestvalue = 0;
}

void digestint(int x) {
    extern int digestvalue;
    digestvalue *= 13;
    digestvalue += x;
}

void digestdouble(double a) {
    extern int digestvalue;
    unsigned char *p;
    int debug=0;
    int i;

    if (debug) printf("(%12.12g)", a);
    p = (unsigned char *) &a;
    for (i=0; i<sizeof(double); i++) {
	if (debug) printf("%d ", (char) p[i]);
	digestvalue *= 13;
	digestvalue += (char) p[i];
    }
    if (debug) printf("\n");
}

void digeststr(char *s) {
    while(*s != 0) {
	digestint((int) *s);
        ++s;
    }
}

void digestopts(OPTS *opts, char *optstring) {
    digestdouble(opts->font_size);
    digestint(opts->mirror);
    digestint(opts->font_num);
    digestint(opts->justification);
    digestdouble(opts->rotation);
    digestdouble(opts->width);
    digestdouble(opts->scale);
    digestdouble(opts->aspect_ratio);
    digestdouble(opts->slant);
    if (opts->cname != NULL) digeststr(opts->cname);
    if (opts->sname != NULL) digeststr(opts->sname);
}

void digestcoords(COORDS *cp) {
    int debug=0;
    COORDS *p=cp;

    while(p != NULL) {
        digestdouble(p->coord.x);
        digestdouble(p->coord.y);
	if (debug) printf("%g %g\n", p->coord.x, p->coord.y);
	p = p->next;
    }
}

int digestval() {
    return(digestvalue);
}

void cksum(DB_DEFLIST *p) {

    int debug = 0;

    switch (p->type) {

    case ARC:  /* arc definition */

        if (debug) printf("arc ");
	digestint(p->u.a->layer);
        digestopts(p->u.a->opts, ARC_OPTS);
	digestdouble(p->u.a->x1); digestdouble(p->u.a->y1);
	digestdouble(p->u.a->x2); digestdouble(p->u.a->y2);
	digestdouble(p->u.a->x3); digestdouble(p->u.a->y3);
	break;

    case CIRC:  /* circle definition */

        if (debug) printf("circ %g %g %g %g ", p->u.c->x1, p->u.c->y1, p->u.c->x2, p->u.c->y2);
	digestint(p->u.c->layer);
        digestopts(p->u.c->opts, CIRC_OPTS);
	digestdouble(p->u.c->x1); digestdouble(p->u.c->y1);
	digestdouble(p->u.c->x2); digestdouble(p->u.c->y2);
	break;

    case LINE:  /* line definition */

        if (debug) printf("line ");
	digestint(p->u.l->layer);
        digestopts(p->u.l->opts, LINE_OPTS);
        digestcoords(p->u.l->coords);
	break;

    case NOTE:  /* note definition */

        if (debug) printf("note ");
	digestint(p->u.n->layer);
	digestopts(p->u.n->opts, TEXT_OPTS);
	digeststr(p->u.n->text);
	digestdouble(p->u.n->x); digestdouble(p->u.n->y);
	break;

    case OVAL:  /* oval definition */

	break;

    case POLY:  /* polygon definition */

        if (debug) printf("poly ");
	digestint(p->u.p->layer);
	digestopts(p->u.p->opts, POLY_OPTS);
        digestcoords(p->u.p->coords);
	break;

    case RECT:  /* rectangle definition */

        if (debug) printf("rect ");
	digestint(p->u.r->layer);
	digestopts(p->u.r->opts, RECT_OPTS);
	digestdouble(p->u.r->x1); digestdouble(p->u.r->y1);
	digestdouble(p->u.r->x2); digestdouble(p->u.r->y2);
	break;

    case TEXT:  /* text definition */

        if (debug) printf("text %s ", p->u.t->text);
	digestint(p->u.t->layer);
	digestopts(p->u.t->opts, TEXT_OPTS);
	digeststr(p->u.t->text);
	digestdouble(p->u.t->x); digestdouble(p->u.t->y);
	break;

    case INST:  /* instance call */

        if (debug) printf("inst ");
	digeststr(p->u.i->name);
	digestopts(p->u.i->opts, INST_OPTS);
	digestdouble(p->u.i->x); digestdouble(p->u.i->y);
	break;

    default:
	eprintf("unknown record type (%d) in cksum\n", p->type );
	break;
    }
}

int db_cksum(DB_DEFLIST *dp) 
{
    DB_DEFLIST *p; 
    int debug=0;

    if (dp==NULL) {
        return(0); 
    } else {
	digestinit();
	for (p=dp; p!=(struct db_deflist *)0; p=p->next) {
	    cksum(p);
	    if (debug) printf("# %ld %d\n", (long int) p, digestval());
	}
	return(digestval());
    }
}


void db_def_print(FILE *fp, DB_TAB *dp, int mode) 
{
    DB_DEFLIST *p; 
    time_t now;

    if (mode == ARCHIVE) {
	/* fprintf(fp, "PURGE %s;\n", dp->name);  */
	fprintf(fp, "EDIT %s;\n", dp->name);
	fprintf(fp, "SHOW #E;\n");
	fprintf(fp, "DISP OFF;\n"); 
    }

    now = time(NULL);
    fprintf(fp, "$$ Saved: %s", ctime(&now));
    fprintf(fp, "LOCK %g;\n", dp->lock_angle);
    fprintf(fp, "LEVEL %d;\n", dp->logical_level);
    fprintf(fp, "GRID :C%d %g %g %g %g %g %g;\n", 
        dp->grid_color,
    	dp->grid_xd, dp->grid_yd,
	dp->grid_xs, dp->grid_ys,
	dp->grid_xo, dp->grid_yo);

    fprintf(fp, "WIN ");
    printcoords(fp, &unity_transform, dp->minx, dp->miny);
    printcoords(fp, &unity_transform, dp->maxx, dp->maxy);
    fprintf(fp, ";\n");

    for (p=dp->dbhead; p!=(struct db_deflist *)0; p=p->next) {
	printdef(fp, p, NULL);
    }

    if (mode == ARCHIVE) {
	fprintf(fp, "DISP ON;\n"); 
	fprintf(fp, "SAVE;\n");
	fprintf(fp, "EXIT;\n\n");
    }
}

//          +++++++++++++++
//          |             v
//   H-->[ f ]-->[ s ]-->[ l ]--->/
//        ^       | ^      | 
//        +-------- +------  

// DB_TAB *cell;		   /* current rep */
// struct db_deflist *dp;          /* pbest */

void db_unlink_component(DB_TAB *cell, DB_DEFLIST *dp) 
{

    if(dp == NULL) {
    	printf("can't delete a null instance\n");
    } else if(cell->dbhead == dp ) {	/* first one the list */
	cell->dbhead = dp->next;
	if (dp->next != NULL) {
	    dp->next->prev = dp->prev;
	}
    } else if(dp->next == NULL ) {	/* last in the list */
	dp->prev->next = NULL;
	cell->dbhead->prev = dp->prev;
    } else {				/* somewhere in the chain */
	dp->prev->next = dp->next;
	dp->next->prev = dp->prev;
    }

    db_free_component(dp);	        /* gone for good now! */
    currep->modified++;
}

//          +++++++++++++++
//          |             v
//   H-->[ f ]-->[ s ]-->[ l ]--->/
//        ^       | ^      | 
//        +-------- +------  

// DB_TAB *cell;
// struct db_deflist *dp;

void db_insert_component(DB_TAB *cell, struct db_deflist *dp) 
{
    dp->next = NULL;

    /* add definition at *end* of list */
    if (cell->dbhead == NULL) {
	cell->dbhead = dp;
	dp->prev = dp;	/* only one, so prev points at itself */
    } else {
	dp->prev = cell->dbhead->prev; 	/* doubly linked list */
	cell->dbhead->prev->next = dp;
	cell->dbhead->prev = dp;
    }

    cell->modified++;
}

void db_set_layer(struct db_deflist *p, int new_layer) /* set the layer of any component */
{
    if (p == NULL) {
    	printf("db_set_layer: can't change layer on null component!\n");
    } else {
	switch (p->type) {
	    case ARC:
		p->u.a->layer = new_layer;
		break;
	    case CIRC:
		p->u.c->layer = new_layer;
		break;
	    case INST:
		break;
	    case LINE:
		p->u.l->layer = new_layer;
		break;
	    case NOTE:
		p->u.n->layer = new_layer;
		break;
	    case OVAL:
		p->u.o->layer = new_layer;
		break;
	    case POLY:
		p->u.p->layer = new_layer;
		break;
	    case RECT:
		p->u.r->layer = new_layer;
		break;
	    case TEXT:
		p->u.t->layer = new_layer;
		break;
	    default:
		printf("db_change_layer: unknown case\n");
		break;
	}
    }
}

void db_move_component(struct db_deflist *p, double dx, double dy) /* move any component by dx, dy */
{
    COORDS *coords;

    switch (p->type) {
    case ARC:  /* arc definition */
	p->u.a->x1 += dx;
	p->u.a->y1 += dy;
	p->u.a->x2 += dx;
	p->u.a->y2 += dy;
	p->u.a->x3 += dx;
	p->u.a->y3 += dy;
	break;
    case CIRC:  /* circle definition */
	p->u.c->x1 += dx;
	p->u.c->y1 += dy;
	p->u.c->x2 += dx;
	p->u.c->y2 += dy;
	break;
    case LINE:  /* line definition */
	coords = p->u.l->coords;
	while(coords != NULL) {
	    coords->coord.x += dx;
	    coords->coord.y += dy;
	    coords = coords->next;
	}
	break;
    case NOTE:  /* note definition */
	p->u.n->x += dx;
	p->u.n->y += dy;
	break;
    case OVAL:  /* oval definition */
        /* FIXME: Not implemented */
	/* for now just use oblique circles */
	break;
    case POLY:  /* polygon definition */
	coords = p->u.p->coords;
	while(coords != NULL) {
	    coords->coord.x += dx;
	    coords->coord.y += dy;
	    coords = coords->next;
	}
	break;
    case RECT:  /* rectangle definition */
	p->u.r->x1 += dx;
	p->u.r->y1 += dy;
	p->u.r->x2 += dx;
	p->u.r->y2 += dy;
	break;
    case TEXT:  /* text definition */
	p->u.t->x += dx;
	p->u.t->y += dy;
	break;
    case INST:  /* instance call */
	p->u.i->x += dx;
	p->u.i->y += dy;
	break;

    default:
	eprintf("unknown record type (%d) in db_move_component\n", p->type );
	break;
    }
}

int db_add_arc(DB_TAB *cell, int layer, OPTS *opts, NUM x1,NUM y1,NUM x2,NUM y2,NUM x3,NUM y3) 
{
    struct db_arc *ap;
    struct db_deflist *dp;

    ap = (struct db_arc *) emalloc(sizeof(struct db_arc));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;
    dp->prev = NULL;

    dp->u.a = ap;
    dp->type = ARC;

    ap->layer=layer;
    ap->opts=opts;
    ap->x1=x1;
    ap->y1=y1;
    ap->x2=x2;
    ap->y2=y2;
    ap->x3=x3;
    ap->y3=y3;

    db_insert_component(cell,dp);
    return(0);
}

int db_add_circ(DB_TAB *cell, int layer, OPTS *opts, NUM x1,NUM y1,NUM x2,NUM y2)
{
    struct db_circ *cp;
    struct db_deflist *dp;
 
    cp = (struct db_circ *) emalloc(sizeof(struct db_circ));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;
    dp->prev = NULL;

    dp->u.c = cp;
    dp->type = CIRC;

    cp->layer=layer;
    cp->opts=opts;
    cp->x1=x1;
    cp->y1=y1;
    cp->x2=x2;
    cp->y2=y2;

    db_insert_component(cell,dp);
    return(0);
}

int db_add_line(DB_TAB *cell, int layer, OPTS *opts, COORDS *coords)
{
    struct db_line *lp;
    struct db_deflist *dp;

    lp = (struct db_line *) emalloc(sizeof(struct db_line));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;
    dp->prev = NULL;
    dp->u.l = lp;
    dp->type = LINE;
    lp->layer=layer;
    lp->opts=opts;
    lp->coords=coords;

    db_insert_component(cell,dp);
    return(0);
}

int db_add_oval(DB_TAB *cell, int layer, OPTS *opts, NUM x1,NUM y1,NUM x2,NUM y2,NUM x3,NUM y3) 
{
    struct db_oval *op;
    struct db_deflist *dp;

    op = (struct db_oval *) emalloc(sizeof(struct db_oval));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;
    dp->prev = NULL;

    dp->u.o = op;
    dp->type = OVAL;

    op->layer=layer;
    op->opts=opts;
    op->x1=x1;		/* first foci */
    op->y1=y1;
    op->x2=x2;		/* second foci */
    op->y2=y2;
    op->x3=x3;		/* point on curve */
    op->y3=y3;

    db_insert_component(cell,dp);
    return(0);
}

int db_add_poly(DB_TAB *cell, int layer, OPTS *opts, COORDS *coords)
{
    struct db_poly *pp;
    struct db_deflist *dp;
    int debug = 0;
 
    pp = (struct db_poly *) emalloc(sizeof(struct db_poly));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;
    dp->prev = NULL;

    dp->u.p = pp;
    dp->type = POLY;

    if (debug) printf("db_add_poly: setting poly layer %d\n", layer);
    pp->layer=layer;
    pp->opts=opts;
    pp->coords=coords;

    db_insert_component(cell,dp);
    return(0);
}

int db_add_rect(DB_TAB *cell, int layer, OPTS *opts, NUM x1,NUM y1,NUM x2,NUM y2)
{
    struct db_rect *rp;
    struct db_deflist *dp;

    rp = (struct db_rect *) emalloc(sizeof(struct db_rect));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;
    dp->prev = NULL;

    dp->u.r = rp;
    dp->type = RECT;

    rp->layer=layer;
    rp->x1=x1;
    rp->y1=y1;
    rp->x2=x2;
    rp->y2=y2;
    rp->opts = opts;

    db_insert_component(cell,dp);
    return(0);
}

int db_add_note(DB_TAB *cell, int layer, OPTS *opts, char *string ,NUM x,NUM y) 
{
    struct db_note *np;
    struct db_deflist *dp;
 
    np = (struct db_note *) emalloc(sizeof(struct db_note));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;
    dp->prev = NULL;

    dp->u.n = np;
    dp->type = NOTE;

    np->layer=layer;
    np->text=string;
    np->opts=opts;
    np->x=x;
    np->y=y;

    db_insert_component(cell,dp);
    return(0);
}

int db_add_text(DB_TAB *cell, int layer, OPTS *opts, char *string , NUM x, NUM y) 
{
    struct db_text *tp;
    struct db_deflist *dp;
 
    tp = (struct db_text *) emalloc(sizeof(struct db_text));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;
    dp->prev = NULL;

    dp->u.t = tp;
    dp->type = TEXT;

    tp->layer=layer;
    tp->text=string;
    tp->opts=opts;
    tp->x=x;
    tp->y=y;

    db_insert_component(cell,dp);
    return(0);
}

struct db_inst * db_add_inst(DB_TAB *cell, DB_TAB *subcell, OPTS *opts, NUM x, NUM y)
{
    struct db_inst *ip;
    struct db_deflist *dp;
 
    ip = (struct db_inst *) emalloc(sizeof(struct db_inst));
    dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
    dp->next = NULL;
    dp->prev = NULL;

    dp->u.i = ip;
    dp->type = INST;

    /* ip->def=subcell; */
    ip->name=strsave(subcell->name);

    ip->x=x;
    ip->y=y;
    ip->colx=x;
    ip->coly=y;
    ip->rowx=x;
    ip->rowy=y;
    ip->opts=opts;
    db_insert_component(cell,dp);
    return(ip);
}

/*
main () { 	

    DB_TAB *currep, *oldrep;

    currep = db_install("1foo");
    currep = db_install("2xyz");
	db_add_rect(currep,1,2.0,3.0,4.0,5.0);
	db_add_rect(currep,6,7.0,8.0,9.0,10.0);
    oldrep = currep;
    currep = db_install("3abc");
	db_add_rect(currep,11,12.0,13.0,14.0,15.0);
	db_add_inst(currep,oldrep,16.0,17.0);
    db_print();
}
*/

static double font_size = 10.0;
static double font_slant = 0.0;

void db_set_font_size(double size) { /* set default from FSIze command */
    char buf[16];
    sprintf(buf, "%4.4f", size);
    EVset("PIG_FONT_SIZE", buf);               /* grid color */
    font_size = size;
}

double db_get_font_size() { 	 /* get default from FSIze command */
    char *p;

    if ((p=EVget("PIG_FONT_SIZE")) != NULL) {
        if (sscanf(p,"%lg", &font_size) <= 0) {
                printf("bad PIG_FONT_SIZE=\"%s\"\n",p);
        }
    }
    return font_size;
}

void db_set_text_slant(double slant) {  /* get default from TSLant command */
    char buf[16];
    sprintf(buf, "%4.4f", slant);
    EVset("PIG_FONT_SLANT", buf);               /* font slant */
    font_slant = slant;
}

double db_get_text_slant() { 	 /* get default from TSLant command */
    char *p;

    if ((p=EVget("PIG_FONT_SLANT")) != NULL) {
        if (sscanf(p,"%lg", &font_slant) <= 0) {
                printf("bad PIG_FONT_SLANT=\"%s\"\n",p);
        }
    }
    return font_slant;
}


OPTS *opt_copy(OPTS *opts)
{
    OPTS *tmp;
    tmp = (OPTS *) emalloc(sizeof(OPTS));
    
    /* set defaults */
    tmp->font_size = opts->font_size;
    tmp->font_num = opts->font_num;
    tmp->height = opts->height;
    tmp->justification = opts->justification;
    tmp->stepflag = opts->stepflag;
    tmp->rows = opts->rows;
    tmp->cols = opts->cols;
    tmp->mirror = opts->mirror;
    tmp->rotation = opts->rotation;
    tmp->width = opts->width;
    tmp->aspect_ratio = opts->aspect_ratio;
    tmp->scale = opts->scale;
    tmp->slant = opts->slant;
    tmp->cname = strsave(opts->cname);
    tmp->sname = strsave(opts->sname);

    return(tmp);
}     

void opt_set_defaults(OPTS *opts)
{
    opts->font_size = 10.0;       /* :F<font_size> */
    opts->height = 0.0;       	  /* :H<substrate_height> */
    opts->mirror = MIRROR_OFF;    /* :M<x,xy,y>    */
    opts->font_num=0;		  /* :N<font_num> */
    opts->justification=0;	  /* :J<justification> */
    opts->stepflag=0;		  /* not a stepped instance */
    opts->rows=1;		  /* :S<rows>,<cols> */
    opts->cols=1;
    opts->rotation = 0.0;         /* :R<rotation,resolution> */
    opts->width = 0.0;            /* :W<width> */
    opts->aspect_ratio = 1.0;     /* :Y<yx_aspect_ratio> */
    opts->scale=1.0;              /* :X<scale> */
    opts->slant=0.0;              /* :Z<slant_degrees> */
    opts->cname= (char *) NULL;   /* component name */
    opts->sname= (char *) NULL;   /* signal name */
}

OPTS *opt_create()
{
    OPTS *tmp;
    tmp = (OPTS *) emalloc(sizeof(OPTS));
    
    opt_set_defaults(tmp);

    return(tmp);
}     

void db_print_opts(fp, popt, validopts) /* print options */
FILE *fp;
OPTS *popt;
/* validopts is a string which sets which options are printed out */
/* an example for a line is "W", text uses "MNRYZF" */
char *validopts;
{
    char *p;

    if (popt == NULL) {
        return;
    }

    if (popt->cname != NULL) {
    	fprintf(fp, "%s ", popt->cname);
    }

    if (popt->sname != NULL) {
    	fprintf(fp, "%s ", popt->sname);
    }

    for (p=validopts; *p != '\0'; p++) {
    	switch(toupper(*p)) {
	    case 'J':
		if (popt->justification != 0) {
		    fprintf(fp, ":J%d ", popt->justification);
		}
	    	break;
	    case 'H':
		if (popt->height != 0.0) {
		    fprintf(fp, ":H%g ", popt->height);
		}
	    	break;
	    case 'F':
		fprintf(fp, ":F%g ", popt->font_size);
	    	break;
	    case 'M':
		switch(popt->mirror) {
		    case MIRROR_OFF:
		    	break;
		    case MIRROR_X:
	    		fprintf(fp, ":MX ");
			break;
		    case MIRROR_Y:
	    		fprintf(fp, ":MY ");
			break;
		    case MIRROR_XY:
	    		fprintf(fp, ":MXY ");
			break;
		    default:
	    		weprintf("invalid mirror case\n");
		    	break;	
		}
	    	break;
	    case 'N':
		if (popt->font_num != 0 && popt->font_num != 1) {
		    fprintf(fp, ":N%d ", popt->font_num);
		}
	    	break;
	    case 'R':
		if (popt->rotation != 0.0) {
		    fprintf(fp, ":R%g ", popt->rotation);
		}
	    	break;
	    case 'S':
		if (popt->stepflag) {
		    fprintf(fp, ":S%d,%d ", popt->cols, popt->rows);
		}
	    	break;
	    case 'W':
		if (popt->width != 0.0) {
		    fprintf(fp, ":W%g ", popt->width);
		}
	    	break;
	    case 'X':
		if (popt->scale != 1.0) {
		    fprintf(fp, ":X%g ", popt->scale);
		}
	    	break;
	    case 'Y':
		if (popt->aspect_ratio != 1.0) {
		    fprintf(fp, ":Y%g ", popt->aspect_ratio);
		}
	    	break;
	    case 'Z':
		if (popt->slant != 0.0) {
		    fprintf(fp, ":Z%g ", popt->slant);
		}
	    	break;
	    default:
		weprintf("invalid option %c\n", *p); 
	    	break;
	}
    }
}


char *strsave(char *s)   /* save string s somewhere */
{
    char *p;

    if (s == NULL) {
        return(s);
    }

    if ((p = (char *) emalloc(strlen(s)+1)) != NULL)
	strcpy(p,s);
    return(p);
}

/******************** plot geometries *********************/

/* ADD Amask [.cname] [@sname] [:Wwidth] [:Rres] coord coord coord */ 
void do_arc(DB_DEFLIST *def, BOUNDS *bb, int mode)
{
    NUM x1,y1,x2,y2,x3,y3;
    int res;
    double seg;
    double nseg;
    double dtheta3, theta1, dtheta2, theta;
    double q, r, x0, y0;
    int debug=0;

    COORDS *cp;
    DB_DEFLIST *dp;
    struct db_line *lp;

    x1=def->u.a->x1;	/* #1 end point */
    y1=def->u.a->y1;

    x2=def->u.a->x2;	/* #2 end point */
    y2=def->u.a->y2;

    x3=def->u.a->x3;	/* point on curve */
    y3=def->u.a->y3;

    jump(bb,mode); set_layer(def->u.a->layer, ARC);

    if ((def->u.a->opts->rotation) != 0.0) {
	res = (int) (360.0/def->u.a->opts->rotation);	/* resolution */
    } else {
    	res = 64;
    }

    if( def->u.a->opts->width != 0.0) {
        /* render with width using the line drawing routine */
	dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
	dp->next = NULL;
	dp->prev = NULL;
	dp->type = LINE;
        lp = (struct db_line *) emalloc(sizeof(struct db_line));
	dp->u.l = lp;
	dp->u.l->layer=def->u.a->layer;
	dp->u.l->opts=opt_create();
        dp->u.l->opts->width = def->u.a->opts->width;
    }

    /* check to see if (x1,y1), (x2,y2), (x3,y3) are colinear,
       because if they are then our arc algorithm blows up, 
       use a simple line segment instead.

    	does (x1,y1) + alpha*(x2-x1, y2-y1) = (x3,y3) ?

	alphax*(x2-x1) + x1 = x3
	alphax = (x3-x1)/(x2-x1)
	alphay = (y3-y1)/(y2-y1)

	colinear iff:

	(x3-x1)/(x2-x1) = (y3-y1)/(y2-y1)

	rearrange to avoid divbyzero:

	collinear iff ((x3-x1)*(y2-y1) - (y3-y1)*(x2-x1) == 0)

	note: it should be OK to test for exact equality here because
	this test uses only subtraction and multiplication, and
	the points have already been snapped to integer multiples
	of the resolution setting.
    */

    if (((x3-x1)*(y2-y1) - (y3-y1)*(x2-x1) == 0)) { 	/* colinear ? */

	/* draw straight line from x1,y1 to x2,y2 */

 	if( def->u.a->opts->width == 0.0) {
	    draw(x1, y1, bb, mode);
	    draw(x2, y2, bb, mode);
	} else {
	    cp = coord_new(x1, y1);
	    dp->u.l->coords=cp;
	    coord_append(cp, x2, y2);
	}

    } else {	/* otherwise compute arc */

	/*
	Given two points:  p1:x1,y1 p2:x2,y2 at the ends of a circular arc and
	a third point p3:x3,y3 on the arc itself, then find the center of
	the circle and the radius.

	Note that a perpendicular from the center of the line segment
	drawn between p1 and p2, will intersect the perpendicular extended
	from the center of the line segment p2p3 at the center of the circle.

	Center points:
	    { (x1+x2)/2 , (y1+y2)/2 }
	    { (x2+x3)/2 , (y3+y3)/2 }

	Perpendiculars are constructed by computing dx and dy and then taking
	these as the vector { -dy, dx }.

	So the equation for the center of the circle is found by finding P,Q
	such that:
	    { (x1+x2)/2 , (y1+y2)/2 } + P * { y1-y2, x2-x1 } =
	    { (x2+x3)/2 , (y2+y3)/2 } + Q * { y2-y3, x3-x2 }
	*/

	q = ((y1-y2)*((y2+y3)/2 - (y1+y2)/2) - (x2-x1)*((x2+x3)/2 - (x1+x2)/2));
	q /= ((y2-y3)*(x2-x1) - (x3-x2)*(y1-y2));

	x0 = (x2+x3)/2 + q*(y2-y3);
	y0 = (y2+y3)/2 + q*(x3-x2);   

	r = sqrt(pow((x3-x0),2.0)  +pow((y3-y0),2.0));

	theta1 = atan2(y1-y0, x1-x0);
	dtheta2 = atan2(y2-y0, x2-x0) - theta1;
	dtheta3 = atan2(y3-y0, x3-x0) - theta1;

	if (dtheta2 < 0.0) {
	    dtheta2 += 2.0*M_PI;
	}
	if (dtheta3 < 0.0) {
	    dtheta3 += 2.0*M_PI;
	}

	if ((dtheta3 > dtheta2)) {
	    nseg = res-fabs(floor(res*(dtheta2)/(2.0*M_PI)));
	    if (debug) printf("YY t1=%g, dt2=%g, dt3=%g, nseg=%g\n", 
		    theta1, dtheta2, dtheta3, nseg);
	} else {
	    nseg = fabs(floor(res*(dtheta2)/(2.0*M_PI)));
	    if (debug) printf("XX t1=%g, dt2=%g, dt3=%g, nseg=%g\n", 
		    theta1, dtheta2, dtheta3, nseg);
	}
	if (debug) fflush(stdout);

	for (seg=0; seg<=nseg; seg++) {
	    if ((dtheta3 > dtheta2)) {
		theta=dtheta2+theta1+seg*(2.0*M_PI-dtheta2)/nseg; 
	    } else {
		theta=theta1+seg*(dtheta2)/nseg; 
	    }
	    if( def->u.a->opts->width == 0.0) {
		draw(x0+r*cos(theta), y0+r*sin(theta), bb, mode);
	    } else {
		if (seg==0) {
		    cp = coord_new(x0+r*cos(theta), y0+r*sin(theta));
		    dp->u.l->coords=cp;
		} else {
		    coord_append(cp, x0+r*cos(theta), y0+r*sin(theta));
		}
	    }
	}
    } 
    
    if( def->u.a->opts->width != 0.0) {
    	do_line(dp, bb, mode);		/* draw it */
	db_free_component(dp);		/* now destroy it */
    }
}

/* ADD Cmask [.cname] [@sname] [:Yyxratio] [:Wwidth] [:Rres] coord coord */
// int mode;		/* drawing mode */

void do_circ(DB_DEFLIST *def, BOUNDS *bb, int mode)
{
    int i;
    double r1, r2,theta,x,y,x1,y1,x2,y2;
    double theta_start;
    double radius;
    double width;
    int res;

    if ((def->u.c->opts->rotation) != 0.0) {
	res = (int) (360.0/def->u.c->opts->rotation);	/* resolution */
    } else {
    	res = 64;
    }

    /* printf("# rendering circle\n"); */

    x1 = (double) def->u.c->x1; 	/* center point */
    y1 = (double) def->u.c->y1;

    x2 = (double) def->u.c->x2;	/* point on circumference */
    y2 = (double) def->u.c->y2;

    /* if (mode==D_RUBBER) { xwin_draw_circle(x1,y1); xwin_draw_circle(x2,y2); } */
    
    jump(bb,mode); set_layer(def->u.c->layer, CIRC);

    x = (double) (x2-x1);
    y = (double) (y2-y1);

    radius = sqrt(x*x+y*y);

    theta_start = atan2(x,y);
    startpoly(bb,mode);

    width=def->u.c->opts->width;

    if(width == 0.0) {

	/* this next snippet of code works on the realization that an oval */
	/* is really just the vector sum of two counter-rotating phasors */
	/* of different lengths.  If r1=r and r2=0, you get a circle.  If you */
	/* solve the math for r1, r2 as a function of major/minor axis widths */
	/* you get the following.  If :Y<aspect_ratio> == 0.0, then the following */
	/* defaults to a perfectly round circle... */

	r1 = (1.0+def->u.c->opts->aspect_ratio)*radius/2.0;
	r2 = (1.0-def->u.c->opts->aspect_ratio)*radius/2.0;

	for (i=0; i<=res; i++) {	
	    theta = ((double) i)*2.0*M_PI/((double) res);
	    x = r1*sin(theta_start-theta) + r2*sin(theta_start+theta);
	    y = r1*cos(theta_start-theta) + r2*cos(theta_start+theta);
	    draw(x1+x, y1+y, bb, mode);
	}
    } else {
	r1 = (1.0+def->u.c->opts->aspect_ratio)*radius/2.0+width/2.0;
	r2 = (1.0-def->u.c->opts->aspect_ratio)*radius/2.0;
	for (i=0; i<=res; i++) {
	    theta = ((double) i)*2.0*M_PI/((double) res);
	    x = r1*sin(theta_start-theta) + r2*sin(theta_start+theta);
	    y = r1*cos(theta_start-theta) + r2*cos(theta_start+theta);
	    draw(x1+x, y1+y, bb, mode);
	}
	r1 = (1.0+def->u.c->opts->aspect_ratio)*radius/2.0-width/2.0;
	r2 = (1.0-def->u.c->opts->aspect_ratio)*radius/2.0;
	for (i=res; i>=0; i--) {
	    theta = ((double) i)*2.0*M_PI/((double) res);
	    x = r1*sin(theta_start-theta) + r2*sin(theta_start+theta);
	    y = r1*cos(theta_start-theta) + r2*cos(theta_start+theta);
	    draw(x1+x, y1+y, bb, mode);
	}
    }
    endpoly(bb,mode);	
}

/*
r1 + r2 = r+w
r1 - r2 = ar+w

2r1 = (1+a)r + 2w
2r2 = (1-a)r
*/

/* ADD Cmask [.cname] [@sname] [:Yyxratio] [:Wwidth] [:Rres] coord coord */
// int mode;		/* drawing mode */

void do_circ_orig(DB_DEFLIST *def, BOUNDS *bb, int mode)
{
    int i;
    double r1, r2,theta,x,y,x1,y1,x2,y2;
    double theta_start;
    int res;

    COORDS *cp;
    DB_DEFLIST *dp;
    struct db_line *lp;

    if ((def->u.c->opts->rotation) != 0.0) {
	res = (int) (360.0/def->u.c->opts->rotation);	/* resolution */
    } else {
    	res = 64;
    }

    /* printf("# rendering circle\n"); */

    x1 = (double) def->u.c->x1; 	/* center point */
    y1 = (double) def->u.c->y1;

    x2 = (double) def->u.c->x2;	/* point on circumference */
    y2 = (double) def->u.c->y2;

    /* if (mode==D_RUBBER) { xwin_draw_circle(x1,y1); xwin_draw_circle(x2,y2); } */
    
    jump(bb,mode); set_layer(def->u.c->layer, CIRC);

    x = (double) (x2-x1);
    y = (double) (y2-y1);

    /* this next snippet of code works on the realization that an oval */
    /* is really just the vector sum of two counter-rotating phasors */
    /* of different lengths.  If r1=r and r2=0, you get a circle.  If you */
    /* solve the math for r1, r2 as a function of major/minor axis widths */
    /* you get the following.  If :Y<aspect_ratio> is unset, then the following */
    /* defaults to a perfectly round circle... */

    r1 = (1.0+def->u.c->opts->aspect_ratio)*sqrt(x*x+y*y)/2.0;
    r2 = (1.0-def->u.c->opts->aspect_ratio)*sqrt(x*x+y*y)/2.0;

    theta_start = atan2(x,y);
    if( def->u.c->opts->width != 0.0) {
        /* render a circle with width using the line drawing routine */
	dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
	dp->next = NULL;
	dp->prev = NULL;
	dp->type = LINE;
        lp = (struct db_line *) emalloc(sizeof(struct db_line));
	dp->u.l = lp;
	dp->u.l->layer=def->u.c->layer;
	dp->u.l->opts=opt_create();
        dp->u.l->opts->width = def->u.c->opts->width;
    } else {
	startpoly(bb,mode);
    }

    for (i=0; i<=res+1; i++) {				/* +1 to fill the gap */
	theta = ((double) i)*2.0*M_PI/((double) res);
	x = r1*sin(theta_start-theta) + r2*sin(theta_start+theta);
	y = r1*cos(theta_start-theta) + r2*cos(theta_start+theta);
	if( def->u.c->opts->width == 0.0) {
	    draw(x1+x, y1+y, bb, mode);
	} else {
	    if (i==0) {
	    	cp = coord_new(x1+x, y1+y);
		dp->u.l->coords=cp;
	    } else {
		coord_append(cp, x1+x, y1+y);
	    }
	}
    }
    if( def->u.c->opts->width != 0.0) {
    	do_line(dp, bb, mode);		/* draw it */
	db_free_component(dp);		/* now destroy it */
    } else {
        endpoly(bb,mode);	
    }
}

/* --------------------------------------------------------- */

/* quick and dirty stack for saving points.  Built-in 1024 */
/* point limitation.  FIXME: change to linked list with no */
/* size limit */

static int n=0;
typedef struct cc {
    double x;
    double y;
} CC; 

CC coordstack[1024];

void clear() {
   n=0;
}

void push(double x, double y) {
   coordstack[n].x = x;
   coordstack[n].y = y;
   n++;
}

int pop(double *x, double *y) {
   if (n>0) {
       n--;
       *x = coordstack[n].x;
       *y = coordstack[n].y;
       return 1;
   } else {
       return 0;
   }
}

// given two vectors dx,dy and dxn,dyn
// return the relative angle

int rightturn(double dx,double dy,double dxn,double dyn) {
   return((dy*dxn - dx*dyn)>0);
}

/* --------------------------------------------------------- */
/* ADD Lmask [.cname] [@sname] [:Wwidth] coord coord [coord ...] */
// int mode; 	/* drawing mode */

void do_line(DB_DEFLIST *def, BOUNDS *bb, int mode)
{
    COORDS *temp;

    double x1,y1,x2,y2,x3,y3,dx,dy,a;
    double xa,ya,xb,yb,xc,yc,xd,yd;
    double xx, yy;
    double k,dxn,dyn;
    double width=0.0;
    double height=0.0;
    double d,x,aa;
    int segment;
    int debug=0;	/* set to 1 for copious debug output */

    width = def->u.l->opts->width;
    height = def->u.l->opts->height;

    // compute mitre parameters
    if (height > 0.0) { 
	d = width*sqrt(2.0);
	x = d*(0.52 + 0.65*exp(-1.35*(width/height)));
	// this aa factor differs from the
	// paper for ease of implementation.  aa is
	// defined from the natural end of the line
	// rather than the inner intersection.  the
	// final results are identical.
	aa = (x-d/2.0)*sqrt(2.0)+(width/2.0);
	if (debug) printf("aa=%g\n",aa);
    }

    dx = dy = dxn  = dyn = 0.0;

    /* there are four cases for rendering lines with miters 
     *
     * "===" is rendered segment, "|" is a square end,
     * "/" is a mitered end, "----" is a non-rendered segment.
     * 
     * 1) single segment: 
     *      (temp->next == NULL)
     * 
     *	    |=====|
     *
     * 2) first segment, but more than one
     *      (temp->next != NULL)
     *
     *	    |=====/-----/...
     *
     * 3) middle segment bracketed by active segments:
     *      (temp->next != NULL)
     *
     *      -----/======/-----
     *
     * 4) final segment, preceeded by an active segment:
     *      (temp->next == NULL)
     *      -----/======|
     */

    /* printf("# rendering line\n"); */
    jump(bb,mode);
    set_layer(def->u.l->layer, LINE);
    if (width != 0.0) {
        startpoly(bb,mode);
	clear();
    }

    temp = def->u.l->coords;
    x2=temp->coord.x;
    y2=temp->coord.y;
    temp = temp->next;

    /* if (mode==D_RUBBER) { xwin_draw_circle(x2,y2); } */

    segment = 0;
    if (temp!=NULL) {
	do {
	    x1=x2; x2=temp->coord.x;
	    y1=y2; y2=temp->coord.y;

	    /* if (mode==D_RUBBER) { xwin_draw_circle(x2,y2); } */

	    if (width != 0.0) {
		if (segment == 0) {
		    dx = x2-x1; 
		    dy = y2-y1;
		    a = 1.0/(2.0*sqrt(dx*dx+dy*dy));
		    if (debug) printf("# in 1, a=%g\n",a);
		    dx *= a; 
		    dy *= a;
		} else {
		    if (debug) printf("# in 2\n"); 
		    dx=dxn;
		    dy=dyn;
		}

		if ( temp->next != NULL ) {
		    x3=temp->next->coord.x;
		    y3=temp->next->coord.y;
		    dxn = x3-x2; 
		    dyn = y3-y2;

		    /* prevent a singularity if the last */
		    /* rubber band segment has zero length */
		    if (dxn==0 && dyn==0) {
			a = 0;
		    } else {
			a = 1.0/(2.0*sqrt(dxn*dxn+dyn*dyn));
		    }

		    if (debug) printf("# in 3, a=%g\n",a); 
		    dxn *= a; 
		    dyn *= a;
		}
		
		//------------------------------------------------
		if ( temp->next == NULL && segment == 0) {
		    // just a square cap, only two points

		    if (debug) printf("# in 4\n"); 
		    xa = x1+dy*width; ya = y1-dx*width;
		    xb = x2+dy*width; yb = y2-dx*width;
		    xc = x2-dy*width; yc = y2+dx*width;
		    xd = x1-dy*width; yd = y1+dx*width;
		    draw(xd, yd, bb, mode);
		    draw(xc, yc, bb, mode);
		    push(xa, ya);
		    push(xb, yb);

		//------------------------------------------------
		} else if (temp->next != NULL) {  
		    // mid span: do a mitre or intersection

		    if (segment == 0) {
			// establish starting coordinates
			xa = x1+dy*width; ya = y1-dx*width;
			xd = x1-dy*width; yd = y1+dx*width;
			draw(xd, yd, bb, mode);
			push(xa, ya);
		    }

		    if (debug) printf("# in 11\n");
		    if (fabs(dx+dxn) < EPS) {
			k = (dx - dxn)/(dyn + dy); 
			if (debug) printf("# in 12, k=%g\n",k);
		    } else {
			if (debug) printf("# in 13, k=%g\n",k); 
			k = (dyn - dy)/(dx + dxn);
		    }

		    /* check for co-linearity of segments */
		    if ((fabs(dx-dxn)<EPS && fabs(dy-dyn)<EPS) ||
			(fabs(dx+dxn)<EPS && fabs(dy+dyn)<EPS)) {
			if (debug) printf("# in 8\n"); 
			xb = x2+dy*width; yb = y2-dx*width;
			xc = x2-dy*width; yc = y2+dx*width;
		    } else { 
			if (debug) printf("# in 9\n"); 
			xb = x2+dy*width + k*dx*width;
			yb = y2-dx*width + k*dy*width;
			xc = x2-dy*width - k*dx*width; 
			yc = y2+dx*width - k*dy*width;
		    }

		    // RCW2
		    if (height != 0.0) {
		        if (rightturn(dx,dy,dxn,dyn)) {
			    xc = x2-dy*width-aa*dx*2.0;
			    yc = y2+dx*width-aa*dy*2.0;
			    draw(xc, yc, bb, mode);
			    xc = x2-dyn*width+aa*dxn*2.0;
			    yc = y2+dxn*width+aa*dyn*2.0;
			    draw(xc, yc, bb, mode);
			    push(xb, yb);
			} else {
			    xb = x2+dy*width-aa*dx*2.0;
			    yb = y2-dx*width-aa*dy*2.0;
			    push(xb, yb);
			    xb = x2+dyn*width+aa*dxn*2.0;
			    yb = y2-dxn*width+aa*dyn*2.0;
			    push(xb, yb);
			    draw(xc, yc, bb, mode);
			}
		    } else {
			draw(xc, yc, bb, mode);
			push(xb, yb);
			// draw(xd, yd, bb, mode);
			// push(xa, ya);
		    }

		//------------------------------------------------
		} else if ((temp->next == NULL && segment >= 0) ||
			  ( temp->next->coord.x == x2 && 
			    temp->next->coord.y == y2)) {
		    // end cap on line with more than one segment

		    if (debug) printf("# in 10\n");
		    xb = x2+dy*width; yb = y2-dx*width;
		    xc = x2-dy*width; yc = y2+dx*width;
		    // draw(xd, yd, bb, mode);
		    draw(xc, yc, bb, mode);
		    // push(xa, ya);
		    push(xb, yb);

		}
		//------------------------------------------------
	    /* 
	     *	    Each line segment with width:
	     *
	     *	    xy(a)---------xy(b)
	     *	    xy(d)---------xy(c)
	     *
	     *      We immmediately draw() (xd,yd) (xc,yc)
	     *      and push (xa,ya) (xb,yb) on a stack.
	     *
	     *      When finished with all segments, we pop
	     *      and draw() all the points on the stack
	     *      and finish the polygonal line with (xd,yd)
	     */

		/* printf("#dx=%g dy=%g dxn=%g dyn=%g\n",dx,dy,dxn,dyn); */
		/* check for co-linear reversal of path */
		if ((fabs(dx+dxn)<EPS && fabs(dy+dyn)<EPS)) {
		    if (debug) printf("# in 20\n"); 
		    xa = xc; ya = yc;
		    xd = xb; yd = yb;
		} else {
		    if (debug) printf("# in 21\n"); 
		    xa = xb; ya = yb;
		    xd = xc; yd = yc;
		}
	    } else {		/* width == 0 */
		jump(bb,mode);
		draw(x1,y1, bb, mode);
		draw(x2,y2, bb, mode);
		jump(bb,mode);
	    }

	    temp = temp->next;
	    segment++;

	} while(temp != NULL);

	if (width != 0.0) {
	    while (pop(&xx, &yy) != 0) {
		draw(xx,yy,bb,mode);
	    }
	    endpoly(bb,mode);
	}
    }
}

/* ADD Omask [.cname] [@sname] [:Wwidth] [:Rres] coord coord coord   */

void do_oval(DB_DEFLIST *def, BOUNDS *bb, int mode)
{
    // NUM x1,y1,x2,y2,x3,y3;

    /* printf("# rendering oval (not implemented)\n"); */

    // x1=def->u.o->x1;	/* focii #1 */
    // y1=def->u.o->y1;

    // x2=def->u.o->x2;	/* focii #2 */
    // y2=def->u.o->y2;

    // x3=def->u.o->x3;	/* point on curve */
    // y3=def->u.o->y3;

}

/* ADD Pmask [.cname] [@sname] [:Wwidth] coord coord coord [coord ...]  */ 

void do_poly(DB_DEFLIST *def, BOUNDS *bb, int mode)
{
    COORDS *temp;
    int debug=0;
    int npts=0;
    double x1, y1;
    double x2, y2;
    
    if (debug) printf("# rendering polygon\n"); 

    if (debug) printf("# setting pen %d\n", def->u.p->layer); 
    set_layer(def->u.p->layer, POLY);

    
    /* FIXME: should eventually check for closure here and probably */
    /* at initial entry of points into db.  If last point != first */
    /* point, then the polygon should be closed automatically and an */
    /* error message generated */

    temp = def->u.p->coords;
    x1=temp->coord.x; y1=temp->coord.y;

    /* if (mode==D_RUBBER) { xwin_draw_circle(x1,y1); } */

    jump(bb,mode);
    startpoly(bb,mode);
    while(temp != NULL) {
	x2=temp->coord.x; y2=temp->coord.y;

	/* if (mode==D_RUBBER) { xwin_draw_circle(x2,y2); } */

	if (npts && temp->prev->coord.x==x2 && temp->prev->coord.y==y2) {
	    ;
	} else {
	    npts++;
	    draw(x2, y2, bb, mode);
	}
	temp = temp->next;
    }
    
    /* when rubber banding, it is important not to redraw over a */
    /* line because it will erase it.  This can occur when only */
    /* two points have been drawn so far.  In this case, we should */
    /* not automatically close the polygon.  So we only execute the  */
    /* next bit of code when npts > 2 */

    if ((x1 != x2 || y1 != y2) && npts > 2) {
	/* if (mode==D_RUBBER) { xwin_draw_circle(x1,y1); } */
	draw(x1, y1, bb, mode);
    }
    endpoly(bb,mode);
}


/* ADD Rmask [.cname] [@sname] [:Wwidth] coord coord  */
// int mode;	/* drawing mode */

void do_rect(DB_DEFLIST *def, BOUNDS *bb, int mode)
{
    double x1,y1,x2,y2;
    COORDS *cp;
    DB_DEFLIST *dp;
    struct db_line *lp;

    /* printf("# rendering rectangle\n"); */

    x1 = (double) def->u.r->x1;
    y1 = (double) def->u.r->y1;
    x2 = (double) def->u.r->x2;
    y2 = (double) def->u.r->y2;
    
    jump(bb,mode);

    set_layer(def->u.r->layer, RECT);

    if (def->u.r->opts->width == 0.0) {
	startpoly(bb,mode);
	draw(x1, y1, bb, mode); draw(x1, y2, bb, mode);
	draw(x2, y2, bb, mode); draw(x2, y1, bb, mode);
	draw(x1, y1, bb, mode);
	endpoly(bb,mode);
	jump(bb,mode);
    } else { 
    
        /* render a rectangle with width using the line drawing routine */
	dp = (struct db_deflist *) emalloc(sizeof(struct db_deflist));
	dp->next = NULL;
	dp->prev = NULL;
	dp->type = LINE;
        lp = (struct db_line *) emalloc(sizeof(struct db_line));
	dp->u.l = lp;
	dp->u.l->layer=def->u.r->layer;
	dp->u.l->opts=opt_create();
        dp->u.l->opts->width = def->u.r->opts->width;

	cp = coord_new(x1, (y1+y2)/2.0);
	dp->u.l->coords=cp;

	coord_append(cp, x1, y2);
	coord_append(cp, x2, y2);
	coord_append(cp, x2, y1);
	coord_append(cp, x1, y1);
	coord_append(cp, x1, (y1+y2)/2.0);
    	do_line(dp, bb, mode);

	db_free_component(dp);		/* now destroy it */
    }

    /* if (mode==D_RUBBER) { xwin_draw_circle(x1,y1); xwin_draw_circle(x2,y2); } */
}


/* ADD Nmask [.cname] [:Mmirror] [:Rrot] [:Yyxratio]
 *             [:Zslant] [:Ffontsize] [:Nfontnum] "string" coord  
 */

void do_note(DB_DEFLIST *def, BOUNDS *bb, int mode)
{

    XFORM *xp;

    /* create a unit xform matrix */

    xp = (XFORM *) emalloc(sizeof(XFORM));  
    xp->r11 = 1.0; xp->r12 = 0.0; xp->r21 = 0.0;
    xp->r22 = 1.0; xp->dx  = 0.0; xp->dy  = 0.0;

    /* NOTE: To work properly, these transformations have to */
    /* occur in the proper order, for example, rotation must come */
    /* after slant transformation */

    switch (def->u.n->opts->mirror) {
	case MIRROR_OFF:
	    break;
	case MIRROR_X:
	    xp->r22 *= -1.0;
	    break;
	case MIRROR_Y:
	    xp->r11 *= -1.0;
	    break;
	case MIRROR_XY:
	    xp->r11 *= -1.0;
	    xp->r22 *= -1.0;
	    break;
    }

    mat_scale(xp, def->u.n->opts->font_size, def->u.n->opts->font_size);
    mat_scale(xp, 1.0/def->u.n->opts->aspect_ratio, 1.0);
    mat_slant(xp, def->u.n->opts->slant);
    mat_rotate(xp, def->u.n->opts->rotation);

    jump(bb,mode); set_layer(def->u.n->layer, NOTE);

    xp->dx += def->u.n->x;
    xp->dy += def->u.n->y;

    writestring(def->u.n->text, xp, def->u.n->opts->font_num,
    	def->u.n->opts->justification, bb, mode);

    free(xp);
}

/* ADD Tmask [.cname] [:Mmirror] [:Rrot] [:Yyxratio]
 *             [:Zslant] [:Ffontsize] [:Nfontnum] "string" coord  
 */

void do_text(DB_DEFLIST *def, BOUNDS *bb, int mode)
{

    XFORM *xp;

    /* create a unit xform matrix */

    xp = (XFORM *) emalloc(sizeof(XFORM));  
    xp->r11 = 1.0; xp->r12 = 0.0; xp->r21 = 0.0;
    xp->r22 = 1.0; xp->dx  = 0.0; xp->dy  = 0.0;

    /* NOTE: To work properly, these transformations have to */
    /* occur in the proper order, for example, rotation must come */
    /* after slant transformation or else it wont work properly */

    switch (def->u.t->opts->mirror) {
	case MIRROR_OFF:
	    break;
	case MIRROR_X:
	    xp->r22 *= -1.0;
	    break;
	case MIRROR_Y:
	    xp->r11 *= -1.0;
	    break;
	case MIRROR_XY:
	    xp->r11 *= -1.0;
	    xp->r22 *= -1.0;
	    break;
    }

    mat_scale(xp, def->u.t->opts->font_size, def->u.t->opts->font_size);
    mat_scale(xp, 1.0/def->u.t->opts->aspect_ratio, 1.0);
    mat_slant(xp, def->u.t->opts->slant);
    mat_rotate(xp, def->u.t->opts->rotation);

    jump(bb,mode); set_layer(def->u.t->layer, TEXT);

    xp->dx += def->u.t->x;
    xp->dy += def->u.t->y;

    writestring(def->u.t->text, xp, def->u.t->opts->font_num,
    	def->u.t->opts->justification, bb, mode);

    free(xp);
}

/***************** coordinate transformation utilities ************/

XFORM *compose(XFORM *xf1, XFORM *xf2)
{
     XFORM *xp;
     xp = (XFORM *) emalloc(sizeof(XFORM)); 

     xp->r11 = (xf1->r11 * xf2->r11) + (xf1->r12 * xf2->r21);
     xp->r12 = (xf1->r11 * xf2->r12) + (xf1->r12 * xf2->r22);
     xp->r21 = (xf1->r21 * xf2->r11) + (xf1->r22 * xf2->r21);
     xp->r22 = (xf1->r21 * xf2->r12) + (xf1->r22 * xf2->r22);
     xp->dx  = (xf1->dx  * xf2->r11) + (xf1->dy  * xf2->r21) + xf2->dx;
     xp->dy  = (xf1->dx  * xf2->r12) + (xf1->dy  * xf2->r22) + xf2->dy;

     return (xp);
}

/* in-place rotate a transform matrix by theta degrees */
void mat_rotate(XFORM *xp, double theta)
{
    double s,c,t;
    s=sin(2.0*M_PI*theta/360.0);
    c=cos(2.0*M_PI*theta/360.0);

    t = c*xp->r11 - s*xp->r12;
    xp->r12 = c*xp->r12 + s*xp->r11;
    xp->r11 = t;

    t = c*xp->r21 - s*xp->r22;
    xp->r22 = c*xp->r22 + s*xp->r21;
    xp->r21 = t;

    t = c*xp->dx - s*xp->dy;
    xp->dy = c*xp->dy + s*xp->dx;
    xp->dx = t;
}

/* in-place scale transform */
void mat_scale(XFORM *xp, double sx, double sy) 
{
    xp->r11 *= sx;
    xp->r12 *= sy;
    xp->r21 *= sx;
    xp->r22 *= sy;
    xp->dx  *= sx;
    xp->dy  *= sy;
}

/* in-place slant transform (for italics) */
void mat_slant(XFORM *xp, double theta) 
{
    double s,c;
    double a;

    int debug = 0;
    if (debug) printf("in mat_slant with theta=%g\n", theta);
    if (debug) mat_print(xp);

    s=sin(2.0*M_PI*theta/360.0);
    c=cos(2.0*M_PI*theta/360.0);
    a=s/c;

    xp->r21 += a*xp->r11;
    xp->r22 += a*xp->r12;

    if (debug) mat_print(xp);
}

void mat_print(XFORM *xa)
{
     printf("\n");
     printf("\t%g\t%g\t%g\n", xa->r11, xa->r12, 0.0);
     printf("\t%g\t%g\t%g\n", xa->r21, xa->r22, 0.0);
     printf("\t%g\t%g\t%g\n", xa->dx, xa->dy, 1.0);
}   


void show_list(DB_TAB *currep, int layer) 
{
    int i,j;

    if (currep == NULL) return;

    printf("\n");
    i=layer;
    printf("%d ",i);
    for (j=((sizeof(i)*8)-1); j>=0; j--) {
	if (getbits(currep->show[i], j, 1)) {
	    printf("1");
	} else {
	    printf("0");
	}
    }
    printf("\n");
}

/* get n bits from position p */ 

int getbits(unsigned int x, unsigned int p, unsigned int n)	
{
    return((x >> (p+1-n)) & ~(~0 << n));
}

void show_init(DB_TAB *currep) { /* set everyone visible, but RO */
    int i;

    if (currep == NULL) return;

    /* default is all visible, none modifiable. */
    for (i=0; i<MAX_LAYER; i++) {
    	currep->show[i]=0|ALL;  /* visible */
    }
}

void show_set_visible(DB_TAB *currep, int comp, int layer, int state) {

    int lstart, lstop, i;
    int debug=0;

    if (currep == NULL) return;

    if (layer==0) {
    	lstart=0;
	lstop=MAX_LAYER;
    } else {
    	lstart=layer;
	lstop=layer;
    }

    for (i=lstart; i<=lstop; i++) {
	if (state) {
	    currep->show[i] |= comp;
	} else {
	    currep->show[i] &= ~(comp);
	}
    }
     
    if (debug) printf("setting layer %d, comp %d, state %d db %d\n",
    	layer, comp, state,  show_check_visible(currep, comp, layer));
}

void show_set_modify(DB_TAB *currep, int comp, int layer, int state) {
    int lstart, lstop, i;
    int debug=0;

    if (currep == NULL) return;

    if (layer==0) {
    	lstart=0;
	lstop=MAX_LAYER;
    } else {
    	lstart=layer;
	lstop=layer;
    }

    for (i=lstart; i<=lstop; i++) {
	if (state) {
	    currep->show[i] |= (comp*2);
	} else {
	    currep->show[i] &= ~(comp*2);
	}
    }

    if (debug) printf("setting layer %d, comp %d, state %d db %d\n",
    	layer, comp, state,  show_check_modifiable(currep, comp, layer));
}

/* check modifiability */
int show_check_modifiable(DB_TAB *currep, int comp, int layer) {

    int debug=0;

    if (currep == NULL) return(0);

    if (debug) show_list(currep, layer);

    return (currep->show[layer] & (comp*2));
}

/* check visibility */
int show_check_visible(DB_TAB *currep, int comp, int layer) {

    int debug=0;
    if (debug) show_list(currep, layer);

    if (currep == NULL) return(1);

    if (debug) printf("checking vis, comp %d, layer %d, returning %d\n",
    	comp, layer, currep->show[layer] & comp);

    return (currep->show[layer] & (comp));
}

