#include "readfont.h"
#include "db.h"
#include <stdio.h>

    /* int getc(FILE *stream); */
    /* int ungetc(int c, FILE *stream); */

int line=1;

int dx[2];		/* size of font cell */
int dy[2];		/* size of font cell */	
int fonttab[2][256];
int xdef[2][5000];
int ydef[2][5000];


writestring(s,xf, id, bb, mode)
char *s;
XFORM *xf;
int id; 	/* font id */
BOUNDS *bb;
int mode;
{
    double offset=0.0;
    int debug=0;
    
    if (debug) printf("in writestring id=%d\n", id);

    while(*s != 0) {
	writechar(*s,
		(((double)(dx[id]))*0.80*offset)/((double)(dy[id])),
		0.0,xf,id, bb, mode);
	offset+=1.0;
	++s;
    }
}

writechar(c,x,y,xf,id,bb,mode)
int c;
double x;
double y;
XFORM *xf;
int id;			/* font id */
BOUNDS *bb;
int mode;		/* drawing mode */
{
    int i;
    double xp,yp,xt,yt;

    /* printf("# %c %d\n",c); */
    
    if (c==' ') {
	return(0);
    }

    i = fonttab[id][c];

    jump();
    while (xdef[id][i] != -64 || ydef[id][i] != -64) {
	if (xdef[id][i] != -64) {

	    xp = x + (0.8 * ( (double) xdef[id][i] / (double) dy[id]));
	    yp = y + (0.8 * ( (double) ydef[id][i] / (double) dy[id]));

	    xt = xp*xf->r11 + yp*xf->r21 + xf->dx;
	    yt = xp*xf->r12 + yp*xf->r22 + xf->dy;

	    draw(xt,yt, bb, mode); 

	} else {
	    
	    jump();
	}
	i++;
    }
}

loadfont(file, id)
char *file;
int id;
{
    FILE *fp;
    int i;
    int x;
    int y;
    int done;
    int next;
    int lit;
    extern int line;
    int index=0;	/* index into font table */

    /* initialize font table */
    for (i=0; i<=5000; i++) {
	xdef[id][i] = ydef[id][i] = -64;
    }
    for (i=0; i<=255; i++) {
	fonttab[id][i]=-1;
    }

    if((fp=fopen(file,"r")) == NULL) {
	fprintf(stderr, "error: fopen call failed\n");
	exit(1);
    }

    line=0;
    done=0;

    /* note reversed order of arguments */
    next=getxy(fp,&dy[id],&dx[id]);

    /* printf("got %d, %d next=%c\n", x,y,next); */

    /* make first line look properly terminated */
    x=-64;
    y=-64;

    while (!done) {

	if (next == '\n') {
	    line++;
	    getc(fp);

	    if (x==-64 && y==-64) {
		if ((lit=eatwhite(fp)) != EOF) {
		    /* printf("lit=%c\n",lit); */
		    getc(fp);
		    fonttab[id][(int) lit] = index;
		} else {
		    done++;
		}
	    }

	} else if (next == EOF) {
	    done++;
	}

	if (!done) {
	    next=getxy(fp,&x,&y);
	    /* printf("line %d: got %d, %d next=%c\n", line, x,y,next);  */
	    xdef[id][index] = x;
	    ydef[id][index] = y;
	    index++;
	}
    }
}


getxy(fp,px,py)
FILE *fp;
int *px;
int *py;
{
    int c;
    extern int line;

    c=eatwhite(fp);
    /* printf("eating white, next=%c\n",c); */
    if(getint(fp,px) != 1) {
	fprintf(stderr,"error at line %d: expected a digit\n", line);
	exit(3);
    };		

    eatwhite(fp);
    if ((c=getc(fp)) != ',') {
	fprintf(stderr,"error at line %d: expected a comma\n", line);
	exit(2);
    }

    eatwhite(fp);
    if(getint(fp,py) != 1) {
	fprintf(stderr,"error at line %d: expected a digit\n", line);
	exit(3);
    };		

    return(eatwhite(fp));
}

eatwhite(fp)
FILE *fp;
{
    int c;
    int done=0;

    while (!done) {
	c=getc(fp);
	if (c != ' ' && c != '\t') {
	    done++;
	}
    }
    ungetc(c,fp);
    return(c);
}

/* parse an input of the form [+-][0-9]* */
/* using just getc(fp) and ungetc(c,fp) */
/* returns 0 if no digit found, 1 if successful */

int getint(fp,pi) 	
FILE *fp;
int *pi;
{
    int c;
    int state=0;
    int done=0;
    int sign=1;
    int val=0;
    int err=0;
    
    state=0;

    while (!done) {
	c = getc(fp);
	switch (state) {
	    case 0:	/* exponent sign */
		if (c=='+') {
		    state=1;
		} else if (c=='-') {
		    sign=-1;
		    state=1;
		} else if (isdigit(c)) {
		    ungetc(c,fp);
		    state=1;
		} else {
		    err++;
		    ungetc(c,fp);
		    done++;
		}
		break;
	    case 1:	/* first digit */
		if (isdigit(c)) {
		    val=10*val+(int) (c-'0');	
		    state=2;
		} else {
		    err++;
		    ungetc(c,fp);
		    done++;
		}
		break;
	    case 2:	/* remaining digits */
		if (isdigit(c)) {
		    val=10*val+(int) (c-'0');	
		} else {
		    ungetc(c,fp);
		    done++;
		}
		break;
	}
    }

    if (err) {
	return(0);
    } 

    *pi = val*sign;
    return(1);
}


