#include <stdio.h>

/* definition of hierarchical editor data structures */

/*
 *   (this chain from HEAD is the cell definition symbol table)
 *
 *   HEAD->[db_tab <cellname0> ]->[<cellname1>]->...[<cellnamek>]->0
 *                           |               |                 |
 *                           |              ...               ...
 *                           |
 *                       [db_deflist ]->[ ]->[ ]->...[db_deflist]->0
 *                                  |    |    |
 *                                  |    |  [db_inst]
 *                                  |  [db_line]
 *                                [db_rect]
 *
 *
 */

typedef struct coord_pair {
    int x,y;
} PAIR;

typedef struct coord_list {
    PAIR coord;
    struct coord_list *next;
} COORDS;

/********************************************************/

struct db_tab {
    char *name;             /* cell name */
    struct db_deflist *db;  /* pointer to cell definitions */
    struct db_tab *next;    /* to link to another */
} DB_TAB;

static struct db_tab *HEAD = 0;	/* master symbol table pointers */
static struct db_tab *TAIL = 0;

typedef struct db_rect {
    int layer;
    int x1,y1,x2,y2;
} foo1; 

typedef struct db_inst {
    struct db_tab *def;
    int x,y;
} foo2; 

typedef struct db_note {
    int layer;
    char *text;
    int x1,y1;
} foo3; 

typedef struct db_text {
    int layer;
    char *text;
    int x1,y1;
} foo4; 

typedef struct db_line {
    int layer;
    COORDS *coords;
} foo5; 

typedef struct db_circ {
    int layer;
    int x1,y1,x2,y2;
} foo6; 

typedef struct db_poly {
    int layer;
    COORDS *coords;
} foo7; 

/********************************************************/

struct db_deflist {
    short   type;
    union {
        struct db_rect *r;  /* rectangle definition */
        struct db_text *t;  /* text definition */
        struct db_line *l;  /* line definition */
        struct db_circ *c;  /* circle definition */
        struct db_poly *p;  /* polygon definition */
        struct db_inst *i;  /* instance call */
    } u;
    struct db_deflist *next;    /* to link to another */
};
            


/* db_lookup */
/* db_install */
/* db_purge */   
/* db_add */

/********************************************************/

struct db_tab *db_lookup(s)           /* find s in db */
char *s;
{
    struct db_tab *sp;

    for (sp=HEAD; sp!=(struct db_tab *)0; sp=sp->next)
        if (strcmp(sp->name, s)==0)
            return sp;
    return 0;               	/* 0 ==> not found */
} 

struct db_tab *db_install(s)		/* install s in db */
char *s;
{
    struct db_tab *sp;
    char *emalloc();

    sp = (struct db_tab *) emalloc(sizeof(struct db_tab));
    sp->name = emalloc(strlen(s)+1); /* +1 for '\0' */
    strcpy(sp->name, s);

    sp->next = HEAD; /* put at front of list */
    return (HEAD = sp);
}

int db_print()           /* print db */
{
    struct db_tab *sp;

    for (sp=HEAD; sp!=(struct db_tab *)0; sp=sp->next) {
        printf("edit ");
	if (sp->db != (struct db_deflist *) 0) {
	    /* db_def_print(sp->db_deflist); */
	}
        printf("%s;\n",sp->name);
        printf("save;\n");
        printf("exit;\n\n");
    }
    return 0;
}

char *emalloc(n)    /* check return from malloc */
unsigned n;
{
    char *p, *malloc();

    p = malloc(n);
    if (p == 0)
        fprintf(stderr, "out of memory", (char *) 0);
    return p;
}   

main () {
    db_install("1foo");
    db_install("2xyz");
    db_install("3abc");
    db_print();
}


