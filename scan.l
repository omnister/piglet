
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

A{D}+			{
			    sscanf(yytext+1, "%lf", &yylval.num);
			    return token(ARC);
			}
C{D}+			{
			    sscanf(yytext+1, "%lf", &yylval.num);
			    return token(CIRC);
			}
L{D}+			{
			    sscanf(yytext+1, "%lf", &yylval.num);
			    return token(LINE);
			}
N{D}+			{
			    sscanf(yytext+1, "%lf", &yylval.num);
			    return token(NOTE);
			}	
O{D}+			{
			    sscanf(yytext+1, "%lf", &yylval.num);
			    return token(OVAL);
			}
P{D}+			{
			    sscanf(yytext+1, "%lf", &yylval.num);
			    return token(POLY);
			}
R{D}+			{
			    sscanf(yytext+1, "%lf", &yylval.num);
			    return token(RECT);
			}
T{D}+			{
			    sscanf(yytext+1, "%lf", &yylval.num);
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

