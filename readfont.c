#include "readfont.h"
#include "db.h"
#include <stdio.h>

    /* int getc(FILE *stream); */
    /* int ungetc(int c, FILE *stream); */

int line=1;

int dx;		/* size of font cell */
int dy;		/* size of font cell */	
int fonttab[256];
int xdef[5000];
int ydef[5000];


/*
main()
{
    loadfont("NOTEDATA.F");
    writestring("Now is the time",0,0,scale);
    writestring("for all good men",0,-1*(int) scale,scale);
    writestring("to come to the",0,-2*(int) scale,scale);
    writestring("ABCDEFGHIJKLMNOP",0,-3*(int) scale,scale);
    writestring("QRSTUVWXYZabcdef",0,-4*(int) scale,scale);
    writestring("ghijklmnopqrstuv",0,-5*(int) scale,scale);
    writestring("wxyz0123456789~!",0,-6*(int) scale,scale);
    writestring("#$%^&*()_+=-';:",0,-7*(int) scale,scale);
    writestring("][}{.,></?|",0,-8*(int) scale,scale);
}
*/

writestring(s,xf)
char *s;
XFORM *xf;
{
    double offset=0.0;

    while(*s != 0) {
	writechar(*s,(((double)dx)*0.80*offset)/((double) dy),0.0,xf);
	offset+=1.0;
	++s;
    }
}

writechar(c,x,y,xf)
int c;
double x;
double y;
XFORM *xf;
{
    int i;
    double xp,yp,xt,yt;

    /* printf("# %c %d\n",c); */
    
    if (c==' ') {
	return(0);
    }

    i = fonttab[c];

    printf("jump\n");
    while (xdef[i] != -64 || ydef[i] != -64) {
	if (xdef[i] != -64) {

	    xp = x+ (0.8 * (double) xdef[i]) / ((double) dy);
	    yp = y+ (0.8 * (double) ydef[i]) / ((double) dy);

	    xt = xp*xf->r11 + yp*xf->r21 + xf->dx;
	    yt = xp*xf->r12 + yp*xf->r22 + xf->dy; 

	    printf("%g %g\n", xt, yt);
	} else {
	    printf("jump\n");
	}
	i++;
    }
}

loadfont(file)
char *file;
{
    FILE *fp;
    int i;
    int x;
    int y;
    int done;
    int next;
    int lit;
    extern int line;
    extern int dx,dy;
    int index=0;	/* index into font table */

    /* initialize font table */
    for (i=0; i<=5000; i++) {
	xdef[i] = ydef[i] = -64;
    }
    for (i=0; i<=255; i++) {
	fonttab[i]=-1;
    }

    if((fp=fopen(file,"r")) == NULL) {
	fprintf(stderr, "error: fopen call failed\n");
	exit(1);
    }

    line=0;
    done=0;

    /* note reversed order of arguments */
    next=getxy(fp,&dy,&dx);

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
		    fonttab[(int) lit] = index;
		} else {
		    done++;
		}
	    }

	} else if (next == EOF) {
	    done++;
	}

	if (!done) {
	    next=getxy(fp,&x,&y);
	    /* printf("line %d: got %d, %d next=%c\n", line, x,y,next); */
	    xdef[index] = x;
	    ydef[index] = y;
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


