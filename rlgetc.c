#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>  

#include "eprintf.h"
#include "db.h"
#include "xwin.h"
#include "token.h"

extern char *getwd();
extern char *xmalloc();
char * stripwhite();
static char *lineread = (char *) NULL;
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
    // printf("->%2.2x %c\n",c,c);
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

char * expdupstr(char *s, int n)
{
    char *r;
    r = (char *) malloc(strlen( (char *) s) + n);
    strcpy(r, (char *) s);
    return (r);
}


int rl_ungetc(lp,c) 
LEXER *lp;
int c;
{
    int debug=0;

    if (debug) printf("ungetting %2.2x %c at location %d\n",c,c,lp->pbufp); 

    (lp->pushback)[lp->pbufp] = c;
    lp->pbufp++; 
    return(1);

}

/* stuff a string back onto stdin in reverse order */

int rl_ungets(lp,s) 
char *s;
LEXER *lp;
{
    int i;
    for (i=strlen(s)-1; i>=0; i--) {
    	rl_ungetc(lp,s[i]);
    }
    return(1);
}

static FILE *pigrcfp = NULL;

int rl_readin_file(FILE *fp) {
    pigrcfp = fp;
    return(0);
}

/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
char * rl_gets (prompt)
char *prompt;
{

    char *s;
    char buf[1024];

    /* If the buffer has already been allocated, return the memory
       to the free pool. */

    if (lineread) {
        free (lineread);
        lineread = (char *) NULL;
    }

    if (pigrcfp == NULL) { 			/* Get a line from the user. */
	lineread = readline (prompt);
	if (lineread == NULL) {
	    return(NULL);
	}

	/* If the line has any text in it, save it on the history. */
	if (lineread && *lineread) {
	    add_history (lineread);
	}
	
	/* add a newline to return string */

	s = expdupstr(lineread,2);
	strcat(s,"\n");
	free (lineread);
	lineread = s;
    } else {
	if (fgets(buf, 1023, pigrcfp) == NULL)  {
	    pigrcfp=NULL;
	    s = expdupstr(";\n",0);
	} else {
	    s = expdupstr(buf,2);
	    strcat(s,";\n");
	}
	lineread = s;
    }

    return (lineread);
}

void initialize_readline()
{
    
    // unbind tab key or else we get interactive chaos when pigrc is read
    rl_bind_key('\t', rl_insert);

    /* Allow conditional parsing of an inputrc file. */
    /* commented out to force readline to use the default */
    /* .inputrc name */
    /* rl_readline_name = (char *) estrdup(progname()); */

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

int rlgetc(lp)
LEXER *lp;
{
    int c;
    static char *linep = NULL;

    if (lp->pbufp != 0) {				// pushback ?
	lp->pbufp--;
	c = lp->pushback[lp->pbufp];
    } else if (lp->token_stream != stdin) {		// non-interactive readin
	c=getc(lp->token_stream);
    } else if(linep != NULL && *linep != '\0') {	// eating current line?
	c=*(linep++);
    } else {						// get a new line
	if (prompt != NULL) {
	    lineread=rl_gets(prompt);
	} else {
	    lineread=rl_gets(":");
	}
	if (lineread == NULL) {
	    linep = NULL;
	    if (lp->token_stream == stdin) {
		printf("trapped EOF on input: use QUIT to exit...\n");
		c='\n';
	    } else {
	    	c=EOF;
	    }
	} else {
	    linep = lineread;
	    c=*(linep++);
	}
    }

    /* printf("->%2.2x %c\n",c,c); */
    return (c);
}
