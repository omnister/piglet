// Lexer for Autocad .shp font files
// RCW: 08/31/2012 (lexer for kicad)
// RCW: 11/14/2013 (modified for .shp)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define MAXTOK 128		// maximum string length of a token
#define MAXPUSH 4		// max number of pushed back tokens
#define MAXBUF 256

unsigned char *glyphtab[258];		// pointers to byte arrays

typedef enum {
    EOL,
    LP,				// '(' left paren
    RP,				// ')' right paren
    STAR,			// '*' 
    SP,				// space
    NUM,
    WORD,
    QUOTE,
    COMMA,
    UNKNOWN
} TOKEN;

typedef struct tokstruct {
   TOKEN tok;
   char *word;
} TOKSTRUCT;

TOKSTRUCT pushback[MAXPUSH];
int pushidx = 0;

char buf[MAXTOK];
int line = 0;
int debug = 0;

// forward references
TOKEN token_get(char **word);
static char *tok2str(TOKEN t);
char *estrdup(const char *s);
void eatwhite();
int token_unget(TOKEN token, char *word);

int strtonum(char *str) {
    int i;
    int sign=1.0;
    int val;

    i=0;
    if (str[i]=='-') {
	sign=-1.0;
	i++;
    }

    if (str[i]=='0') {
	val = sign*(int) strtol(str+i, NULL, 16);
    } else {
	val = sign*(int) strtol(str+i, NULL, 10);
    }

    // fprintf(stderr, "str2num called with <%s>, ret=%d\n", str, val);

    return (val);
}

void advance(int pendown, double *xloc, double *yloc, double scale,
double x, double y) {
   int debug=0;
   
   if (debug) printf("advance called with xloc: %g, yloc: %g, scale: %g, x:%g, y%g\n", *xloc, *yloc, scale, x, y);
 
   if (!pendown) {
     printf("jump\n");
     // printf("line 2\n");
   } else {
     // printf("line 1\n");
     ;
   }

   *xloc += scale*x;
   *yloc += scale*y;

   printf("%g %g\n", *xloc, *yloc);
}

// -------------------------------------
// push pop stack ADT
//
#define MAXSTACK 100
double xstack[MAXSTACK]; 
double ystack[MAXSTACK]; 
int sp=0;

void push(double *x, double *y) {
    xstack[sp] = *x;
    ystack[sp] = *y;
    sp++;
    if (sp >= MAXSTACK) {
       fprintf(stderr, "overflowed on a push()\n");
       sp = MAXSTACK-1;
       // exit(1);
    }
}

void pop(double *x, double *y) {
    sp--;
    if (sp < 0) {
       fprintf(stderr, "underflowed on a pop()\n");
       sp=0;
       // exit(1);
    }
    *x=xstack[sp];
    *y=ystack[sp];
}

double mag(double x) {
   return (sqrt(x*x));
}

void  bulge_arc( int pendown, double *xx, double *yy, 
	double scale, double p1, double p2, double p3) {

	double xs, xe, ys, ye, xm, ym, dx, dy, xb, yb;
	double alpha;
	double beta;
	double len;
	double radius;
	double mirrorx;
	double theta_start, theta_end, dtheta;
	double sx, cx;
	int debug=0;

	xe=p1;   ye=p2;

	alpha = p3/127.0;	// bulge factor

	if (alpha < 0.0) {	// mirror
	   alpha *= -1.0;
	   mirrorx=-1.0;
	} else {
	   mirrorx=1.0;
	}

	if (debug) fprintf(stderr, "alpha=%g\n", alpha);

	if (alpha <= 1e-6) {	// straight line segment
	    advance(pendown, xx, yy, scale, xe, ye);
	} else {		// bulge arc

	    // to avoid branch cuts in the atan2() function
	    // we generate the arc in unit canonical form 
	    // then rotate them back to the original orientation
	    
	    // length of arc

	    len = sqrt(xe*xe + ye*ye);	// length of original arc
	    sx = ye/len;	// sin() of original arc angle
	    cx = xe/len;	// cos() of original arc angle

	    beta = ((alpha*alpha)-1.0)/(2.0*alpha);

	    // so we consider drawing an arc between
	    // (0,0.5) and (0,-0.5)
	    // with a center of arc at (-beta/2.0, 0)
	    
	    radius = sqrt(beta*beta/4.0 + 0.25);
	    
	    // atan2 returns values in range +/-PI
	    theta_start = atan2(-0.5, -beta/2.0);
	    theta_end   = atan2(0.5, -beta/2.0);
	    dtheta=theta_end-theta_start;

	    if (debug) fprintf(stderr, "ts=%g, te=%g, dt=%g\n",
	    	theta_start, theta_end, dtheta);

	    #define BULGERES 8

	    double xx1, yy1, xx2, yy2;
	    double theta;
	    int i;
	    for (i=0; i<=BULGERES; i++) {
	       xx2 = xx1;
	       yy2 = yy1;
	       theta= theta_start+(dtheta)*((double) i)/BULGERES;
	       dx = mirrorx*radius*len*cos(theta);
	       dy = radius*len*sin(theta);
	       xx1 = dx * sx + dy * cx;
	       yy1 = dy * sx - dx * cx;
	       if (i>0) advance(pendown, xx, yy, scale, xx1-xx2, yy1-yy2);
	    }
	}
}

void dump(unsigned int glyphnum, double *xx, double *yy, double scale) {

    int index=0;
    unsigned int c;
    int pendown = 1;
    int x,y;
    int r,o;
    int done=0;
    int debug=0;
    int p1, p2, p3, p4, p5;
    int skip=0;		// skip next instruction

    int i;
    int octant;
    int numoct; 
    int cw;
    double stheta;
    double dtheta;
    double xx1, yy1, xx2, yy2;

    printf("jump\n");
    printf("%g %g\n", *xx, *yy);

    while(!done) {
	c = glyphtab[glyphnum][index++];
	if (debug) fprintf(stderr,"c=%d\n", c);
	if (skip > 0) skip--;
	switch (c) {
	   case 0:		// end of shape def
	     if (!skip) done++;
	     break;
	   case 1:		// pendown
	     if (!skip) pendown++;
	     break;
	   case 2:		// penup
	     if (!skip) pendown=0;
	     break;
	   case 3:		// divide_by(c)
	     c = glyphtab[glyphnum][index++];
	     scale /= (double)(c);
	     break;
	   case 4:		// multiply_by(c)
	     c = glyphtab[glyphnum][index++];
	     scale *= (double)(c);
	     break;
	   case 5:		// push
	     if (!skip) push(xx, yy);
	     break;
	   case 6:		// pop
	     if (!pendown) {
	        printf("jump\n");
	     }
	     if (!skip) pop(xx, yy);
	     break;
	   case 7:		// draw subshape(c)
	     c = glyphtab[glyphnum][index++];
	     // fprintf(stderr, "subshape not implemented!\n");
	     // draw_subshape((unsigned) c);
	     if (glyphtab[c] != NULL) {
		 if (!skip) dump((unsigned int) c, xx, yy, scale);
	     } else {
	         fprintf(stderr, "called null subshape!\n");
	     }
	     break;
	   case 8:		// x,y displacement
	     x = (signed char) (glyphtab[glyphnum][index++]);
	     y = (signed char) (glyphtab[glyphnum][index++]);
	     // fprintf(stderr,"got %g %g\n", (double) x, (double) y);
	     // printf("pen 8\n");
	     if (!skip) advance(pendown, xx, yy, scale, (double) x,(double) y);
	     break;
	   case 9:		// multi x,y displacement
	     x = (signed char) (glyphtab[glyphnum][index++]);
	     y = (signed char) (glyphtab[glyphnum][index++]);
	     if (!skip) advance(pendown, xx, yy, scale, (double) x, (double) y);
	     while (x!=0 || y!=0) {
		 x = (signed char) (glyphtab[glyphnum][index++]);
		 y = (signed char) (glyphtab[glyphnum][index++]);
		 // printf("pen 9\n");
		 if (!skip) advance(pendown, xx, yy, scale, (double) x,(double) y);
	     }
	     break; 
	   case 10: // octant_arc(x,y);
	     r = (unsigned char) (glyphtab[glyphnum][index++]);
	     o = (signed char) (glyphtab[glyphnum][index++]);

	     cw = (o>=0);		// positive is clockwise

	     if (o < 0) {
	        o*=-1;
	     }

	     octant = ((o&0xf0)>>4);	// 0->e, 1->ne...
	     numoct = ((o&0x0f));	// number of actants

	     double stheta = 2.0*M_PI*(((double) octant)/8.0);
	     double dtheta = 2.0*M_PI*(((double) numoct)/8.0);

	     if (!cw) {
	     	dtheta *= -1.0;
	     }
		
	     #define OCTRES 8

	     // if (!skip) printf("pen 4\n");
	     for (i=0; i<=(numoct*OCTRES); i++) {
		xx2 = xx1;
		yy2 = yy1;
		
		xx1 = ((double)r) *cos(stheta + dtheta * \
			((double) i) / ((double) numoct * (double) OCTRES));
		yy1 = ((double) r) *sin(stheta + dtheta * \
			((double) i) / ((double) numoct * (double) OCTRES));
		// fprintf(stderr, "%g %g\n", xx1, yy1);
		if (i>0) {
		    if (!skip) advance(pendown, xx, yy, scale, xx1-xx2, yy1-yy2);
		}
	     }
	     // if (!skip) printf("pen 3\n");

	     // fprintf(stderr, "octant_arc: cw: %d, o:%d oct:%d, n:%d,
	     // r:%d\n", cw, o, octant, numoct,r);

	     break;
	   case 11: // fractional_arc(p1,p2,p3,p4,p5);
	     p1 = (unsigned char) (glyphtab[glyphnum][index++]);
	     p2 = (unsigned char) (glyphtab[glyphnum][index++]);
	     p3 = (unsigned char) (glyphtab[glyphnum][index++]);
	     p4 = (unsigned char) (glyphtab[glyphnum][index++]);
	     p5 = (signed char) (glyphtab[glyphnum][index++]);

	     // p1 encodes angle past starting octant
	     // p2 encodes angle past ending octant
	     // p3 is high byte of radius
	     // p4 is low byte of radius
	     // p5 is hex: +/-0<start><num>

	     cw = (p5>0);		// positive is clockwise
	     if (p5 < 0) {
	        p5*=-1;
	     }

 	     fprintf(stderr,"frac: p1:%d, p2:%d, p3:%d, p4:%d, p5:%02x\n",
	         p1, p2, p3, p4, p5&0xff);

	     double radius;

	     r = 256*p3 + p4;

	     octant = ((p5&0xf0)>>4);	// 0->e, 1->ne...
	     numoct = ((p5&0x0f));	// number of octants

	     stheta =  2.0*M_PI*(((double) octant)/8.0);
	     stheta += 2.0*M_PI*((double)p1)*45.0/(256.0*360.0);
	     dtheta =  2.0*M_PI*(((double) numoct)/8.0);
	     dtheta += 2.0*M_PI*((double)p2)*45.0/(256.0*360.0);

	     if (!cw) dtheta -= 1.0;

 fprintf(stderr,"frac: stheta:%g, dtheta:%g, cw:%d, noct:%d, oct:%d, r:%d\n", 
 	stheta, dtheta, cw, numoct, octant, r); 
		
	     #define OCTRES 8

	     double xx1, yy1, xx2, yy2;
	     int i;
	     // if (!skip) printf("pen 4\n");
	     for (i=0; i<=(numoct*OCTRES); i++) {
		xx2 = xx1;
		yy2 = yy1;
		
		xx1 = ((double)r) *cos(stheta + dtheta * \
			((double) i) / ((double) numoct * (double) OCTRES));
		yy1 = ((double) r) *sin(stheta + dtheta * \
			((double) i) / ((double) numoct * (double) OCTRES));
		// fprintf(stderr, "%g %g\n", xx1, yy1);
		if (i>0) {
		    if (!skip) advance(pendown, xx, yy, scale, xx2-xx1, yy2-yy1);
		}
	     }
	     // if (!skip) printf("pen 3\n");
	     // fprintf(stderr, "fractional_arc not implemented!\n");
	     break;
	   case 12: // bulge_arc(p1,p2,p3);
	     p1 = (signed char) (glyphtab[glyphnum][index++]);
	     p2 = (signed char) (glyphtab[glyphnum][index++]);
	     p3 = (signed char) (glyphtab[glyphnum][index++]);
	     // bulge_arc(p1,p2,p3);
	     // printf("pen 12\n");
	     if (!skip) bulge_arc(pendown, xx, yy, scale, \
		(double) p1, (double) p2,(double) p3);
	     break;
	   case 13: // multiple_bulge_arc(p1,p2,p3);
	     p1 = (signed char) (glyphtab[glyphnum][index++]);
	     p2 = (signed char) (glyphtab[glyphnum][index++]);
	     p3 = (signed char) (glyphtab[glyphnum][index++]);
	     if (!skip) bulge_arc(pendown, xx, yy, scale, \
	     	(double) p1, (double) p2,(double) p3);
	     int done;
	     for (done=0; !done; ) {
		 p1 = (signed char) (glyphtab[glyphnum][index++]);
		 p2 = (signed char) (glyphtab[glyphnum][index++]);
		 if (p1 != 0 || p2 != 0) {
		    p3 = (signed char) (glyphtab[glyphnum][index++]);
		    if (!skip) bulge_arc(pendown, xx, yy, scale, \
			(double) p1, (double) p2,(double) p3);
		 } else {
		    done++;
		 }
	     }
	     break;
	   case 14: // skip next instruction unless vertical mode
	     skip=2;
	     break;
	   default:
	     if (c>=15) {
		 double dx;
		 double dy;
		 double length;
		 int dircode;
		 dircode=(c&0x0f);
		 length=(double) ((c&0xf0)>>4);
		 switch (dircode) {
		   case 0:	// e
		    dx=1.0; dy=0.0; break;
		   case 1:	// nee
		    dx=1.0; dy=0.4142; break;
		   case 2:	// ne
		    dx=1.0; dy=1.0; break;
		   case 3:	// nne
		    dx=0.4142; dy=1.0; break;
		   case 4:	// n
		    dx=0.0; dy=1.0; break;
		   case 5:	// nnw
		    dx=-0.4142; dy=1.0; break;
		   case 6:	// nw
		    dx=-1.0; dy=1.0; break;
		   case 7:	// nww
		    dx=-1.0; dy=0.4142; break;
		   case 8:	// w
		    dx=-1.0; dy=0.0; break;
		   case 9:	// sww
		    dx=-1.0; dy=-0.4142; break;
		   case 10:	// sw
		    dx=-1.0; dy=-1.0; break;
		   case 11:	// ssw
		    dx=-0.4142; dy=-1.0; break;
		   case 12:	// s
		    dx=0.0; dy=-1.0; break;
		   case 13:	// sse
		    dx=0.4142; dy=-1.0; break;
		   case 14:	// se
		    dx=1.0; dy=-1.0; break;
		   case 15:	// see
		    dx=1.0; dy=-0.4142; break;
		 }
	// fprintf(stderr, "delta: dx:%g, dy:%g, len:%g xx:%g, yy:%g\n", 
	// dx,dy,length, *xx, *yy);
		// printf("pen 4\n");
		if (!skip) advance(pendown, xx, yy, scale, dx*length, dy*length);
	     }
	     break;
	}
    }
}

// -----------------------------------------
// create a linked list of fonttables

#define MAXFONTS 128

typedef struct font {
   char *fontname;
   unsigned char *glyphtab[258]; // pointers to byte arrays
   struct font *next;  
} FONT;

FONT *fonttable[MAXFONTS];

// int shp_font_init();
	// called to set fonttable to all NULLs

// int shp_loadfont(char *path);  
	// returns fonttable index, -1 on err
	// saves basename(path) into fontname;
        // note font is loaded in 0
	// txt font is loaded in 1
        // other fonts loaded sequentially into fonttable
        // in first unused position

// int shp_name_to_index(char *name);	
	// tries to find name in fonttable
        // tries to load it from $FONT_PATH if not found
	// returns index, -1 on err

// double shp_string_height(char *s, int id, double *width, double *height);
	// evaluates string and returns width and height

// void shp_writestring( char *s, char *name, int id, ... );
	// never fails: if font name is not found or NULL, 
        // uses specified default id 0 or 1 depending 
        // on NOTE or TXT

// -----------------------------------------

int main()
{
    char *tp;
    TOKEN token;
    int state = 0;

    int glyphnum = 0;
    int numbytes = 0;
    int count=0;
    char fontname[128];
    char bytecode[2000];
    char *t;
    int i;
    int debug=0;

    eatwhite();
 
    for (i=0; i<258; i++) {
	glyphtab[i] = NULL;
    }

    while ((token = token_get(&tp)) != EOF) {
	if (debug) printf("%s: <%s>\n", tok2str(token), tp);
	switch (state) {
	case 0:		// start of record
	    if (token == EOL) {
	        ;
	    } else if (token == SP) {
	        ;
	    } else if (token == STAR) {
		state = 1;
	    } else {
		fprintf(stderr, "parse error in state %d, tok=%s, glyphnum=%d\n", 
		state, tok2str(token), glyphnum);
		exit(1);
	    }
	    break;
	case 1:		// get glyph num
	    if (token == NUM) {
		glyphnum = strtonum(tp);
	    } else if (token == WORD) {
		glyphnum = 0;
	    } else {
		fprintf(stderr, "parse error in state %d\n", state);
		exit(1);
	    }
	    if (debug) printf("got glyphnum %d\n", glyphnum);
	    state = 2;
	    break;
	case 2:		// get number of bytes in def
	    if (token != NUM) {
		fprintf(stderr, "parse error in state %d\n", state);
		exit(1);
	    }
	    numbytes = strtonum(tp);
	    if (debug) printf("got numbytes %d\n", numbytes);
	    state = 3;
	    break;
	case 3:		// get fontname
	    if (token == EOL) {
		strcpy(fontname, "");
		state=5;
		count=0;
	    } else {
		strcpy(fontname, tp);
		state = 4;
	    }
	    if (debug) printf("got fontname %s\n", fontname);
	    break;
	case 4:		// skip to EOL
	    if (token == EOL) {
		state = 5;
		count = 0;
	    }
	    break;
	case 5:
	    if (token == NUM) {
	       bytecode[count]=strtonum(tp);
	       if (debug) printf("    %d %d %d\n", count, numbytes, bytecode[count]);
	       count++; 
	    } else if (token == EOL) {
	       ;
	    } else if (token == SP) {
	       ;
	    } else {
		fprintf(stderr, "parse error in state %d\n", state);
		exit(1);
	    }
	    if (count==numbytes) {
	        state=0;
	        t = (char *) malloc(numbytes);
		if (t == NULL) {
		    fprintf(stderr, "malloc failed\n");
		    exit(1);
		}
		for (i=0; i<numbytes; i++) {
		   t[i] = bytecode[i];
		}
	        glyphtab[glyphnum] = t;
	    }
	    break;
	default:
	    break;
	}
    }

    double xx=0.0;
    double yy=0.0;

    printf("back\n");
    printf("isotropic\n");
    for (i=1; i<258; i++) {
       if (glyphtab[i] != NULL) {
	    dump((unsigned int) i, &xx, &yy, 1.0);
       }
       if ((i%16)==0) {
          xx=0.0;
	  if (glyphtab[0] != NULL) {
	     yy -= (double) glyphtab[0][0] + (double) glyphtab[0][1];
	  } else {
	      yy-=100.0;
	  }
       } else {
	  xx+=0.0;
       }
    }

    exit(0);
}


int token_unget(TOKEN token, char *word)
{
    int debug = 0;

    if (debug)
	printf("token unget called with %s\n", word);

    if (pushidx >= MAXPUSH) {
	fprintf(stderr, "pushed back too many tokens!\n");
	// return(-1);
	exit(1);
    } else {
	pushback[pushidx].word = estrdup(word);
	if (debug)
	    printf("token_unget: %s\n", pushback[pushidx].word);
	pushback[pushidx++].tok = token;
	return (0);
    }
}

// skip white space tokens (spaces + EOL)
void eatwhite()
{
    char *tp;
    TOKEN token;
    int debug = 0;

    while ((token = token_get(&tp)) != EOF) {
	if (debug)
	    printf("in eatwhite - %s: <%s>\n", tok2str(token), tp);
	if (token != SP && token != EOL) {
	    token_unget(token, tp);
	    return;
	}
    }
}


static char *tok2str(TOKEN t)
{
    switch (t) {
    case EOL:
	return ("EOL");
    case LP:
	return ("LP");
    case RP:
	return ("RP");
    case STAR:
	return ("STAR");
    case SP:
	return ("SPACE");
    case NUM:
	return ("NUM");
    case WORD:
	return ("WORD");
    case QUOTE:
	return ("QUOTE");
    case COMMA:
	return ("COMMA");
    case UNKNOWN:
    default:
	return ("UNK");
    }
}

// lookahead to next token
TOKEN token_look(char **word)
{
    TOKEN token;
    token = token_get(word);
    token_unget(token, *word);
    return token;
}

// consume next token
TOKEN token_get(char **word)
{

    enum { NEUTRAL, INQUOTE, INWORD, INNUM, COMMENT } state = NEUTRAL;
    char *w;
    int c, d;
    int debug = 0;

    if (pushidx > 0) {		// pushed back tokens exist
	if (debug)
	    printf("pushing back %s\n", pushback[pushidx - 1].word);
	strcpy(buf, pushback[--pushidx].word);
	*word = buf;
	if (pushback[pushidx].word != NULL) {
	    free(pushback[pushidx].word);
	}
	pushback[pushidx].word == (char *) NULL;
	return (pushback[pushidx].tok);
    }

    *word = buf;
    w = buf;

    while ((c = getc(stdin)) != EOF) {
	switch (state) {
	case NEUTRAL:
	    switch (c) {
	    case ' ':
	    case '\t':
		*w++ = c;
		*w = '\0';
		return (SP);
		continue;
	    case '\n':
	    case '\r':
		*w = '\0';
		line++;
		return (EOL);
	    case ',':
		// *w++ = c;
		// *w = '\0';
		// return (COMMA);
		continue;
	    case '"':
		state = INQUOTE;
		continue;
	    case ')':
		// *w++ = c;
		// *w = '\0';
		// return (RP);
		continue;
	    case '(':
		// *w++ = c;
		// *w = '\0';
		// return (LP);
		continue;
	    case '*':
		*w++ = c;
		*w = '\0';
		return (STAR);
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
	    case '-':
	    case '+':
		*w++ = c;
		state = INNUM;
		continue;
	    case ';':
		*w++ = c;
		state = COMMENT;
		continue;
	    default:
		state = INWORD;
		*w++ = c;
		continue;
	    }
	case INNUM:
	    if (isxdigit(c)) {
		*w++ = c;
		continue;
	    } else {
		ungetc(c, stdin);
		*w = '\0';
		return (NUM);
	    }
	case INWORD:
	    if (!isalnum(c)) {
		ungetc(c, stdin);
		*w = '\0';
		return (WORD);
	    }
	    *w++ = c;
	    continue;
	case INQUOTE:
	    switch (c) {
	    case '\\':
		/* escape quotes, but pass everything else along */
		if ((d = getc(stdin)) == '"') {
		    *w++ = d;
		} else {
		    *w++ = c;
		    *w++ = d;
		}
		continue;
	    case '"':
		*w = '\0';
		return (QUOTE);
	    default:
		*w++ = c;
		continue;
	    }
	case COMMENT:
	    switch (c) {
	    case '\n':
	    case '\r':
		// *w++ = c;
		*w = '\0';
		line++;
		return (EOL);
	    default:
		*w++ = c;
		continue;
	    }
	default:
	    fprintf(stderr, "bad case in lexer\n");
	    return (UNKNOWN);
	    break;
	}			// switch state
    }				// while loop
    return (EOF);
}

/* estrdup: duplicate a string, report if error */
char *estrdup(const char *s)
{
    char *t;

    t = (char *) malloc(strlen(s) + 1);
    if (t == NULL) {
	fprintf(stderr, "estrdup(\"%.20s\") failed:", s);
	exit(1);
    }
    strcpy(t, s);
    return (t);
}
