/* #include "eprintf.h" */

#define NULL 0

typedef double NUM; 

typedef struct coord_pair {
    NUM x,y;
} PAIR;

typedef struct coord_list {
    PAIR coord;
    struct coord_list *next;
} COORDS;

PAIR tmp_pair;
COORDS *first_pair, *last_pair; 

COORDS *coord_create(x,y)
double x,y;
{
    COORDS *tmp;
    tmp = (COORDS *) emalloc(sizeof(COORDS));
    tmp->coord.x = x;
    tmp->coord.y = y;
    tmp->next = NULL;
    return(tmp);
}


void coord_new(x,y)
double x,y;
{
    COORDS *temp;
    first_pair = last_pair = coord_create(x,y);
}

void coord_append(x,y)
double x,y;
{
    last_pair = last_pair->next = coord_create(x,y);
}

void coord_swap_last(x,y)
double x,y;
{
    last_pair->coord.x = x;
    last_pair->coord.y = y;
}


void print_pairs()
{	
    COORDS *tmp;
    int i;

    i=1;
    tmp = first_pair;
    while(tmp != NULL) {
	printf(" %g,%g", tmp->coord.x, tmp->coord.y);
	if ((tmp = tmp->next) != NULL && (++i)%4 == 0) {
	    printf("\n");
	}
    }
    printf(";\n");
}		

/* test harness */
main() 
{
	coord_new(1.0,2.0);
	coord_append(3.0, 4.0);
	coord_append(5.0, 6.0);
	coord_append(7.0, 8.0);
	print_pairs();
	coord_swap_last(9.0, 10.0);
	print_pairs();
}
