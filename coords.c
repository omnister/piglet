#include "db.h"

COORDS *coord_create(NUM x, NUM y);
COORDS *coord_new(NUM x, NUM y);
void coord_append(COORDS *CP, NUM x, NUM y);
void coord_swap_last(COORDS *CP,  NUM x, NUM y);
void coord_drop(COORDS *CP);
void coord_print(COORDS *CP);

COORDS *coord_create(x,y)
double x,y;
{
    COORDS *tmp;
    tmp = (COORDS *) malloc(sizeof(COORDS));
    tmp->coord.x = x;
    tmp->coord.y = y;
    tmp->next = NULL;
    tmp->prev = tmp;	/* prev pointer points at last */
    return(tmp);
}

COORDS *coord_new(x,y)
double x,y;
{
    return(coord_create(x,y));
}

void coord_append(CP, x,y)
COORDS *CP;
double x,y;
{
    COORDS *tmp;
    tmp = CP->prev;	/* save pointer to last coord */
    tmp->next = coord_create(x,y);
    tmp->next->prev = tmp;
    CP->prev = tmp->next;
}

void coord_swap_last(CP, x,y)
COORDS *CP;
double x,y;
{
    CP->prev->coord.x = x;
    CP->prev->coord.y = y;
}

void coord_drop(CP)
COORDS *CP;
{
    COORDS *tmp;
    
    tmp = CP->prev;		/* last in list */
    CP->prev = tmp->prev;
    tmp->prev->next = NULL;
    free((COORDS *) tmp);
}

void coord_print(CP)
COORDS *CP;
{	
    COORDS *tmp;
    int i;

    i=1;
    tmp = CP;
    while(tmp != NULL) {
	printf(" %g,%g", tmp->coord.x, tmp->coord.y);
	if ((tmp = tmp->next) != NULL && (++i)%4 == 0) {
	    printf("\n");
	}
    }
    printf(";\n");
}		

/* test harness */
/*
main() 
{
    COORDS *CP;

    CP = coord_new(1.0,2.0);
    coord_append(CP,3.0, 4.0);
    coord_append(CP,5.0, 6.0);
    coord_append(CP,7.0, 8.0);
    coord_drop(CP);
    coord_print(CP);
    coord_swap_last(CP,9.0, 10.0);
    coord_print(CP);
}
*/
