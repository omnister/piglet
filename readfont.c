#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "readfont.h"
#include "readshpfont.h"
#include "db.h"
#define MAXPOINT 20000

/* 

The TEXT and FONT files are named "TEXTDATA.F" and "NOTEDATA.F"
respectively.  The format for both files is plain ASCII.  The file
starts with two integers separated by a comma.  The first gives the
height of each character definition and the second, the width.  The
second line has two more integers separated by a comma.  The first is
either 1 or 0 and indicates if font is proportional (1) or is a fixed
width font (0), and the second number is an integer that gives the fixed
width size of the font. 

Each character is then listed in printed form and the glyph is defined
by a set of vector coordinates.  It is assumed that each successive x,y
coordinate pair is connected by a stroke unless the path is broken by
the special coordinate "-128,0".  Each definition is terminated by the
special coordinate "-128,-128", followed by a proportional width to be
used for this character. Here is an example of the first three
definitions in "NOTEDATA.F" for the characters <exclam>, <double-quote>,
and <pound>:

16,12
1,12
! 6,2 6,3 -128,0 6,5 6,14 -128,-128 12
" 4,14 4,10 -128,0 8,14 8,10 -128,-128 12
# 2,6 10,6 -128,0 2,10 10,10 -128,0 4,2 4,14 -128,0 8,2 8,14 -128,-128 12

*/


int getxy();
int eatwhite();
int getint();

    /* int getc(FILE *stream); */
    /* int ungetc(int c, FILE *stream); */

int line=1;

static int dx[2];		/* size of font cell */
static int dy[2];		/* size of font cell */	
static int fonttab[2][256];	/* starting index of each character */

static int xdef[2][MAXPOINT];
static int ydef[2][MAXPOINT];

int fillable(int fid) {
   return(fid);
}

void writechar(
    int c,
    double x,
    double y,
    XFORM *xf,
    int fid,		/* font id */
    BOUNDS *bb,
    int mode		/* drawing mode */
) {
    int i;
    double xp,yp,xt,yt;
    int debug=0;

    // BUG fails if fid==0;

    if (c==' ') {
	return;
    }

    i = fonttab[fid][c];
    if (debug) fprintf(stderr, "writechar fid=%d, c=%c, font index = %d\n", fid, c, i);

    jump(bb, mode);
    if (fillable(fid)) {
	startpoly(bb,mode);
    }
    while (xdef[fid][i] != -128 || ydef[fid][i] != -128) { // -128,-128 == END 
	if (xdef[fid][i] != -128) {			 // end of polygon 
	    xp = x + (0.8 * ( (double) xdef[fid][i] / (double) dy[fid]));
	    yp = y + (0.8 * ( (double) ydef[fid][i] / (double) dy[fid]));
	    xt = xp*xf->r11 + yp*xf->r21 + xf->dx;
	    yt = xp*xf->r12 + yp*xf->r22 + xf->dy;
	    draw(xt,yt, bb, mode); 
	} else {
	    if (fillable(fid)) {
		endpoly(bb,mode);
	    }
	    jump(bb, mode);
	    if (fillable(fid)) {
		 startpoly(bb,mode);
	    }
	}
	i++;
    }
    if (fillable(fid)) {
	endpoly(bb,mode);
    }
}


void writestring(
    char *s,
    XFORM *xf,
    int id, 	/* font id */
    int jf, 	/* font justification */
    BOUNDS *bb,
    int mode
) {
    double yoffset=0.0;
    double xoffset=0.0;
    double xoff, yoff;
    int fid;
    int debug=0;

    if (id==NOTE_MODE) {
      fid=0;
    } else {
      fid=1;
    }

    if (debug) fprintf(stderr, "writestring %s id=%d\n",s, id);
    
    if (id==NOTE_MODE || id ==TEXT_MODE) {				// 0 or 1
	if (debug) fprintf(stderr, "is legacy font\n");
   	;	// good to go... 
    } else if (shp_fontexists(id))  {
    	shp_writestring(s,xf,id,jf,bb,mode);
	return;
    } else {					// not loaded
	fprintf(stderr, "no font loaded at position %d, defaulting to NOTE\n", id);
	id = NOTE_MODE;
    }

    if (debug) printf("in writestring fid=%d\n", fid);
   
    double width=(((double)(dx[fid]))*0.80*strlen(s))/((double)(dy[fid]));
    double height=0.8;
    xoff = yoff = 0.0;

    switch (jf) {
        case 0:		/* SW */
	    xoff = 0.0;
	    yoff = 0.0;
	    break;
        case 1:		/* S */
	    xoff = -width/2.0;
	    yoff = 0.0;
	    break;
        case 2:		/* SE */
	    xoff = -width;
	    yoff = 0.0;
	    break;
        case 3:		/* W */
	    xoff = 0.0;
	    yoff = -height/2.0;
	    break;
        case 4:		/* C */
	    xoff = -width/2.0;
	    yoff = -height/2.0;
	    break;
        case 5:		/* E */
	    xoff = -width;
	    yoff = -height/2.0;
	    break;
        case 6:		/* NW */
	    xoff = 0.0;
	    yoff = -height;
	    break;
        case 7:		/* N */
	    xoff = -width/2.0;
	    yoff = -height;
	    break;
        case 8:		/* NE */
	    xoff = -width;
	    yoff = -height;
	    break;
	default:
	    printf("bad justification: %d in writestring()\n", jf);
	    break;
    }


    /* void writechar(c,x,y,xf,fid,bb,mode) */

    while(*s != 0) {
	if (*s != '\n') {
	    writechar(*s,
	        (((double)(dx[fid]))*0.80*xoffset)/((double)(dy[fid]))+xoff,
		-yoffset+yoff,xf,fid, bb, mode);
	    if (debug) printf("writing %c, dx:%d dy:%d fid:%d\n",
	    	*s, dx[fid], dy[fid], fid);
	    xoffset+=1.0;
	} else {
	    xoffset=0.0;
	    yoffset+=1.0; 
	}
	++s;
    }
}

void loadfont(char *file, int fid)
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
    int w, fw, is_proportional;

    if (debug) fprintf(stderr, "loading font %s fid %d\n", file, fid);

    /* initialize font table */
    for (i=0; i<MAXPOINT; i++) {
	xdef[fid][i] = ydef[fid][i] = -128;
    }
    for (i=0; i<=255; i++) {
	fonttab[fid][i]=-1;
    }

    if((fp=fopen(file,"r")) == NULL) {
	fprintf(stderr, "error: fopen call failed\n");
	exit(1);
    } else {
    	printf("loading %s\n",file);
    }

    line=0;
    done=0;

    /* note reversed order of arguments */

    next=getxy(fp,&dy[fid],&dx[fid]);
    next=getxy(fp,&is_proportional,&fw);

    for (i=0; i<=255; i++) {
        //width[i]=fw;
    }

    /* make first line look properly terminated */
    x=-128;
    y=-128;

    while (!done) {

	if (x==-128 && y==-128) {
	    if (getint(fp,&w)) {
		// width[(int) lit]=w;
	    }
	    if ((lit=eatwhite(fp)) != EOF) {
		/* printf("lit=%c\n",lit); */
		getc(fp);
		fonttab[fid][(int) lit] = index;
		if (debug) fprintf(stderr, "loadfont at fid:%d lit:%d index%d\n", fid, (int)lit, index);
		//valid[(int) lit]++;
	    } else {
		done++;
	    }
	} else if (next == EOF) {
	    done++;
	} 

	if (!done) {
	    next=getxy(fp,&x,&y);
	    /* printf("line %d: got %d, %d next=%c\n", line, x,y,next);  */
	    xdef[fid][index] = x;
	    ydef[fid][index] = y;
	    index++;
	}
    }
    if (debug) printf("index = %d\n", index);
}


int getxy(FILE *fp, int *px, int *py)
{
    int c;
    extern int line;
    int ret;

    c=eatwhite(fp);
    if((ret=getint(fp,px)) != 1) {
	fprintf(stderr,"error at line %d: expected a digit2: %c <%d>\n", line, c, ret);
	exit(3);
    };		

    eatwhite(fp);
    if ((c=getc(fp)) != ',') {
	ungetc(c,fp); /* make comma optional to read graffy fonts */
	/* fprintf(stderr,"error at line %d: expected a comma\n", line); exit(2); */
    }

    eatwhite(fp);
    if(getint(fp,py) != 1) {
	fprintf(stderr,"error at line %d: expected a digit3n", line);
	exit(3);
    };		

    return(eatwhite(fp));
}

int eatwhite(FILE *fp)
{
    int c;

    while ((c=getc(fp)) != EOF) {
	if (c=='\n') {
	    line++;
	}
	if (!isspace(c)) {
	    break;
	}
    }
    ungetc(c,fp);
    return(c);
}

int old_eatwhite(FILE *fp)
{
    int c;
    int done=0;

    while (!done) {
	c=getc(fp);
	if (c=='\n') {
	    line++;
	}
	if (c != ' ' && c != '\t' && c != '\n') {
	    done++;
	}
    }
    ungetc(c,fp);
    return(c);
}

/* parse an input of the form [+-][0-9]* */
/* using just getc(fp) and ungetc(c,fp) */
/* returns 0 if no digit found, 1 if successful */

int getint(FILE *fp, int *pi) 	
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


