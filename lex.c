#include <stdio.h>
#include <string.h>		/* for strchr() */
#include <ctype.h>		/* for toupper */

#include <readline/readline.h> 	/* for command line editing */
#include <readline/history.h>  

#include "rlgetc.h"
#include "db.h"
#include "token.h"
#include "xwin.h" 	
#include "lex.h"


char prompt[128];

/* The names of functions that actually do the manipulation. */

int com_add(), com_archive(), com_area(), com_background();
int com_bye(), com_change(), com_copy(), com_define();
int com_delete(), com_display(), com_distance(), com_edit();
int com_equate(), com_exit(), com_files(), com_grid();
int com_group(), com_help(),  com_identify();
int com_input(), com_interrupt(), com_layer(), com_level();
int com_list(), com_lock(), com_macro(), com_menu();
int com_move(), com_plot(), com_point(), com_process();
int com_purge(), com_retrieve(), com_save(), com_search();
int com_set(), com_shell(), com_show(), com_smash();
int com_split(), com_step(), com_stretch(), com_trace();
int com_undo(), com_units(), com_version(), com_window();
int com_wrap();

#define MAIN 0
#define PRO 1
#define SEA 2
#define MAC 3
#define EDI 4

int mode=MAIN;
/* A structure which contains information on the
   commands this program can understand. */

typedef struct {
    char *name;			/* User printable name of the function. */
    Function *func;		/* Function to call to do the job. */
    char *doc;			/* Documentation for this function.  */
} COMMAND;

COMMAND commands[] =
{
    {"add", com_add, "add a component to the current device"},
    {"archive", com_archive, "create an archive file of the specified device"},
    {"area", com_area, "calculate and display the area of selected component"},
    {"background", com_background, "specified device for background overlay"},
    {"bye", com_bye, "terminate edit session"},
    {"change", com_change, "change characteristics of selected components"},
    {"copy", com_copy, "copy a component from one location to another"},
    {"define", com_define, "define a macro"},
    {"delete", com_delete, "delete a component from the current device"},
    {"display", com_display, "turn the display on or off"},
    {"distance", com_distance, "measure the distance between two points"},
    {"edit", com_edit, "begin edit of an old or new device"},
    {"equate", com_equate, "define characteristics of a mask layer"},
    {"exit", com_exit, "leave an EDIT, PROCESS, or SEARCH subsystem"},
    {"files", com_files, "purge named files"},
    {"grid", com_grid, "set grid spacing or turn grid on/off"},
    {"group", com_group, "create a device from existing components"},
    {"help", com_help, "print syntax diagram for the specified command"},
    {"?", com_help, "Synonym for `help'"},
    {"identify", com_identify, "identify named instances or components"},
    {"input", com_input, "take command input from a file"},
    {"interrupt", com_interrupt, "interrupt an ADD to issue another command"},
    {"layer", com_layer, "set a default layer number"},
    {"level", com_level, "set the logical level of the current device"},
    {"list", com_list, "list information about the current environment"},
    {"lock", com_lock, "set the default lock angle"},
    {"macro", com_macro, "enter the MACRO subsystem"},
    {"menu", com_menu, "change or save the current menu"},
    {"move", com_move, "move a component from one location to another"},
    {"plot", com_plot, "make a plot of the current device"},
    {"point", com_point, "display the specified point on the screen"},
    {"process", com_process, "enter the PROCESS subsystem"},
    {"purge", com_purge, "remove device from memory and disk"},
    {"quit", com_bye, "terminate edit session"},
    {"retrieve", com_retrieve, "read commands from an ARCHIVE file"},
    {"save", com_save, "save the current file or device to disk"},
    {"search", com_search, "modify the search path"},
    {"set", com_set, "set environment variables"},
    {"shell", com_shell, "run a program from within the editor"},
    {"!", com_shell, "Synonym for `shell'"},
    {"show", com_show, "define which kinds of things to display"},
    {"smash", com_smash, "replace an instance with its components"},
    {"split", com_split, "cut a component into two halves"},
    {"step", com_step, "copy a component in an array fashion"},
    {"stretch", com_stretch, "make a component larger or smaller"},
    {"trace", com_trace, "highlight named signals"},
    {"undo", com_undo, "undo the last command"},
    {"units", com_units, "set editor resolution and user unit type"},
    {"version", com_version, "identify the version number of program"},
    {"window", com_window, "change the current window parameters"},
    {"wrap", com_wrap, "create a new device using existing components"},
    {(char *) NULL, (Function *) NULL, (char *) NULL}
};

static int numbyes=0;

main(argc,argv)
int argc;
char **argv;
{
    char word[128];
    char buf[128];
    int retcode;
    TOKEN token;
    COMMAND *command;
    COMMAND * find_command();
    int i; /* scratch variable */

    /* set program name for eprintf() error report package */
    setprogname(argv[0]);

    initX();

    loadfont("NOTEDATA.F");

    initialize_readline();
    rl_pending_input='\n';
    rl_setprompt("INIT> ");
    
    while((token=token_get(word)) != EOF) {
	switch (mode) {
	    case MAIN:
		rl_setprompt("MAIN> ");
		break;
	    case PRO:
		rl_setprompt("PROCESS> ");
		break;
	    case SEA:
		rl_setprompt("SEARCH> ");
		break;
	    case MAC:
		rl_setprompt("MACRO> ");
		break;
	    case EDI:
		if (currep != NULL) {
		    sprintf(buf, "EDIT %s> ", currep->name);
		    rl_setprompt(buf);
		} else {
		    rl_setprompt("EDIT> ");
		}
		break;
	    default:
		rl_setprompt("> ");
		break;
	}                           

	switch(token) {
	    case CMD:	/* find and call the command */
		command = find_command(word);
		if (command == NULL) {
		    printf(" bad command\n");
		    token_flush();
		} else {
		    retcode = ((*(command->func)) (""));
		}
		break;
	    case EOL:
	    case EOC:
		break;
	    default:
		printf(" expected COMMAND\n");
		token_flush();
		break;
	}
    }
}

/* Look up NAME as the name of a command, and return a pointer to that
   command.  Return a NULL pointer if NAME isn't a command name. */

COMMAND * find_command(name)
char *name;
{
    register int i, size;

    size = strlen(name);
    size = size > 2 ? size : 3;
    
    for (i = 0; commands[i].name; i++)
	if (strncasecmp(name, commands[i].name, size) == 0)
	    return (&commands[i]);

    return ((COMMAND *) NULL);
}

/* returns 1 if name found, 0 if not */
int lookup_command(name)
char *name;
{
    register int i, size;
 
    size = strlen(name);
    size = size > 2 ? size : 3;
 
    for (i = 0; commands[i].name; i++)
        if (strncasecmp(name, commands[i].name, size) == 0)
            return (1);
 
    return ((TOKEN) NULL);
}                            


/* **************************************************************** */
/*                                                                  */
/*                    Built-in   Commands                           */
/*                                                                  */
/* **************************************************************** */

/* 
    Add a component to the current device.
    This routine checks for <component[layer]> or <instance_name>
    and dispatches control the the appropriate add_<comp> subroutine
*/

com_add(arg)		
char *arg;	/* currently unused */
{
    char *line;
    TOKEN token;
    char word[BUFSIZE];
    char buf[BUFSIZE];
    int done=0;
    int nnum=0;
    int state;
    int retval;

    int layer;
    int comp;
    double x1,y1,x2,y2;
    int debug=0;

    /* check that we are editing a rep */
    if (currep == NULL ) {
	printf("must do \"EDIT <name>\" before ADD\n");
	token_flush();
	return(1);
    }

    rl_setprompt("ADD> ");

/* 
    To dispatch to the proper add_<comp> routine, we look 
    here for either a primitive indicator
    concatenated with an optional layer number:

	A[layer_num] ;(arc)
	C[layer_num] ;(circle)
	L[layer_num] ;(line)
	N[layer_num] ;(note)
	O[layer_num] ;(oval)
	P[layer_num] ;(poly)
	R[layer_num] ;(rectangle)
	T[layer_num] ;(text)

    or a instance name.  Instance names can be quoted, which
    allows the user to have instances which overlap the primitive
    namespace.  For example:  N7 is a note on layer seven, but
    "N7" is an instance call.

*/

    while(!done) {
	token = token_get(word);
	if (token == IDENT) { 	
	    /* check to see if is a valid comp descriptor */
  	    comp = toupper(word[0]);
	    retval = sscanf(&word[1], "%d", &layer);
	    if (debug) printf("layer=%d\n",layer);

	    if (index("ACLNOPRT", comp) && retval==1)  {
		switch (comp) {
		    case 'A':
			add_arc(&layer);
			break;
		    case 'C':
			add_circ(&layer);
			break;
		    case 'L':
			add_line(&layer);
			break;
		    case 'N':
			add_note(&layer);
			break;
		    case 'O':
			add_oval(&layer);
			break;
		    case 'P':
			add_poly(&layer);
			break;
		    case 'R':
			add_rect(&layer);
			break;
		    case 'T':
			add_text(&layer);
			break;
		    default:
			printf("invalid comp name\n");
			break;
		}
	    } else {  /* must be a identifier */
		printf("calling add_inst with %s\n", word);
		add_inst(word);
	    }
	} else if (token == QUOTE) {
		add_inst(word);
	} else if (token == CMD) { /* return control back to top */
	    token_unget(token, word);
	    done++;
	} else if (token == EOL) {
	    ; /* ignore */
	} else if (token == EOC) {
	    done++;
	} else {
	    printf("expected COMP/LAYER or INST name\n");
	    token_flush();
	    done++;
	}
    }
}

int add_arc(int *layer)
{
    rl_setprompt("ADD_ARC> ");
    printf("in add_arc (unimplemented)\n");
    token_flush();
    return(1);
}


int add_note(int *layer)
{
    rl_setprompt("ADD_NOTE> ");
    printf("in add_note (unimplemented)\n");
    token_flush();
    return(1);
}

int add_oval(int *layer)
{
    rl_setprompt("ADD_OVAL> ");
    printf("in add_oval (unimplemented)\n");
    token_flush();
    return(1);
}

int add_poly(int *layer)
{
    rl_setprompt("ADD_POLY> ");
    printf("in add_poly (unimplemented)\n");
    token_flush();
    return(1);
}


int add_text(int *layer)
{
    rl_setprompt("ADD_TEXT> ");
    printf("in add_text (unimplemented)\n");
    token_flush();
    return(1);
}

com_archive(arg)    /* create an archive file of the specified device */
char *arg;
{
    numbyes=0;
    need_redraw++; 
    int err;

    if (currep != NULL) {
	if (db_def_archive(currep)) {
	    printf("unable to archive %s\n", currep->name);
	    exit(-1);
	};
	printf("    archived %s\n", currep->name);
	modified = 0;
    } else {
	printf("error: not currently editing a cell\n");
    }    	

    /*
	if(<name> != curr_name() && name != NULL) {
	    if (exists_on_disk(name)) {
		if_ask("do you want to overwrite?",name)
		    writedb(currdev, name);
		    mark_unmodified(name);
	    } else {
	    	writedb(currdev,name)
		mark_unmodified(name);
	    }
	} else {
	    if (currrname == NULL) {
		prompt for new name or break
	    }
	    writedb(currdev, currname);
	    mark_unmodified(name);
	}
    */

    return (0);
}


com_area(arg)		/* display the area of selected component */
char *arg;
{
    printf("    com_area %s\n", arg);
    return (0);
}

com_background(arg)	/* specified device for background overlay */
char *arg;
{
    printf("    com_background %s\n", arg);
    return (0);
}


com_bye(arg)		/* terminate edit session */
char *arg;
{
    /* The user wishes to quit using this program */
    /* Just set DONE non-zero. */

    numbyes++;	/* number of com_bye() calls */

    if (currep != NULL && modified && numbyes==1) {
	printf("    you have an unsaved instance (%s)!\n", currep->name);
    } else {
	done = 1;
	fprintf(stderr, "    com_bye %s\n", arg);
	exit(1); 	/* for now just bail */
    }
    return (0);
}

com_change(arg)		/* change characteristics of selected components */
char *arg;
{
    printf("    com_change %s\n", arg);
    return (0);
}

com_copy(arg)		/* copy a component from one location to another */
char *arg;
{
    printf("    com_copy %s\n", arg);
    return (0);
}

com_define(arg)		/* define a macro */
char *arg;
{
    printf("    com_define %s\n", arg);
    return (0);
}

com_delete(arg)		/* delete a component from the current device */
char *arg;
{
    printf("    com_delete %s\n", arg);
    return (0);
}

com_display(arg)	/* turn the display on or off */
char *arg;
{
    printf("    com_display %s\n", arg);
    return (0);
}

com_distance(arg)	/* measure the distance between two points */
char *arg;
{
    printf("    com_distance %s\n", arg);
    return (0);
}

com_edit(arg)		/* begin edit of an old or new device */
char *arg;
{
    TOKEN token;
    int done=0;
    char buf[128];
    char name[128];
    int error=0;

    /* printf("    com_edit <%s>\n", arg); */

    if (mode == MAIN || (mode == EDI && !modified)) {

	/* check for a name provided */
	if (token=token_get(name) != IDENT) {
	    printf("    must provide a name: EDIT <name>\n");
	    return (0);
	} 

	/* printf("got %s\n", name); */

	mode = EDI;
    
	/* don't destroy it if it's already in memory */
	if ((currep=db_lookup(name)) == NULL) {
	    currep = db_install(name);
	} 
	need_redraw++;

	/* 
	if inpath(PATH, <device>) {
	    push=push_env(device);
	    read_in(device);
	} else {
	    push=push_env(NULL);
	}
	*/

    } else if (mode == EDI) {
	printf("    must SAVE current device before new EDIT\n");
    } else {
	printf("    must EXIT before entering EDIT subsystem\n");
    }
    return (0);
}

com_equate(arg)		/* define characteristics of a mask layer */
char *arg;
{
    printf("    com_equate %s\n", arg);
    return (0);
}

com_exit(arg)		/* leave an EDIT, PROCESS, SEARCH subsystem */
char *arg;
{
    printf("    com_exit %s\n", arg);
    if (modified) {
	printf("    must save before exiting\n");
    } else {
	mode = MAIN;
	currep = NULL;
	need_redraw++;
    }
    return (0);
}

com_files(arg)		/* purge named files */
char *arg;
{
    printf("    com_files %s\n", arg);
    return (0);
}


/* "GRID [ON | OFF] [:Ccolor] [spacing mult [xypnt]]" */
/* com_grid(STATE, color, spacing, mult, x, y) */

com_grid(arg)		/* change or redraw grid */
char *arg;
{
    TOKEN token;
    int done=0;
    char buf[128];
    char word[128];
    int gridcolor=0;
    GRIDSTATE grid_state = TOGGLE;	
    double pts[6] = {0.0, 0.0, 1.0, 1.0, 0.0, 0.0};
    int npts=0;
    int nopts=0;
    double tmp;
    int debug=0;

    buf[0]='\0';
    while(!done && (token=token_get(word)) != EOF) {
	switch(token) {
	    case IDENT: 	/* identifier */
		if (strncasecmp(word, "ON", 2) == 0) {
		    grid_state=ON;
		    xwin_grid_state(grid_state);
		} else if (strncasecmp(word, "OFF", 2) == 0) {
		    grid_state=OFF;
		    xwin_grid_state(grid_state);
		} else {
	    	     printf("bad argument to GRID: %s\n", word);
		}
		break;
	    case QUOTE: 	/* quoted string */
		break;
	    case OPT:		/* option */
		if (strncasecmp(word, ":C", 2) == 0) { /* set color */
		    if(sscanf(word+2, "%d", &gridcolor) != 1) {
		         weprintf("invalid GRID argument: %s\n", word+2);
			return(-1);
		    }
		    nopts++;
		    if (debug) printf("setting color: %d\n", (gridcolor%8)+1);
		    xwin_grid_color((gridcolor%8)+1);
		} else {
	    	    weprintf("bad option to GRID: %s\n", word);
		    return(-1);
		}
		break;
	    case END:		/* end of file */
		break;
	    case CMD:		/* command */
		/* token_unget(token, word); */
		break;
	    case NUMBER: 	/* number */
		if(sscanf(word, "%lf", &tmp) != 1) {
		    weprintf("invalid GRID argument: %s\n", word);
		    return(-1);
		}
		if (debug) printf("com_grid: point #%d: %g\n", npts, tmp);

		switch (npts) {
		    case 0:
		    case 1:
		    case 2:
		    case 3:
		    	if (tmp <= 0) {
			    weprintf("GRID STEPs, SKIPs must be positive integers\n");
			    return(-1);
			}
		    case 4:
		    case 5:
		    	break;
		    default:
			weprintf("too many numbers to GRID\n");
			return(-1);
		    	break;
		}

		pts[npts++] = tmp;
		break;
	    case EOL:		/* newline or carriage return */
	    case EOC:		/* end of command */
		done++;
		break;
	    case COMMA:		/* comma */
		/* ignore */
		break;
	    default:
		eprintf("bad case in com_grid");
		break;
	}
    }

    if (npts == 2 || npts == 4 || npts == 6 ) {
	if (debug) printf("setting grid ON\n");
	xwin_grid_state(ON);
	xwin_grid_pts(pts[0], pts[1], pts[2], pts[3], pts[4], pts[5]);
    } else if (npts == 0) { 	/* no points */
    	if( nopts == 0) { 	/* and no options */
	    if (debug) printf("no args, toggling grid %d\n", grid_state);
	    xwin_grid_state(grid_state);
    	}
    } else {
	printf("bad number of arguments\n");
	return(-1);
    }

    return (0);
}

com_group(arg)		/* create a device from existing components */
char *arg;
{
    printf("    com_group %s\n", arg);
    return (0);
}

com_identify(arg)	/* identify named instances or components */
char *arg;
{
    printf("    com_identify\n", arg);
    return (0);
}

com_input(arg)		/* take command input from a file */
char *arg;
{
    printf("    com_input\n", arg);
    return (0);
}

com_interrupt(arg)	/* interrupt an ADD to issue another command */
char *arg;
{
    printf("    com_interrupt\n", arg);
    return (0);
}

com_layer(arg)		/* set a default layer number */
char *arg;
{
    printf("    com_layer\n", arg);
    return (0);
}

com_level(arg)		/* set the logical level of the current device */
char *arg;
{
    printf("    com_level\n", arg);
    return (0);
}

com_list(arg)		/* list information about the current environment */
char *arg;
{
    printf("    com_list\n", arg);
    return (0);
}

com_lock(arg)		/* set the default lock angle */
char *arg;
{
    printf("    com_lock\n", arg);
    return (0);
    return (0);
}

com_macro(arg)		/* enter the MACRO subsystem */
char *arg;
{
    printf("    com_macro\n", arg);
    if (mode == MAIN) {
	mode = MAC;
    } else {
	printf("    must EXIT before entering MACRO subsystem\n");
    }
    return (0);
}

com_menu(arg)		/* change or save the current menu */
char *arg;
{
    printf("    com_menu\n", arg);
    return (0);
}

com_move(arg)		/* move a component from one location to another */
char *arg;
{
    printf("    com_move\n", arg);
    return (0);
}

com_plot(arg)		/* make a plot of the current device */
char *arg;
{
    printf("    com_plot\n", arg);
    return (0);
}

com_point(arg)		/* display the specified point on the screen */
char *arg;
{
    printf("    com_point\n", arg);
    return (0);
}

com_process(arg)	/* enter the PROCESS subsystem */
char *arg;
{
    printf("    com_process\n", arg);
    if (mode == MAIN) {
	mode = PRO;
    } else {
	printf("    must EXIT before entering PROCESS subsystem\n");
    }
    return (0);
}

com_purge(arg)		/* remove device from memory and disk */
char *arg;
{
    printf("    com_purge\n", arg);
    return (0);
}

com_retrieve(arg)	/* read commands from an ARCHIVE file */
char *arg;
{
    printf("    com_retrieve\n", arg);
    return (0);
}

com_save()		/* save the current file or device to disk */
{
    numbyes=0;
    need_redraw++; 
    int err;

    if (currep != NULL) {
	if (db_save(currep)) {
	    printf("unable to save %s\n", currep->name);
	};
	printf("    saved %s\n", currep->name);
	modified = 0;
    } else {
	printf("error: not currently editing a cell\n");
    }    	

    /*
	if(<name> != curr_name() && name != NULL) {
	    if (exists_on_disk(name)) {
		if_ask("do you want to overwrite?",name)
		    writedb(currdev, name);
		    mark_unmodified(name);
	    } else {
	    	writedb(currdev,name)
		mark_unmodified(name);
	    }
	} else {
	    if (currrname == NULL) {
		prompt for new name or break
	    }
	    writedb(currdev, currname);
	    mark_unmodified(name);
	}
    */

    return (0);
}

com_search(arg)		/* modify the search path */
char *arg;
{
    printf("    com_search\n", arg);

    if (mode == MAIN) {
	mode = SEA;
    } else {
	printf("    must EXIT before entering SEARCH subsystem\n");
    }
    return (0);
}

com_set(arg)		/* set environment variables */
char *arg;
{
    printf("    com_set\n", arg);
    return (0);
}

com_shell(arg)		/* run a program from within the editor */
char *arg;
{
    printf("    com_shell\n", arg);
    return (0);
}

com_show(arg)		/* define which kinds of things to display */
char *arg;
{
    printf("    com_show\n", arg);
    return (0);
}

com_smash(arg)		/* replace an instance with its components */
char *arg;
{
    printf("    com_smash\n", arg);
    return (0);
}

com_split(arg)		/* cut a component into two halves */
char *arg;
{
    printf("    com_split\n", arg);
    return (0);
}

com_step(arg)		/* copy a component in an array fashion */
char *arg;
{
    printf("    com_step\n", arg);
    return (0);
}

com_stretch(arg)	/* make a component larger or smaller */
char *arg;
{
    printf("    com_stretch\n", arg);
    return (0);
}

com_trace(arg)		/* highlight named signals */
char *arg;
{
    printf("    com_trace\n", arg);
    return (0);
}

com_undo(arg)		/* undo the last command */
char *arg;
{
    printf("    com_undo\n", arg);
    return (0);
}

com_units(arg)		/* set editor resolution and user unit type */
char *arg;
{
    printf("    com_units\n", arg);
    return (0);
}

com_version(arg)	/* identify the version number of program */
char *arg;
{
    printf("    com_version: %s\n", version);
    return (0);
}

com_window(arg)		/* change the current window parameters */
char *arg;
{
    double dx, dy, xmin, ymin, xmax, ymax, tmp;
    double  x1, y1, x2, y2;
    int n;
    TOKEN token;
    int done=0;
    char buf[128];
    char word[128];
    double scale=1.0;
    int debug=0;
    int fit=0;
    int nest=9;

    if (debug) printf("    com_window\n", arg);

    buf[0]='\0';
    while(!done && (token=token_get(word)) != EOF) {
	switch(token) {
	    case IDENT: 	/* identifier */
		token_unget(token, word); 
		done++;
		break;
	    case QUOTE: 	/* quoted string */
	    case OPT:		/* option */
		if (strncasecmp(word, ":F", 2) == 0) {
		     fit++;
		} else if (strncasecmp(word, ":X", 2) == 0) {
		    if(sscanf(word+2, "%lf", &scale) != 1) {
		        weprintf("WIN invalid scale argument: %s\n", word+2);
			return(-1);
		    }
		    if (scale < 0.0) {
		    	scale = 1.0/(-scale);
		    }
		    if (scale > 10.0 || scale < 0.10) {
		        weprintf("WIN: scale out of range %s\n", word+2);
			return (-1);
		    }
		} else if (strncasecmp(word, ":N", 2) == 0) {
		    if(sscanf(word+2, "%d", &nest) != 1) {
		        weprintf("WIN invalid nest level %s\n", word+2);
			return(-1);
		    }
		    db_set_nest(nest);
		} else {
	    	     printf("WIN: bad option: %s\n", word);
		}
		break;
	    case END:		/* end of file */
		break;
	    case CMD:		/* command */
		token_unget(token, word); 
		done++;
		break;
	    case NUMBER: 	/* number */
		strcat(buf,word);
		strcat(buf," ");
		break;
	    case EOC:		/* end of command */
		done++;
		break;
	    case EOL:		/* newline or carriage return */
	    case COMMA:		/* comma */
		break;
	    default:
		eprintf("bad case in com_window");
		break;
	}
    }
    if (debug) printf("xwin_window got %s\n",buf);

    n = sscanf(buf, "%lf %lf %lf %lf", &x1, &y1, &x2, &y2);

    xwin_window_get(&xmin, &ymin, &xmax, &ymax);

    if (n==2) {
	dx=(xmax-xmin);
	dy=(ymax-ymin);
	xmin=x1-dx/2.0;
	ymin=y1-dx/2.0;
	xmax=x1+dx/2.0;
	ymax=y1+dx/2.0;
    } else if (n==4) {
    	xmin=x1;
	ymin=y1;
	xmax=x2;
	ymax=y2;
    } else if (n==0 || n== -1) {
	xwin_window_get(&xmin, &ymin, &xmax, &ymax);
    } else {
	eprintf("WIN: bad number of points");
	return(0);
    }

    if (debug) printf("%g %g %g %g\n", xmin, ymin, xmax, ymax);

    if (fit) {
	db_bounds(&xmin, &ymin, &xmax, &ymax);
    }

    if (debug) printf("%g %g %g %g %d\n", xmin, ymin, xmax, ymax, n);

    if (xmax < xmin) {		/* canonicalize the selection rectangle */
	tmp = xmax; xmax = xmin; xmin = tmp;
    }
    if (ymax < ymin) {
	tmp = ymax; ymax = ymin; ymin = tmp;
    }

    if (fit) {
	dx=(xmax-xmin);
	dy=(ymax-ymin);
	xmin-=dx/10.0;
	xmax+=dx/10.0;
	ymin-=dy/10.0;
	ymax+=dy/10.0;
    } 

    if (scale != 1.0) {
	dx=(xmax-xmin);
	dy=(ymax-ymin);
	xmin=((xmax+xmin)/2.0)-((dx*scale)/2.0);
	xmax=((xmax+xmin)/2.0)+((dx*scale)/2.0);
	ymin=((ymax+ymin)/2.0)-((dy*scale)/2.0);
	ymax=((ymax+ymin)/2.0)+((dy*scale)/2.0);
    } 

    if (debug) printf("%g %g %g %g\n", xmin, ymin, xmax, ymax);

    if (xmin == 0 && ymin == 0 && xmax == 0 && ymax == 0) {
	;	/* don't set the window to a null size */
    } else {
	xwin_window_set(xmin,ymin,xmax,ymax);
    }
    

    return (0);
}

com_wrap(arg)		/* create a new device using existing components*/
char *arg;
{
    printf("    com_wrap\n", arg);
    return (0);
}


 /* Print out help for ARG, or for all commands if ARG is not present. */
com_help(arg)
char *arg;
{
    register int i;
    int printed = 0;

    printf("    com_help\n", arg);

    for (i = 0; commands[i].name; i++) {
	if (!*arg || (strcmp(arg, commands[i].name) == 0)) {
	    printf("%-12s%s.\n", commands[i].name, commands[i].doc);
	    printed++;
	}
    }

    if (!printed) {
	printf("No commands match `%s'.  Possibilties are:\n", arg);

	for (i = 0; commands[i].name; i++) {
	    /* Print in six columns. */
	    if (printed == 6) {
		printed = 0;
		printf("\n");
	    }
	    printf("%s\t", commands[i].name);
	    printed++;
	}

	if (printed)
	    printf("\n");
    }
    return (0);
}

 /* Return non-zero if ARG is a valid argument for CALLER, else print
        an error message and return zero. */

int valid_argument(caller, arg)
char *caller, *arg;
{
    if (!arg || !*arg) {
	eprintf("%s: Argument required.", caller);
    }
    return (1);
}


