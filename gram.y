%{
#include <stdio.h>
#include <math.h>
char *malloc();

#define  MAX_FILE_NAME 256
#define  MAX_NUM_LEN   32

#define YES 1
#define NO  0


typedef struct coord_pair { 
    int x,y; 
} PAIR;

typedef struct coord_leaf { 
    PAIR coord; 
    struct coord_leaf *next;
} COORD_LEAF, *COORD_LIST;

typedef struct rect {
    char *name; 
    int x1,y1,x2,y2; 
    struct rect *next; 
} RECT, *RECT_LIST;

COORD_LIST first_pair,last_pair;
RECT_LIST  first_add,last_add,first_delete,last_delete;
int hflag,delta,feature;
int	line_number;

%}

%start archive
%token FILES FILE_NAME EDIT SHOW LOCK GRID LEVEL WINDOW ADD SAVE QUOTED
%token ALL LINE RECTANGLE POLYGON CIRCLE NUMBER EXIT OPTION UNKNOWN
%token TEXT NOTE


%union {
	int num;
	PAIR pair;
	COORD_LIST pairs;
	char *name;
	}

%type <num> archive files_command command_list 
%type <num> FILES command 
%type <num> EDIT SHOW LOCK GRID LEVEL WINDOW ADD RECTANGLE POLYGON
%type <num> SAVE ALL CIRCLE LINE NUMBER EXIT
%type <pair> coord 
%type <pairs> coord_list
%type <name> FILE_NAME file_name_list UNKNOWN
%type <name> OPTION option_list 

%%
%{

	COORD_LIST pair_alloc();
        void append_pair(), discard_pairs(),do_polygon(),do_rectangle();
        PAIR temp_pair; 

%}

archive		:	files_command command_list
			    {
				/* when done, print all definitions */
				printdefs();
			    }
		;

files_command	:	FILES file_name_list '$' ';'
		;

file_name_list	:	FILE_NAME
			    {
				/*  uninstall($1); */
				printf(" purging %s\n",$1);
			    }
	      	|	file_name_list ',' FILE_NAME
			    {
				/*  uninstall($3); */
				printf(" purging %s\n",$3);
			    }
	      	;

option_list	:	/* allow null option list */
			    {
				$$ = (char *) NULL;
			    }
		|       OPTION
			    {
				$$ = $1;
			    }
		|       option_list OPTION
			    {
				/* build option list */	
			    }
		;

command_list	:	command
		|	command_list command
	    	;

command		:	EDIT FILE_NAME ';'
				{
				    /*
				    if (curr_rep = lookup(FILE_NAME) {
					display();
				    } else if (in_path(FILE_NAME) {
					curr_rep = read_in(FILE_NAME);
				    } else {
					curr_rep = install($2);
				    }
				    */

				    install($2);
				    printf("got edit file: %s\n",$2);
				}
		|	SHOW ALL ';'
		|	LOCK NUMBER ';'
		|       GRID NUMBER ',' NUMBER NUMBER ',' NUMBER ';'
		|	LEVEL NUMBER ';'
		|	WINDOW NUMBER ',' NUMBER NUMBER ',' NUMBER ';'
		|	ADD POLYGON option_list coord_list ';'
				{
				    /* install_part(POLYGON, $2, $4); */

				    do_polygon($2,$4);
				    discard_pairs();
				}
		|	ADD TEXT option_list QUOTED coord ';'
				{
				/* do_text($2,$3,$4); */
				}
		|	ADD NOTE option_list QUOTED coord ';'
				{
				/* do_note($2,$3,$4); */
				}
		|	ADD RECTANGLE option_list coord coord ';'
				{
				    first_pair = last_pair = pair_alloc($4); 
				    append_pair($5);

				    /* install_part(RECTANGLE, $2, first_pair);*/

				    do_rectangle($2, first_pair);
				}
		|	ADD CIRCLE option_list coord coord ';'
				{
				    first_pair = last_pair = pair_alloc($4); 
				    append_pair($5);

				    /* install_part(CIRCLE, $2, first_pair);*/

				    do_circle($2,first_pair);
				}
		|	ADD LINE option_list coord_list ';'
				{
				    /* install_part(LINE, $2, $4); */

				    do_line($2,$4); 
				    discard_pairs(); 
				}
		/* the coord_list is only added below for :s hack */
		|	ADD FILE_NAME option_list coord ';'
		|	ADD FILE_NAME option_list coord_list ';'
		|	SAVE ';'
				/*
				{
				    if curr_rep != NULL) {   
					write_db(curr_rep);
				    } else {
					notify user that there is no name yet
				    }
				}
				*/
		|	EXIT ';'
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

main() {
#ifdef YYDEBUG
    extern int yydebug;

    yydebug = 1;
#endif
    printf("yyparse() == %d\n", yyparse());
}


/* yywhere() -- custom input position for yyparse()
 * yymark()  -- get ingoramtiaon from '# line file'
 */

FILE *yyerfp = stdout;	/* error stream */
extern char yytext[];	/* current token */
extern int yyleng;		/* and its length */
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

	fprintf(yyerfp, "[error %d] ", ++yynerrs);
	yywhere();
	fputs(s,yyerfp);
	putc('\n', yyerfp);
}


/******************** handle coordinates *********************/

COORD_LIST pair_alloc(p)
PAIR p;
{	char *temp;
	temp = malloc(sizeof(COORD_LEAF));
	((COORD_LIST)temp)->coord = p;
	((COORD_LIST)temp)->next = NULL;
	return((COORD_LIST)temp);
}

void append_pair(p)
PAIR p;
{	last_pair->next = pair_alloc(p);
	last_pair = last_pair->next;
}

void discard_pairs()
{	
	COORD_LIST temp;

	while(first_pair != NULL)
	{
		temp = first_pair;
		first_pair = first_pair->next;
		free((char *)temp);
	}
	last_pair = NULL;
}		

/******************** handle coordinates *********************/

void do_circle(lnum, pcoords)
int lnum;
COORD_LIST pcoords;
{
    void draw();
    void jump();
    int i;
    double r,theta,x,y,x1,y1,x2,y2;

    x1 = (double) pcoords->coord.x;
    y1 = (double) pcoords->coord.y;
    x2 = (double) pcoords->next->coord.x;
    y2 = (double) pcoords->next->coord.y;
    
    if (pcoords->next->next != 0) {
	printf("error in circle record: too many coordinates\n");
    }

    jump();
    set_pen(lnum);

    x = (double) (x1-x2);
    y = (double) (y1-y2);
    r = sqrt(x*x+y*y);
    for (i=0; i<=16; i++) {
	theta = ((double) i)*2.0*3.1415926/16.0;
	x = r*sin(theta);
	y = r*cos(theta);
	draw((int) x1+ (int) x, (int) y1+ (int) y);
    }
}

void do_rectangle(lnum, pcoords)
int lnum;
COORD_LIST pcoords;
{
    void draw();
    void jump();

    double x1,y1,x2,y2;

    x1 = (double) pcoords->coord.x;
    y1 = (double) pcoords->coord.y;
    x2 = (double) pcoords->next->coord.x;
    y2 = (double) pcoords->next->coord.y;
    
    if (pcoords->next->next != 0) {
	printf("error in circle record: too many coordinates\n");
    }

    jump();
    set_pen(lnum);
    draw((int) x1, (int) y1); draw((int) x1, (int) y2);
    draw((int) x2, (int) y2); draw((int) x2, (int) y1);
    draw((int) x1, (int) y1);
}

void do_line(lnum, pcoords)
int lnum;
COORD_LIST pcoords;
{
    void draw();
    void jump();

    COORD_LIST temp;
    
    jump();
    set_pen(lnum);
    temp = pcoords;
    while(temp != NULL) {
	    draw(temp->coord.x, temp->coord.y);
	    temp = temp->next;
    }
}

void do_polygon(lnum, pcoords)
int lnum;
COORD_LIST pcoords;
{
    void draw();
    void jump();

    COORD_LIST temp;
    
    jump();
    set_pen(lnum);

    temp = pcoords;
    /* should eventually check for closure */
    while(temp != NULL) {
	    draw(temp->coord.x, temp->coord.y);
	    temp = temp->next;
    }
}

void draw(x, y)
int x,y;
{
    printf("%d %d\n",x,y);
}

void jump() 
{
    printf("jump\n");
}

set_pen(lnum)
int lnum;
{
    printf("pen %d\n", (lnum%5)+1);
}
    

char *strsave(s)   /* save string s somewhere */
char *s;
{
    char *p, *alloc();

    if ((p = malloc(strlen(s)+1)) != NULL)
	strcpy(p,s);
    return(p);
}
