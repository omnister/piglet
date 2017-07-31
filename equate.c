#include "db.h"
#include "equate.h"
#include <string.h>
#include <ctype.h>

/* private definitions */

typedef struct flags {
    unsigned int color    : 4;  /* Color, 3-bits, 0=Red, 1=G, ... BYPAW, 60% grey, 33% grey */
    unsigned int fill     : 3;  /* 0=unfilled, 1=solid, 2=stip, 3=45_up, 4=45_down... */
    unsigned int override : 1;  /* 1=forces solid fill (used by win:o<layer>) */
    unsigned int masktype : 2;  /* 0=Boundary, 1=Detail, 2=Symbolic, 3=Interconnect */	
    unsigned int pen      : 3;  /* Pen, 3-bits (0=unused) */  
    unsigned int linetype : 3;  /* Mask: 0=Solid, 1=Dotted, 2=Broken */
    unsigned int used     : 1;  /* Set 1 if defined, otherwise not plotted */
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
	equates[i].flags.override=0;	/* default unfilled */
	equates[i].flags.masktype=0;	/* boundary, detail, symbolic; interconnect */
	equates[i].flags.pen=1;	        /* Pen, 3-bits (0=unused) */  
	equates[i].flags.color=1;	/* set default G: 0-7=>WRGBCMY */
	equates[i].flags.linetype=0;	/* default Solid Lines */
	equates[i].flags.used=0;        /* set unused */
    }

    /* set up some default layers */
    for (i=0; i<=0; i++) {
	equates[i].flags.linetype=i%6;
	equates[i].flags.used++;
	equates[i].flags.color=i;
	equates[i].flags.pen=i;	   
	equates[i].flags.masktype=i%2;
	equates[i].flags.fill=i%2;
	equates[i].flags.override=0;
    }

    return(0);
}

static char *COLORTAB =  "WRGBCMYWLD";
static char *LINETYPE =  "SDB34567";
static char *MASKTYPE =  "BDSI";

int equate2color(int c) {
    if (c <= 9) {
        return(COLORTAB[c]);
    } else {
    	return(c+'0');
    }
}

/* parse character for 0-7, WRGB...Y or return -1 */
int color2equate(char c) {
   char *i;
   if (isdigit((unsigned char)c)) {
       return((c-'0'));
   } else if ((i=strchr(COLORTAB, toupper((unsigned char)c)))) {
       return((int)(i-COLORTAB));
   } else {
       return(-1);
   }
}

int equate2masktype(int c) {
   return(MASKTYPE[c%4]);
}

int masktype2equate(char c) {
   char *i;
   if ((i=strchr(MASKTYPE, toupper((unsigned char)c)))) {
       return((int)(i-MASKTYPE));
   } else {
       return(-1);
   }
}

int equate2linetype(int c) {
   return(LINETYPE[c%7]);
}

int linetype2equate(char c) {
   char *i;
   if (isdigit((unsigned char)c)) {
	return((c-'0')%7);
   } else if ((i=strchr(LINETYPE, toupper((unsigned char)c)))) {
       return((int)(i-LINETYPE));
   } else {
       return(-1);
   }
}

/* EQUATE :C[1-7WRGBCMY] :P[0-7] :M[DSI] :F[0-7] :[SDB] INSTBOUN 0 */
int equate_print() {
    int i;
    for (i=0; i<MAX_LAYER; i++) {
	if (equates[i].flags.used) {
	    printf("EQUATE :C%c ", equate2color(equates[i].flags.color));
	    printf(":P%d ", equates[i].flags.pen);
	    printf(":M%c ", equate2linetype(equates[i].flags.linetype));
	    printf(":%c ", equate2masktype(equates[i].flags.masktype));
	    printf(":F%d ", equates[i].flags.fill);
	    if (equates[i].flags.override) {
	    	printf(":O ");
	    }
	    printf("%s %d;\n", equates[i].label, i);
	}
    };
    return(0);
}

// a set override bit forces the displayed polygons to be hard filled.  
// otherwise the normal fill pattern is used.  This facility is used
// by win:O<layer> to selectively fill layers for visibility

void equate_clear_overrides(void) {	
    int i;
    for (i=0; i<MAX_LAYER; i++) {
	equates[i].flags.override=0;	
    }
}
void equate_toggle_override(int layer) {
    if (layer < MAX_LAYER && layer >= 0) {
	if (equates[layer].flags.override==0) {
	    equates[layer].flags.override=1;
	} else {
	    equates[layer].flags.override=0;
	}
    } else {
        printf("invalid layer %d\n", layer);
    }
}
int equate_get_override(layer) {	
    if (layer < MAX_LAYER && layer >= 0) {
        return(equates[layer].flags.override);
    }
    return(0);
}

void  equate_set_linetype(int layer, int linetype) {
    equates[layer].flags.used=1;
    equates[layer].flags.linetype = linetype;
}
int   equate_get_linetype(int layer) {
    return(equates[layer].flags.linetype);
}

void equate_set_masktype(int layer, int masktype) {
    equates[layer].flags.used=1;
    equates[layer].flags.masktype = masktype;
}
int  equate_get_masktype(int layer) {
    return(equates[layer].flags.masktype);
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

