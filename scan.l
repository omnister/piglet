
L		[A-Za-z]
LD		[A-Za-z0-9]
D		[0-9]
LDM		[A-Za-z0-9/_.]
X		[+-]?{D}*"."{D}+
Z		[+-]?{D}+"."{D}*
U		[+-]?{D}+
W		[\t ]*

%{
#include <stdio.h>
#include "db.h"
#include "xwin.h"

#ifdef DEBUG	/* debugging version - if assert ok */
#include <assert.h>

    main()
    {
	char *p;
	assert(sizeof(int) >= sizeof(char *));

	while (p = (char *) yylex()) {
	    printf("%-10.10s is \"%s\"\n", p, yytext);
	}
    }

#define token(x)    (int) "x"
#else	!DEBUG	/* production version */
#include "y.tab.h"
#define token(x)    x
#endif DEBUG

int layer=0;	/* default layer storage */

/*  redefine lex input stream */
#undef YY_INPUT
#define YY_INPUT(b, r, ms) (r = my_yyinput(b,ms))

#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>

char *lineread = (char *) NULL;
int pushback = (char) NULL;
char    *rl_gets(); 

%}

%%

[ \t\n]+		;

\"[^"]*\"		{
			    /* strsave(yytext); */
			    /* remove open and close quotes */
			    yylval.name = (char *) strdup(yytext+1);
			    if (yylval.name[yyleng-2] != '"') {
				fprintf(stderr, 
				    "unterminated character string",
				    (char *) 0);
			    } else {
				yylval.name[yyleng-2] = '\0';
			    }
			    return token(QUOTED);
			}    

^$FILES			return token(FILES);
^EDI[T]			return token(EDIT);
^SHO[W]			return token(SHOW);
^LOC[K]			return token(LOCK);
^GRI[D]			return token(GRID);
^LEVEL                  |
^LEV			return token(LEVEL);
^WINDOW                 |
^WIN			return token(WINDOW);
^ADD			return token(ADD);

A{D}*			{
			    if (strlen(yytext) == 1) {
				yylval.num = layer;
			    } else {
				sscanf(yytext+1, "%lf", &yylval.num);
				layer = yylval.num;
			    }
			    return token(ARC);
			}
C{D}*			{
			    if (strlen(yytext) == 1) {
				yylval.num = layer;
			    } else {
				sscanf(yytext+1, "%lf", &yylval.num);
				layer = yylval.num;
			    }
			    return token(CIRC);
			}
L{D}*			{
			    if (strlen(yytext) == 1) {
				yylval.num = layer;
			    } else {
				sscanf(yytext+1, "%lf", &yylval.num);
				layer = yylval.num;
			    }
			    return token(LINE);
			}
N{D}*			{
			    if (strlen(yytext) == 1) {
				yylval.num = layer;
			    } else {
				sscanf(yytext+1, "%lf", &yylval.num);
				layer = yylval.num;
			    }
			    return token(NOTE);
			}	
O{D}*			{
			    if (strlen(yytext) == 1) {
				yylval.num = layer;
			    } else {
				sscanf(yytext+1, "%lf", &yylval.num);
				layer = yylval.num;
			    }
			    return token(OVAL);
			}
P{D}*			{
			    if (strlen(yytext) == 1) {
				yylval.num = layer;
			    } else {
				sscanf(yytext+1, "%lf", &yylval.num);
				layer = yylval.num;
			    }
			    return token(POLY);
			}
R{D}*			{
			    if (strlen(yytext) == 1) {
				yylval.num = layer;
			    } else {
				sscanf(yytext+1, "%lf", &yylval.num);
				layer = yylval.num;
			    }
			    return token(RECT);
			}
T{D}*			{
			    if (strlen(yytext) == 1) {
				yylval.num = layer;
			    } else {
				sscanf(yytext+1, "%lf", &yylval.num);
				layer = yylval.num;
			    }
			    return token(TEXT);
			}

I			{
			    return token(INST);
			}

^SAV[E]			return token(SAVE);
#A			return token(ALL);
^EXI[T]			return token(EXIT);

^PUR[G]			|
^PURG[E]		return token(PURGE);

[.:@]{L}[^ \t\n]+	{
			    yylval.name = strsave(yytext); 
			    return token(OPTION);
			}

{U}			|
{X}			|
{Z}			{
			    sscanf(yytext, "%lf", &yylval.num); 
			    return token(NUMBER);
			}

"/"{LDM}+		|
{L}{LDM}*		{
			    yylval.name = strsave(yytext); 
			    return token(FILE_NAME);
			}

[;:,$"]			return token(yytext[0]);

.			return token(UNKNOWN);

%%

yywrap()
{
	return(1);
}

/* simple version.  truncates long input strings to size of lex buffer */

int yymin(a,b)
int a,b;
{
   return((a<b)?a:b);
}

int my_yyinput(buf, maxsize)
char *buf;
int maxsize;
{
       char *s;
       int k,n;

       /* s = rl_gets(""); */
       /* s = fgets(buf,maxsize,stdin); */

       /* bad form because buf sizes are not carefully considered... */
       s = procXevent();
    
       if (s==0) return (0);

       k = strlen(s);
       n = yymin(maxsize, k);

       /* printf("maxsize=%d, s=%s n=%d\n",maxsize, s, n); */

       if (n>0) {
	    memcpy(buf,s,n);
       }
       return n;
}

/* *************************************************** */
/* some routines to implement command line history ... */
/* *************************************************** */


/* expand and duplicate a string with malloc */

char * expdupstr(s,n)
int s, n;
{
    char *r;

    r = (char *) xmalloc(strlen( (char *) s) + n);
    strcpy(r, (char *) s);
    return (r);
}

rl_ungetc(c,fd) 
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

initialize_readline()
{
    /* Allow conditional parsing of the ~/.inputrc file. */
    rl_readline_name = ".pigrc";
}


rlgetc(fd)
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
	    lineread=rl_gets(":");
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
