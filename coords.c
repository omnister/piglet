#include "db.h"

COORDS *coord_new(NUM x, NUM y);
COORDS *coord_copy(COORDS *CP);
void coord_append(COORDS *CP, NUM x, NUM y);
void coord_swap_last(COORDS *CP,  NUM x, NUM y);
void coord_drop(COORDS *CP);
void coord_print(COORDS *CP);
int  coord_count(COORDS *CP);

int coord_get(CP, n, px, py) 	/* get nth set of x,y coords in list */
COORDS *CP;
int n;
double *px, *py;
{	
    COORDS *tmp;
    int i;
    int err=0;

    i=1;
    tmp = CP;
    while(i<n && (tmp = tmp->next) != NULL) {
	i++;
    }
    if (i!=n) {
    	err = -1;
    } else {
    	*px = tmp->coord.x;
    	*py = tmp->coord.y;
    }
    return(err);
}		

int coord_count(CP) 	/* return number of coords in list */
COORDS *CP;
{	
    COORDS *tmp;
    int i;

    i=1;
    tmp = CP;
    while((tmp = tmp->next) != NULL) {
	i++;
    }
    return(i);
}		

COORDS *coord_new(x,y)
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

void coord_append(CP, x,y)
COORDS *CP;
double x,y;
{
    COORDS *tmp;
    tmp = CP->prev;	/* save pointer to last coord */
    tmp->next = coord_new(x,y);
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

COORDS *coord_copy(CP)
COORDS *CP;
{
    COORDS *p;
    COORDS *new_coords; 
    int i;

    if (CP == NULL) {
    	printf("coord_copy: can't copy null coordinate list\n");
    } else {
	p=CP;
	new_coords = coord_new(p->coord.x,p->coord.y);
	p=p->next;
	while(p != NULL) {
	    coord_append(new_coords, p->coord.x, p->coord.y);
	    p=p->next;
	}
    }
    return(new_coords);
}

/* test harness */

/*
main() 
{
    COORDS *CP;
    COORDS *NP;
    double x,y;

    CP = coord_new(1.0,2.0);
    coord_append(CP,3.0, 4.0);
    coord_append(CP,5.0, 6.0);
    coord_append(CP,7.0, 8.0);
    coord_drop(CP);
    coord_print(CP);
    coord_swap_last(CP,9.0, 10.0);
    coord_print(CP);
    coord_print(coord_copy(CP));
    printf("num coords = %d\n", coord_count(CP));
    coord_get(CP, 1, &x, &y);
    printf("1st coord is = %g,%g\n",x,y);
    coord_get(CP, 2, &x, &y);
    printf("2nd coord is = %g,%g\n",x,y);
    coord_get(CP, 3, &x, &y);
    printf("3rd coord is = %g,%g\n",x,y);
}
*/
