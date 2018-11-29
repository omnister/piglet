#include "db.h"

COORDS *coord_new(NUM x, NUM y);
// COORDS *coord_copy(XFORM *xp, COORDS *CP);
COORDS *coord_append(COORDS *CP, NUM x, NUM y);
void coord_swap_last(COORDS *CP,  NUM x, NUM y);
void coord_drop(COORDS *CP);
void coord_print(COORDS *CP);
int  coord_count(COORDS *CP);
int  coord_get(COORDS *CP, int n, NUM *px, NUM *py);
int  coord_edit(COORDS *CP, int n, NUM x, NUM y); 

// perhaps all the void functions should instead return pointers
// to the modified list?


// if n>0;  edits nth coord from beginning counting 1, 2, 3
// if n<=0; edits nth coord from end counting 0, -1, -2, -3...

int coord_get(COORDS *CP, int n, NUM *px, NUM *py) 	/* get nth set of coords */
{	
    COORDS *tmp;
    int i;
    int err=0;

    if (n>0) {	// positive counts from beginning
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
    } else {	// negative counts from end
	i=n;
	tmp = CP->prev;
	while(i<0 && (tmp = tmp->prev) != NULL) {
	    i++;
	}
	if (i!=0) {
	    err = -1;
	} else {
	    *px = tmp->coord.x;
	    *py = tmp->coord.y;
	}
    }
    return(err);
}		

// if n>0;  gets nth coord from beginning counting 1, 2, 3
// if n<=0; gets nth coord from end counting 0, -1, -2, -3...

int coord_edit(COORDS *CP, int n, NUM x, NUM y) 	/* mod nth set of coords */
{	
    COORDS *tmp;
    int i;
    int err=0;

    if (n>0) {	// positive counts from beginning
	i=1;
	tmp = CP;
	while(i<n && (tmp = tmp->next) != NULL) {
	    i++;
	}
	if (i!=n) {
	    err = -1;
	} else {
	    tmp->coord.x = x;
	    tmp->coord.y = y;
	}
    } else {	// negative counts from end
	i=n;
	tmp = CP->prev;
	while(i<0 && (tmp = tmp->prev) != NULL) {
	    i++;
	}
	if (i!=0) {
	    err = -1;
	} else {
	    tmp->coord.x = x;
	    tmp->coord.y = y;
	}
    }
    return(err);
}		

int coord_count(COORDS *CP) 	/* return number of coords in list */
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


// Piglet uses this doubly linked structure alot
//
// It bears a short description every time 
//
// We have a doubly linked list of x,y nodes
// COORDS->next points to the next one, or null at end
// COORDS->prev points to the previous one, or the last one if first
//
// Cool, eh?  Can append at both ends with at most a double redirect
//

COORDS *coord_new(double x,double y)
{
    COORDS *tmp;
    tmp = (COORDS *) malloc(sizeof(COORDS));
    tmp->coord.x = x;
    tmp->coord.y = y;
    tmp->next = NULL;
    tmp->prev = tmp;	/* prev pointer points at last */
    return(tmp);
}

COORDS *coord_append(COORDS *CP, double x,double y)
{
    COORDS *tmp;
    if (CP == NULL) {
	tmp = coord_new(x,y);
    } else {
	tmp = CP->prev;	/* save pointer to last coord */
	tmp->next = coord_new(x,y);
	tmp->next->prev = tmp;
	CP->prev = tmp->next;
	tmp=CP;
    }
    return (tmp);
}

void coord_swap_last(COORDS *CP, double x,double y)
{
    CP->prev->coord.x = x;
    CP->prev->coord.y = y;
}

void coord_drop(COORDS *CP)
{
    COORDS *tmp;
    
    tmp = CP->prev;		/* last in list */
    CP->prev = tmp->prev;
    tmp->prev->next = NULL;
    free((COORDS *) tmp);
}

void coord_print(COORDS *CP)
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

/* copies a coordinate list with optional transform */
/* if xp is NULL, no transform done */

COORDS *coord_copy(XFORM *xp, COORDS *CP)
{
    COORDS *p;
    COORDS *new_coords=NULL; 
    double x,y;

    if (CP == NULL) {
    	printf("coord_copy: can't copy null coordinate list\n");
    } else if (xp == NULL) {
	p=CP;
	new_coords = coord_new(p->coord.x,p->coord.y);
	p=p->next;
	while(p != NULL) {
	    coord_append(new_coords, p->coord.x, p->coord.y);
	    p=p->next;
	}
    } else {
	p=CP;
	x = p->coord.x; y = p->coord.y; xform_point(xp, &x, &y);
	new_coords = coord_new(x, y);
	p=p->next;
	while(p != NULL) {
	    x = p->coord.x; y = p->coord.y; xform_point(xp, &x, &y);
	    coord_append(new_coords, x, y);
	    p=p->next;
	}
    }
    return(new_coords);
}

/* test harness */

/*
int main() 
{
    COORDS *CP;
    double x,y;

    CP = coord_new(1.0,2.0);
    coord_append(CP,3.0, 4.0);
    coord_append(CP,5.0, 6.0);
    coord_append(CP,7.0, 8.0);
    coord_drop(CP);
    coord_print(CP);
    printf("doing coord_swap_last(CP,9,10)\n");
    coord_swap_last(CP,9.0, 10.0);
    coord_print(CP);
    printf("doing coord_edit(CP,0, 3,0)\n");
    coord_edit(CP,0,3,0);
    coord_print(CP);


    coord_print(coord_copy(NULL,CP));
    printf("num coords = %d\n", coord_count(CP));
    coord_get(CP, 1, &x, &y);
    printf("1st coord is = %g,%g\n",x,y);
    coord_get(CP, 2, &x, &y);
    printf("2nd coord is = %g,%g\n",x,y);
    coord_get(CP, 3, &x, &y);
    printf("3rd coord is = %g,%g\n",x,y);
    coord_edit(CP, 2, 20,30); 
    coord_get(CP, 2, &x, &y);
    printf("2nd coord is = %g,%g\n",x,y);
    exit(1);
}
*/
