
L		[A-Za-z]
LD		[A-Za-z0-9]
D		[0-9]
LDM		[A-Za-z0-9/_.]
Z		[+-]?{D}+"."{D}*
U		[+-]?{D}+
W		[\t ]*

%{
#include <stdio.h>
#include "gram.h"

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

\"[^"]*\"		return token(QUOTED);

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

N{D}+			{
			    sscanf(yytext+1, "%d", &yylval.num);
			    return token(NOTE);
			}	
T{D}+			{
			    sscanf(yytext+1, "%d", &yylval.num);
			    return token(TEXT);
			}
R{D}+			{
			    sscanf(yytext+1, "%d", &yylval.num);
			    return token(RECTANGLE);
			}
P{D}+			{
			    sscanf(yytext+1, "%d", &yylval.num);
			    return token(POLYGON);
			}
L{D}+			{
			    sscanf(yytext+1, "%d", &yylval.num);
			    return token(LINE);
			}
C{D}+			{
			    sscanf(yytext+1, "%d", &yylval.num);
			    return token(CIRCLE);
			}

^SAV[E]			return token(SAVE);
#A			return token(ALL);
^EXI[T]			return token(EXIT);

[.:@]{L}[^ \t\n]+	{
			    return token(OPTION);
			    yylval.name = strsave(yytext); 
			}

{U}			|
{Z}			{
			    sscanf(yytext, "%d", &yylval.num); 
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

