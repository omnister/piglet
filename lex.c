
#include <stdio.h>
#include <string.h>		/* for strchr() */

#include <readline/readline.h> 	/* for command line editing */
#include <readline/history.h>  

#include "rlgetc.h"
#include "db.h"

#include "xwin.h" 	

typedef enum {
    IDENT, 	/* identifier */
    CMD,	/* command */
    QUOTE, 	/* quoted string */
    NUMBER, 	/* number */
    OPT,	/* option */
    EOL,	/* newline or carriage return */
    EOC,	/* end of command */
    COMMA,	/* comma */
    END		/* end of file */
} TOKEN;

static TOKEN gettoken();
static int ungettoken(TOKEN token, char *word);
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
    
    while((token=gettoken(word)) != EOF) {
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
	    case IDENT: /* identifier */
		printf("	IDENT: %s\n", word);
		break;
	    case CMD:	/* command */
		/* printf("	CMD: %s\n", word); */
		command = find_command(word);

		/* Call the function. */
		retcode = ((*(command->func)) (""));

		break;
	    case QUOTE: /* quoted string */
		printf("	QUOTE: %s\n", word);
		break;
	    case NUMBER: /* number */
		printf("	NUMBER: %s\n", word);
		break;
	    case OPT:	/* option */
		printf("	OPT: %s\n", word);
		break;
	    case COMMA:	/* comma */
		printf("	COMMA: %s\n", word);
		break;
	    case EOL:	/* newline or carriage return */
		printf("	EOL:\n");
		break;
	    case EOC:	/* end of command */
		printf("	EOC: %s\n", word);
		break;
	    case END:	/* end of file */
		printf("	IDENT: %s\n", word);
		break;
	}
    }
}

#define BUFSIZE 100
struct savetok {
    char *word;
    TOKEN tok;
} tokbuf[BUFSIZE];

int bufp = 0;		/* next free position in buf */

/* stuff back a token */
static int ungettoken(TOKEN token, char *word) 
{
    if (bufp >= BUFSIZE) {
	eprintf("ungettoken: too many characters");
	return(-1);
    } else {
	tokbuf[bufp].word = (char *) estrdup(word);
	tokbuf[bufp++].tok = token;
	return(0);
    }
}

static TOKEN gettoken(word) /* collect and classify token */
char *word;
{
    enum {NEUTRAL,INQUOTE,INWORD,INOPT,INNUM} state = NEUTRAL;
    int c;
    char *w;
    
    if (bufp > 0) {
	strcpy(word, tokbuf[--bufp].word);
	free(tokbuf[bufp].word);
	tokbuf[bufp].word = (char *) NULL;
	return(tokbuf[bufp].tok);
    }

    w=word;
    while((c=rlgetc(stdin)) != EOF) {
	switch(state) {
	    case NEUTRAL:
		switch(c) {
		    case ' ':
		    case '\t':
			continue;
		    case '\n':
		    case '\r':
			*w++ = c;
			*w = '\0';
			return(EOL);
		    case ',':
			*w++ = c;
			*w = '\0';
			return(COMMA);
		    case '"':
			state = INQUOTE;
			continue;
		    case ':':
			state = INOPT;
			*w++ = c;
			continue;
		    case ';':
			*w++ = c;
			*w = '\0';
			return(EOC);
		    case '0':
		    case '1':
		    case '2':
		    case '3':
		    case '4':
		    case '5':
		    case '6':
		    case '7':
		    case '8':
		    case '9':
		    case '+':
		    case '-':
			state = INNUM;
			*w++ = c;
			continue;
		    case '.':
			*w++ = c;
			c = rlgetc(stdin);
			if (isdigit(c)) {
			    rl_ungetc(c,stdin);
			    state = INNUM;
			} else {
			    rl_ungetc(c,stdin);
			    state = INOPT;
			}
			continue;	
		    default:
			state = INWORD;
			*w++ = c;
			continue;
		}
	    case INNUM:
		if (isdigit(c) || c=='.') {
		    *w++ = c;
		    continue;
		} else {
		    rl_ungetc(c,stdin);
		    *w = '\0';
		    return(NUMBER);
		}
	    case INOPT:
		if (isalnum(c)) {
		    *w++ = c;
		    continue;
		} else if (strchr("+-.",c) != NULL) {
		    *w++ = c;
		    continue;
		} else {
		    rl_ungetc(c,stdin);
		    *w = '\0';
		    return(OPT);
		}
	    case INQUOTE:
		switch(c) {
		    case '\\':
			*w++ = rlgetc(stdin);
			continue;
		    case '"':
			*w = '\0';
			return(QUOTE);
		    default:
			*w++ = c;
			continue;
		}
	    case INWORD:
		if (!isalnum(c)) {
		    rl_ungetc(c,stdin);
		    *w = '\0';
		    if (lookup_command(word)) {
			return(CMD);
		    } else {
			return(IDENT);
		    }
		}
		*w++ = c;
		continue;
	    default:
		fprintf(stderr,"pig: error in lex loop\n");
		exit(1);
		break;
	} /* switch state */
    } /* while loop */
    return(EOF);
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

com_add(arg)		/* add a component to the current device */
char *arg;
{
    char *line;
    double x1,y1,x2,y2;

    /* check that we are editing a rep */
    if (currep == NULL ) {
	printf("must do \"EDIT <name>\" before ADD\n");
	return(1);
    }

    /* check args */
    if (sscanf(arg, "%lf,%lf %lf,%lf", &x1, &y1, &x2, &y2) != 4) {
	printf("need two coordinates\n");
	return(1);
    }

    printf("adding rect to currep, layer 1, no opts %g %g %g %g\n",x1,y1,x2,y2);
    db_add_rect(currep, 1, (OPTS *) NULL, x1, y1, x2, y2);

    /* line = readline("coords: "); */
    /* free(line); */

    modified = 1;
    need_redraw++;

    return (0);
}

com_archive(arg)	/* create an archive file of the specified device */
char *arg;
{
    printf("    com_archive %s\n", arg);
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

    if (currep != NULL) {
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

    if (mode == MAIN) {

	/* check for a name provided */
	if (token=gettoken(name) != IDENT) {
	    printf("    1must provide a name: EDIT <name>\n");
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
	printf("    recursive EDIT not supported\n");
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


com_grid(arg)		/* change or redraw grid */
char *arg;
{
    TOKEN token;
    int done=0;
    char buf[128];
    char word[128];

    printf("    com_grid %s\n", arg);

    buf[0]='\0';
    while(!done && (token=gettoken(word)) != EOF) {
	switch(token) {
	    case IDENT: 	/* identifier */
	    case QUOTE: 	/* quoted string */
	    case OPT:		/* option */
	    case END:		/* end of file */
		break;
	    case CMD:		/* command */
		/* ungettoken(token, word); */
		break;
	    case NUMBER: 	/* number */
		strcat(buf,word);
		strcat(buf," ");
		break;
	    case EOL:		/* newline or carriage return */
	    case EOC:		/* end of command */
		done++;
		break;
	    case COMMA:		/* comma */
		break;
	    default:
		eprintf("bad case in com_grid");
		break;
	}
    }
    printf("calling xwin_grid with %s\n",buf);
    xwin_grid(buf);
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

com_save(arg)		/* save the current file or device to disk */
char *arg;
{
    need_redraw++; 

    printf("    com_save\n", arg);

    if (currep != NULL) {
	db_save(currep);
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
    double xmin, ymin, xmax, ymax, tmp;
    int n;
    TOKEN token;
    int done=0;
    char buf[128];
    char word[128];

    printf("    com_window\n", arg);

    buf[0]='\0';
    while(!done && (token=gettoken(word)) != EOF) {
	switch(token) {
	    case IDENT: 	/* identifier */
	    case QUOTE: 	/* quoted string */
	    case OPT:		/* option */
	    case END:		/* end of file */
		break;
	    case CMD:		/* command */
		/* ungettoken(token, word); */
		break;
	    case NUMBER: 	/* number */
		strcat(buf,word);
		strcat(buf," ");
		break;
	    case EOL:		/* newline or carriage return */
	    case EOC:		/* end of command */
		done++;
		break;
	    case COMMA:		/* comma */
		break;
	    default:
		eprintf("bad case in com_grid");
		break;
	}
    }
    printf("xwin_window got %s\n",buf);

    n = sscanf(buf, "%lf %lf %lf %lf",
	&xmin, &ymin,
	&xmax, &ymax
    );

    if (xmax < xmin) {		/* canonicalize the selection rectangle */
	tmp = xmax; xmax = xmin; xmin = tmp;
    }
    if (ymax < ymin) {
	tmp = ymax; ymax = ymin; ymin = tmp;
    }
    
    xwin_window(n,xmin,ymin,xmax,ymax);

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
