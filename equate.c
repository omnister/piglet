#include "db.h"
#include "equate.h"
#include <string.h>
#include <ctype.h>

/* private definitions */

typedef struct flags {
    unsigned int color : 3;  /* Color, 3-bits, 0=Red, 1=G, ... BYPAW */
    unsigned int fill  : 1;  /* 0=unfilled, 1=filled */
    unsigned int mask  : 2;  /* Mask: 0=Solid, 1=Dotted, 2=Broken */
    unsigned int pen   : 3;  /* Pen, 3-bits (0=unused) */  
    unsigned int type  : 2;  /* 0=Detail, 1=Symbolic, 2=Interconnect */	
    unsigned int used  : 1;  /* Set 1 if defined, otherwise not plotted */
} FLAGS;

typedef struct equate {
   FLAGS flags;
   char *label;
} EQUATE;

EQUATE equates[MAX_LAYER];

int initialize_equates() {
    int i;
    char buf[128];
    for (i=0; i<MAX_LAYER; i++) {
	sprintf(buf,"UNDEF%04d", i);
    	equates[i].label=strsave(buf);
	equates[i].flags.fill=0;	/* default unfilled */
	equates[i].flags.mask=0;	/* default Solid Lines */
	equates[i].flags.pen=1;	        /* Pen, 3-bits (0=unused) */  
	equates[i].flags.color=1;	/* set default G: 0-7=>WRGBCMY */
	equates[i].flags.type=0;	/* detail, symbolic; interconnect */
	equates[i].flags.used=0;        /* set unused */
    }

    /* set up some default layers */
    for (i=0; i<=0; i++) {
	equates[i].flags.type=0;
	equates[i].flags.used++;
	equates[i].flags.color=i;
	equates[i].flags.pen=i;	   
	equates[i].flags.mask=i%2;
	equates[i].flags.fill=i%2;
    }

    return(0);
}
static char *COLORTAB =  "WWRGBCMY";
static char *LINETYPE =  "SDB";
static char *MASKTYPE =  "BDSI";

int equate2color(int c) {
   return(COLORTAB[c%7]);
}

/* parse character for 0-7, WRGB...Y or return -1 */
int color2equate(char c) {
   char *i;
   if (isdigit(c)) {
       return((c-'0')%7);
   } else if ((i=strchr(COLORTAB, toupper(c)))) {
       return((int)(i-COLORTAB));
   } else {
       return(-1);
   }
}

int equate2mask(int c) {
   return(MASKTYPE[c%3]);
}

int mask2equate(char c) {
   char *i;
   if ((i=strchr(MASKTYPE, toupper(c)))) {
       return((int)(i-MASKTYPE));
   } else {
       return(-1);
   }
}

int equate2type(int c) {
   return(LINETYPE[c%3]);
}

int type2equate(char c) {
   char *i;
   if ((i=strchr(LINETYPE, toupper(c)))) {
       return((int)(i-LINETYPE));
   } else {
       return(-1);
   }
}

/* EQUATE :C[1-7WRGBCMY] :P[0-8] :M[DSI] :[SDB] INSTBOUN 0 */
int equate_print() {
    int i;
    for (i=0; i<MAX_LAYER; i++) {
	if (equates[i].flags.used) {
	    printf("EQUATE :C%c ", equate2color(equates[i].flags.color));
	    printf(":P%d ", equates[i].flags.pen);
	    printf(":M%c ", equate2mask(equates[i].flags.mask));
	    printf(":%c ", equate2type(equates[i].flags.type));
	    if (equates[i].flags.fill) {
		printf(":O ");
	    }
	    printf("%s %d;\n", equates[i].label, i);
	}
    };
    return(0);
}

void  equate_set_type(int layer, int type) {
    equates[layer].flags.used=1;
    equates[layer].flags.type = type;
}
int   equate_get_type(int layer) {
    return(equates[layer].flags.type);
}

void equate_set_mask(int layer, int mask) {
    equates[layer].flags.used=1;
    equates[layer].flags.mask = mask;
}
int  equate_get_mask(int layer) {
    return(equates[layer].flags.mask);
}

void equate_set_fill(int layer, int fill) {
    equates[layer].flags.used=1;
    equates[layer].flags.fill = fill;
}
int  equate_get_fill(int layer) {
    return(equates[layer].flags.fill);
}

void equate_set_color(int layer, int color) {
    equates[layer].flags.used=1;
    equates[layer].flags.color = color;
}
int  equate_get_color(int layer) {
    return(equates[layer].flags.color);
}

void equate_set_pen(int layer, int pen) {
    equates[layer].flags.used=1;
    equates[layer].flags.pen = pen;
}
int  equate_get_pen(int layer) {
    return(equates[layer].flags.pen);
}

void equate_set_label(int layer, char *label) {
    equates[layer].flags.used=1;
    if (equates[layer].label != NULL) {
    	free(equates[layer].label);
    }
    equates[layer].label = strsave(label);
}
char *equate_get_label(int layer) {
    return(equates[layer].label);
}

