%{

#include <stdio.h>
#include <math.h>
#include "db.h"
#include "readfont.h"

#define  MAX_FILE_NAME 256
#define  MAX_NUM_LEN   32

#define YES 1
#define NO  0

DB_TAB *currep;		/* keep track of current rep */
DB_TAB *newrep;		/* scratch pointer for new rep */
OPTS   *opts;
XFORM  *xp;

%}

%start archive
%token FILES FILE_NAME EDIT SHOW LOCK GRID LEVEL WINDOW ADD SAVE QUOTED
%token ALL NUMBER EXIT OPTION UNKNOWN PURGE
%token ARC CIRC INST LINE NOTE OVAL POLY RECT TEXT 


%union {
	double num;
	PAIR pair;
	COORDS *pairs;
	OPTS *opts;
	char *name;
	}

%type <num> archive command_list 
%type <num> FILES command 
%type <num> EDIT SHOW LOCK GRID LEVEL WINDOW ADD
%type <num> SAVE ALL LINE NUMBER EXIT PURGE
%type <num> ARC CIRC INST LINE NOTE OVAL POLY RECT TEXT
%type <pair> coord 
%type <pairs> coord_list
%type <name> FILE_NAME file_name_list UNKNOWN QUOTED
%type <name> OPTION 
%type <opts> option_list 

%%
%{

        void do_polygon(),do_rectangle();
        PAIR temp_pair; 

%}

archive		:	command_list

			    {

				xp = (XFORM *) malloc(sizeof(XFORM));  

				xp->r11 = 1.0;
				xp->r12 = 0.0;
				xp->r21 = 0.0;
				xp->r22 = 1.0;
				xp->dx  = 0.0;
				xp->dy  = 0.0;

				/* when done, print all definitions */
				    /* db_print(); */
				/* or render the last rep */
				db_render(currep,xp,0);

				/* db_def_archive(currep); */
			    }
		;


command_list	:	command
		|	command_list command
	    	;

command	        :	FILES file_name_list '$' ';'
        	|	EDIT FILE_NAME ';'
				{
				    /*
				    if (currep = lookup(FILE_NAME) {
					display();
				    } else if (in_path(FILE_NAME) {
					currep = read_in(FILE_NAME);
				    } else {
					currep = install($2);
				    }
				    */

				    currep = db_install($2);
				    printf("#got edit file: %s\n",$2);
				}
		|	SHOW ALL ';'
		|	LOCK NUMBER ';'
		|       GRID NUMBER NUMBER NUMBER ',' NUMBER ';'
		|       GRID NUMBER ',' NUMBER NUMBER ',' NUMBER ';'
		|	LEVEL NUMBER ';'
		|	WINDOW NUMBER ',' NUMBER NUMBER ',' NUMBER ';'
		|	ADD ARC option_list coord coord coord ';'
				{
				    db_add_arc(currep,
					(int) $2, $3, $4.x, $4.y,
					$5.x, $5.y, $6.x,$6.y);
				}
		|	ADD CIRC option_list coord coord ';'
				{
				    db_add_circ(currep, 
					(int) $2, $3, $4.x, $4.y, $5.x, $5.y);
				}
		|	ADD LINE option_list coord_list ';'
				{
				    db_add_line(currep, (int) $2, $3, $4);
				}
		|	ADD NOTE option_list QUOTED coord ';'
				{
				    db_add_note(currep, (int) $2, $3,
					$4, $5.x, $5.y);
				}
		|	ADD OVAL option_list coord coord coord ';'
				{
				    db_add_oval(currep,
					(int) $2, $3, $4.x, $4.y,
					$5.x, $5.y, $6.x,$6.y);
				}
		|	ADD POLY option_list coord_list ';'
				{
				    db_add_poly(currep, 
					(int) $2, $3, $4);
				}
		|	ADD RECT option_list coord coord ';'
				{
    				    db_add_rect(currep, (int) $2,
				    $3, $4.x, $4.y, $5.x, $5.y);
				}
		|	ADD TEXT option_list QUOTED coord ';'
				{
				    db_add_text(currep, (int) $2, $3,
					$4, $5.x, $5.y);
				}
		|	ADD INST FILE_NAME option_list coord ';'
				{
				    if ((newrep=db_lookup($3)) == 0) {
					printf("can't add non-existant cell\n");
				    }
				    db_add_inst(currep, newrep, $4, $5.x, $5.y);
				}	
		|	ADD FILE_NAME option_list coord ';'
				{
				    if ((newrep=db_lookup($2)) == 0) {
					printf("can't add non-existant cell\n");
				    }
				    db_add_inst(currep, newrep, $3, $4.x, $4.y);
				}	
		|	SAVE ';'
				/*
				{
				    if currep != NULL) {   
					write_db(currep);
				    } else {
					notify that there is no name yet
				    }
				}
				*/
		|	PURGE FILE_NAME ';'
			    {
				/*  uninstall($1); */
				printf("# 3) should purge: %s\n",$2);
			    }
		|	EXIT ';'
		|	error ';' 
				{
				    yyerrok;
				}
		;

file_name_list	:	FILE_NAME
			    {
				/*  uninstall($1); */
				printf("# 1) should purge: %s\n",$1);
			    }
	      	|	file_name_list ',' FILE_NAME
			    {
				/*  uninstall($3); */
				printf("# 2) should purge: %s\n",$3);
			    }
	      	;

option_list	:	/* allow null option list */
			    {
				$$ = (OPTS *) NULL;
			    }
		|       OPTION
			    {
				 first_opt = last_opt = opt_alloc($1);
				 $$ = first_opt;  
			    }
		|       option_list OPTION
			    {
				/* build option list */	
				 append_opt($2);
				 $$ = first_opt;  
			    }
		;
coord		:	NUMBER ',' NUMBER
				{
				    temp_pair.x = $1;
				    temp_pair.y = $3;
				    $$ = temp_pair;
				}
		;

coord_list	:	coord  coord
				{
				    first_pair = last_pair = pair_alloc($1); 
				    append_pair($2);
				    $$ = first_pair;
				}
		|	coord_list coord 
				{
				    append_pair($2);
				    $$ = first_pair;
				}
		;

%%

FILE *yyerfp;  /* error stream */

main() {
#ifdef YYDEBUG
    extern int yydebug;

    yydebug = 1;
#endif
    yyerfp = stdout;

    loadfont("NOTEDATA.F");
    printf("# yyparse() == %d\n", yyparse());
}


/* yywhere() -- custom input position for yyparse()
 * yymark()  -- get information from '# line file'
 */


extern char yytext[];	/* current token */
extern int yyleng;	/* and its length */
extern int yylineno;	/* current input line number */

static char *source;	/* current input filename */

yywhere() {				/* position stamp */
	char colon = 0;		/* a flag */

	if (source && *source && strcmp(source, "\"\"")) {
		char *cp = source;
		int len = strlen(source);

		if (*cp == '"')
			++cp, len -= 2;
		if (strncmp(cp,"./",2)==0)
			cp += 2, len -= 2;
		fprintf(yyerfp, "file %.*s", len, cp);
		colon = 1;
	}
	if (yylineno > 0) {
		if (colon)
			fputs(", ",yyerfp);
		fprintf(yyerfp, "line %d",
			yylineno -
			(*yytext == '\n' || ! *yytext));
		colon = 1;
	}
	if (*yytext) {
		register int i;
		for (i=0; i<20; ++i)
			if(!yytext[i] || yytext[i] == '\n')
				break;
		if (i) {
			if (colon)
				putc(' ', yyerfp);
			fprintf(yyerfp, "near \"%.*s\"",i,yytext);
			colon = 1;
		}
	}
	if (colon)
		fputs(": ",yyerfp);
}
		
yymark()	/* retrieve from '# digits text' */
{
	if (source)
		cfree(source);
	source = (char *) calloc(yyleng, sizeof(char));
	if (source)
		sscanf(yytext, "# %d %s", &yylineno, source);
}

yyerror(s)
register char *s;
{
	extern int yynerrs;	/* total number of errors */

	/* should be: fprintf(yyerfp, "[error %d] ", yynerrs+1); */
	/* but there is a bug in /usr/lib/yacpar that doesn't */
	/* increment the yynerrs variable on syntax errors    */

	fprintf(yyerfp, "# [error %d] ", ++yynerrs);
	yywhere();
	fputs(s,yyerfp);
	putc('\n', yyerfp);
}

