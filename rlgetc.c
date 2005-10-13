#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>  

#include "eprintf.h"
#include "db.h"
#include "xwin.h"

extern char *getwd();
extern char *xmalloc();
char * stripwhite();
char *lineread = (char *) NULL;
int pushback = (char) NULL;
double getdouble();

int	lineno = 1;
#include <signal.h>
#include <setjmp.h>
jmp_buf begin;
int	indef;
char	*infile;    /* input file name */
FILE	*fin;	    /* input file pointer */
char	**gargv;    /* global argument list */
int	gargc;
char    *rl_gets();

/*
main() {
    char *s;
    while((s=rl_gets(":")) != NULL) {
	if (strlen(s) != 1) {
	    printf("got %s",s);
	}
    }
    exit(0);
}
*/


/* *************************************************** */
/* some routines to implement command line history ... */
/* *************************************************** */

char *prompt = (char *) NULL;
char *saveprompt = (char *) NULL;

char *rl_saveprompt() {
    if (prompt != (char *) NULL) {
	 if (saveprompt != (char *) NULL) {
	     free(saveprompt);
	 }
	 saveprompt = (char *) estrdup(prompt);
    } else {
	saveprompt = (char *) estrdup(">");
    }
    return(saveprompt);
}

void rl_restoreprompt() {
    if (saveprompt != (char *) NULL) {
	if (prompt != (char *) NULL) {
	    free(prompt);
	}
        prompt = (char *) estrdup(saveprompt);
    } else {
	prompt = (char *) estrdup(">");
    }
}

void rl_setprompt(char *str)
{
    if (prompt != (char *) NULL) {
	free(prompt);
    }
    prompt = (char *) estrdup(str);
}


/* prototype for working with a pure file input */
/* the real rlgetc() is a co-routine with procXevent */
/* and gets characters from both mouse and keyboard */

int xrlgetc(fd) 
FILE *fd;
{
    int c;
    c=getc(fd);
    /* printf("->%2.2x %c\n",c,c); */
    return (c);
}

int xrl_ungetc(c,fd)
int c;
FILE *fd;
{
    /* printf("ungetting %2.2x %c\n",c,c); */
    return ungetc(c,fd);
}


/* expand and duplicate a string with malloc */

char * expdupstr(s,n)
int s, n;
{
    char *r;

    r = malloc(strlen( (char *) s) + n);
    strcpy(r, (char *) s);
    return (r);
}

int rl_ungetc(c,fd) 
int c;
FILE *fd;
{
    /* printf("ungetting %2.2x %c\n",c,c); */

    if (fd != stdin) {
	return(ungetc(c,fd));
    } else {
	pushback=c;
	return(1);
    }
}


/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
char * rl_gets (prompt)
char *prompt;
{

    char *s;

    /* If the buffer has already been allocated, return the memory
       to the free pool. */

    if (lineread) {
        free (lineread);
        lineread = (char *) NULL;
    }

    /* Get a line from the user. */
    /* printf("entering readline\n"); */
    lineread = readline (prompt);
    if (lineread == NULL) {
	/* printf("got a NULL\n"); */
	return(NULL);
    }
    fflush(stdout);

    /* If the line has any text in it, save it on the history. */
    if (lineread && *lineread)
	add_history (lineread);
    
    /* add a newline to return string */

    s = expdupstr(lineread,2);
    strcat(s,"\n");
    free (lineread);
    lineread = s;

    return (lineread);
}

void initialize_readline()
{
    /* Allow conditional parsing of the ~/.inputrc file. */
    rl_readline_name = (char *) estrdup(progname());

    /* Now tell readline where to get characters 
     *
     * This is how the multitasking operates between the text and
     * graphical portions of the system:  readline calls the Xevent
     * loop to get characters through a select() call, and the Xevent
     * loop returns characters to be processed.  Once it starts up,
     * the two processes act as co-routines, transferring control
     * back and forth.
     */
    rl_getc_function = procXevent;
}


int rlgetc(fd)
FILE *fd;
{
    int c;
    static char *lp = NULL;

    if (fd != stdin) {
	c=getc(fd);
    } else {
	if (pushback != (char) NULL) {
	    c=pushback;
	    pushback=(char) NULL;
	} else if(lp != NULL && *lp != '\0') {
	    c=*(lp++);
	} else {
	    if (prompt != NULL) {
		lineread=rl_gets(prompt);
	    } else {
		lineread=rl_gets(":");
	    }
	    if (lineread == NULL) {
		c=EOF;
	    } else {
		lp = lineread;
		c=*(lp++);
	    }
	}
    }

    /* printf("->%2.2x %c\n",c,c); */
    return (c);
}
