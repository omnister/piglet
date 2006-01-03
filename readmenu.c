#define _GNU_SOURCE
#include <stdio.h>

#include "readmenu.h"
#include "db.h"

#define MAX 16
int getcolor();

/*
int testmain() {
    int i,n;

    MENUENTRY menutab[200];
    int linewidth;
    n=loadmenu(menutab, 200, "MENUDATA_V", &linewidth);

    for (i=1; i<=n; i++) {
    	printf("%d: %d %d %d %d\t\"%s\"\n", 
	i, menutab[i].color, menutab[i].xloc, menutab[i].yloc, menutab[i].width, menutab[i].text);
    }
    printf("%d\n", linewidth);

    return(0);
}
*/

int loadmenu(m, maxcell, s, lw, mr) 
MENUENTRY m[];	/* structure for storing the information */
int maxcell;		/* maximum number of menu cells allocated */
char *s;		/* name of menu file */
int *lw;		/* max columns */
int *mr;		/* max rows */
{

    char * line = NULL;
    FILE *fp;
    int n;
    size_t length;
    char *p;
    int x, y;
    int k, w;
    char color[MAX];
    int err=0;
    int count;
    int c;
    int width;


    if ((fp = fopen(s, "r")) == NULL) {
	printf("can't open menu file %s\n", s);
    	exit(1);
    }

    count=0;
    y=0;
    w=0;
    while((n = getline(&line, &length, fp)) != -1) {
        line[n-1] = '\0';  	/* eliminate the newline */
	x=0;	/* xlocation of start of string on line */
	k=0;	/* index of field */
        if (index(line, '|') != NULL) {
	    p = strtok(line, "|");
	    if (strlen(p) > MAX) {
	    	printf("error on line %d, color field is wider than %d\n", x, MAX);
		err++;
	    }
	    strncpy(color, p, MAX);
	    /* printf("%d: %s|", y, p); */
	    while ((p=strtok(NULL, "|")) != NULL) {
		k++;
		if (count++ >= maxcell) {
		    printf("warning: too many cells in menu definition, truncating at %d!\n", maxcell);
		    break;
		};
		w=strlen(p)+1;
		c = getcolor(color,k);
		/* printf("%d: (%d,%d,%d,%d)\t\"%s\"\n",count, x,y,w, c, p); */
		m[count].xloc = x;
		m[count].yloc = y;
		m[count].width = w;
		m[count].color = c;
		m[count].text = strsave(p);

		x+=w;
	    }
	    if (y==0) {
	    	width = x;	
		*lw = width;
	    } else {
	    	if (x != width) {
		    printf("warning: line width of line %d (%d) does not match first line (%d)\n",
		    y, x, width);
		}
	    }
	    y++;
	} else {
	    ; /* ignore all other lines */
	}
	*mr = y;
    }
    return(count);
}

int getcolor(s,k) 
char *s;  /* a color string */
int k;	  /* index of color */
{
    int i;
    int n=0;
    int c;
    for (i=0; i<=strlen(s); i++) {
    	switch(s[i]) {
	    case 'a':	/* aqua */
		c=5;
		n++;
	    	break;
	    case 'b':	/* blue */
		c=4;
		n++;
	    	break;
	    case 'g':	/* green */
		c=3;
		n++;
	    	break;
	    case 'p':	/* purple */
	    case 'm':	/* magenta */
		c=6;
		n++;
	    	break;
	    case 'r':	/* red */
		c=2;
		n++;
	    	break;
	    case 'w':	/* white */
		c=1;
		n++;
	    	break;
	    case 'y':	/* yellow */
		c=7;
		n++;
	    	break;
	    default:
	    	break;
	}
	if (n==k) {
	    return(c);
	}
    }
    return(0);
}

