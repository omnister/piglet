#include <unistd.h> 	/* exit() */
#include <stdlib.h>	/* malloc() */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "eprintf.h"

/* originally from code in Kernighan & Pike, */
/* "The Practice of Programming" */

static char *name = NULL; 	/* program name for messages */

/* eprintf: print error message and exit */
void eprintf(char *fmt, ...)
{
    va_list args;

    fflush(stdout);
    if(progname() != NULL)
	fprintf(stderr, "%s: ", progname());
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':')
	fprintf(stderr, " %s", strerror(errno));
    fprintf(stderr, "\n");
    exit(2); 
}

/* weprintf: print error message warning (no exit) */
void weprintf(char *fmt, ...)
{
    va_list args;

    fflush(stdout);
    if(progname() != NULL)
	fprintf(stderr, "%s: ", progname());
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':')
	fprintf(stderr, " %s", strerror(errno));
    fprintf(stderr, "\n");
}

/* estrdup: duplicate a string, report if error */
char *estrdup(char *s)
{
    char *t;

    t = (char *) malloc(strlen(s)+1);
    if (t == NULL) 
	eprintf("estrdup(\"%.20s\") failed:", s);
    strcpy(t,s);
    return(t);
}

/* emalloc: malloc and report if error */
void *emalloc(size_t n)
{
    void *p;

    p = (void *) malloc(n);
    if (p == NULL) 
	eprintf("malloc of %u bytes failed:", n);
    return(p); 
}

/* erealloc: malloc and report if error */
void *erealloc(void *p, size_t n)
{
    void *newp;
    newp = (void *) realloc(p, n);
    if (newp == NULL) 
	eprintf("eralloc of %u bytes failed:", n);
    return(newp); 
}

/* setprogname: set stored name of program */
void setprogname(char *str)
{
    name = estrdup(str);
}

/* progname: return stored name of program */
char *progname(void)
{
    return(name);
}


