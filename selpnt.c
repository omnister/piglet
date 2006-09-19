#include "db.h"


SELPNT *selpnt_new(double *x,double *y, DB_DEFLIST *pdef)
{
    SELPNT *tmp;
    tmp = (SELPNT *) malloc(sizeof(SELPNT));
    tmp->p = pdef;
    tmp->xsel = x;
    tmp->ysel = y;
    tmp->xselorig = (x==NULL)?0.0:(*x);
    tmp->yselorig = (y==NULL)?0.0:(*y);
    tmp->next = NULL;
    tmp->prev = tmp;	/* prev pointer points at last */
    return(tmp);
}

void selpnt_clear(SELPNT **selhead) {
    SELPNT *tmp1;
    SELPNT *tmp2;
    tmp1 = *selhead;
    while (tmp1 != NULL) {
	tmp2 = tmp1->next;
	free(tmp1);
        tmp1 = tmp2;
    }
    *selhead = NULL;
}

void selpnt_print(SELPNT **selhead) {
    SELPNT *tmp1;
    int i=0;
    tmp1 = *selhead;
    printf("------------------\n");
    while (tmp1 != NULL) {
        printf("%d: %g %g %g %g\n", i++,
		(tmp1->xsel == NULL)?0.0:(*(tmp1->xsel)),
		(tmp1->ysel == NULL)?0.0:(*(tmp1->ysel)),
		(tmp1->xselorig), (tmp1->yselorig));
	tmp1 = tmp1->next;
    }
}


void selpnt_save(SELPNT **selhead, double *x, double *y, DB_DEFLIST *pdef) {
    SELPNT *tmp;
    if (*selhead == NULL) {
        *selhead = selpnt_new(x,y,pdef);
    } else {
	tmp = (*selhead)->prev;
	tmp->next = selpnt_new(x,y,pdef);
	tmp->next->prev = tmp;
	(*selhead)->prev = tmp->next;
    }
}

/*
int main() {
    SELPNT *selhead = NULL;
    double px = 2.0;
    double py = 4.2;
    selpnt_save(&selhead, &px, &py, NULL);
    py = 9.2;
    selpnt_save(&selhead, NULL, &py, NULL);
    selpnt_save(&selhead, NULL, NULL, NULL);
    px = 6.0;
    selpnt_save(&selhead, &px, &py, NULL);
    selpnt_print(&selhead);
    selpnt_clear(&selhead); 
    selpnt_print(&selhead);
    py = 9.2;
    selpnt_save(&selhead, NULL, &py, NULL);
    selpnt_save(&selhead, NULL, NULL, NULL);
    px = 6.0;
    selpnt_save(&selhead, &px, &py, NULL);
    selpnt_print(&selhead);
    selpnt_clear(&selhead); 

    return(0);
}
*/
