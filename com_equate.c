#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rlgetc.h"
#include "equate.h"

#define MAXBUF 128

int com_equate(LEXER* lp, char *arg)	   /* define properties of each layer */
{
    TOKEN token;
    int done=0;
    char *word;
    char buf[MAXBUF] = "";

    int color = -1;
    int line_type  = -1;
    int mask_type  = -1;
    int layer = -1;
    int pen = -1;
    int fill = 0;

    int debug=0;

    if (lp->mode != PRO) {
    	printf("Can only enter EQUate commands in the PROCESS subsystem!\n");
	token_flush_EOL(lp);
	return(1);
    }

    while(!done && (token=token_get(lp, &word)) != EOF) {
	switch(token) {
	    case OPT:		/* option */
		if (word[0] != ':') {
		    printf("EQUATE: unrecognized option: %s\n", word);
		    done++;
		    token_flush_EOL(lp);
		}
		switch(toupper((unsigned char)word[1])) {
		    case 'C':	/* color */
			if ((color = color2equate(word[2])) == -1) {
			    printf("EQUATE: bad color spec: %c\n", word[2]);
			    done++;
			    token_flush_EOL(lp);
			} 
			if (debug) printf("got color %s %d\n", word+2, color);
		        break;
		    case 'P':	/* pen */
			pen = atoi(word+2);
			if (pen < 0 || pen > 7) {
			    printf("EQUATE: bad pen: %c\n", word[2]);
			    done++;
			    token_flush_EOL(lp);
			} 
			if (debug) printf("got pen %s\n", word+2);
		        break;
		    case 'M':	/* mask linetype */
			if ((line_type = linetype2equate(word[2])) == -1) {
			    printf("EQUATE: bad mask line type: %c\n", word[2]);
			    done++;
			    token_flush_EOL(lp);
			} 
			if (debug) printf("got mask %s\n", word+2);
		        break;
		    case 'F':	/* fill flag */
			fill = atoi(word+2);
			if (fill < 0 || fill > 7) {
			    printf("EQUATE: bad fill: %c\n", word[2]);
			    done++;
			    token_flush_EOL(lp);
			} 
			if (debug) printf("got fill %s\n", word+2);
		        break;
		    case 'B':	/* boundary */
		    case 'D':	/* detail */
		    case 'S':	/* symbolic */
		    case 'I':	/* interconnect */
			if ((mask_type = masktype2equate(word[1])) == -1) {
			    printf("EQUATE: bad mask type: %c\n", word[1]);
			    done++;
			    token_flush_EOL(lp);
			} 
		        break;
		    default:
		    	printf("EQUATE: unrecognized option: %s\n", word);
			done++;
			token_flush_EOL(lp);
			break;
		    }
	        break;

	    case NUMBER: 	/* number */
		if (((layer = atoi(word)) < 0) || layer > MAX_LAYER ) {
		    printf("EQUATE: bad layer: %d\n", layer);
		    done++;
		    token_flush_EOL(lp);
		} 
		if (debug) printf("got layer %d\n", layer);
	        break;
	    case IDENT: 	/* identifier */
		strncpy(buf, word,MAXBUF);
		if (debug) printf("got name %s\n", buf);
	        break;
	    case CMD:		/* command */
		token_unget(lp, token, word);
		done++;
		break;
	    case EOC:		/* end of command */
		done++;
		break;
	    case EOL:		/* newline or carriage return */
	        break;
	    default:
		printf("EQUATE: expected OPT, got %s %s\n", tok2str(token), word);
		done++;
	    	break;
	}
    }

    if (layer == -1) {
	printf("EQUATE: no layer specified\n");
    } else {
    	if (color != -1) 	equate_set_color(layer, color);
	if (line_type != -1)    equate_set_linetype(layer, line_type);
	if (pen != -1)          equate_set_pen(layer, pen);
	if (mask_type != -1)    equate_set_masktype(layer, mask_type);
	if (buf[0] != '\0')     equate_set_label(layer, buf);
		                equate_set_fill(layer, fill);	
    }

    /* equate_print(); */

    return (0);
}
