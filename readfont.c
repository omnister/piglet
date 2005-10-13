#include <ctype.h>
#include <stdio.h>

#include "readfont.h"
#include "db.h"
#define MAXPOINT 5000

int getxy();
int eatwhite();
int getint();

    /* int getc(FILE *stream); */
    /* int ungetc(int c, FILE *stream); */

int line=1;

int dx[2];		/* size of font cell */
int dy[2];		/* size of font cell */	
int fonttab[2][256];

int xdef[2][MAXPOINT];
int ydef[2][MAXPOINT];

int fillable[2] = {0,0};

void writechar(c,x,y,xf,id,bb,mode)
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
	return;
    }

    i = fonttab[id][c];

    jump(bb, mode);
    if (fillable[id]) {
	startpoly(bb,mode);
    }
    while (xdef[id][i] != -64 || ydef[id][i] != -64) {		/* -64,-64 == END */
	if (xdef[id][i] != -64) {				/* end of polygon */
	    xp = x + (0.8 * ( (double) xdef[id][i] / (double) dy[id]));
	    yp = y + (0.8 * ( (double) ydef[id][i] / (double) dy[id]));
	    xt = xp*xf->r11 + yp*xf->r21 + xf->dx;
	    yt = xp*xf->r12 + yp*xf->r22 + xf->dy;
	    draw(xt,yt, bb, mode); 
	} else {
	    if (fillable[id]) {
		endpoly(bb,mode);
	    }
	    jump(bb, mode);
	    if (fillable[id]) {
		 startpoly(bb,mode);
	    }
	}
	i++;
    }
    if (fillable[id]) {
	endpoly(bb,mode);
    }
}


void writestring(s,xf, id, bb, mode)
char *s;
XFORM *xf;
int id; 	/* font id */
BOUNDS *bb;
int mode;
{
    double yoffset=0.0;
    int debug=0;
    double xoffset=0.0;
    
    if (debug) printf("in writestring id=%d\n", id);

    /* void writechar(c,x,y,xf,id,bb,mode) */

    while(*s != 0) {
	if (*s != '\n') {
	    writechar(*s,
	        (((double)(dx[id]))*0.80*xoffset)/((double)(dy[id])),
		-yoffset,xf,id, bb, mode);
	    if (debug) printf("writing %c, dx:%d dy:%d id:%d\n",
	    	*s, dx[id], dy[id], id);
	    xoffset+=1.0;
	} else {
	    xoffset=0.0;
	    yoffset+=1.0; 
	}
	++s;
    }
}

void loadfont(file, id)
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
    int debug=0;

    /* initialize font table */
    for (i=0; i<MAXPOINT; i++) {
	xdef[id][i] = ydef[id][i] = -64;
    }
    for (i=0; i<=255; i++) {
	fonttab[id][i]=-1;
    }

    if((fp=fopen(file,"r")) == NULL) {
	fprintf(stderr, "error: fopen call failed\n");
	exit(1);
    } else {
    	printf("loading %s from disk\n",file);
    }

    line=0;
    done=0;

    /* note reversed order of arguments */

    next=getxy(fp,&dy[id],&dx[id]);
    /* printf("got %d, %d next=%x id=%d\n", dx[id],dy[id],next, id); */

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
    if (debug) printf("index = %d\n", index);
}


int getxy(fp,px,py)
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

int eatwhite(fp)
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


