#include <stdio.h>
#include "db.h"
#include "opt_parse.h"

#define MIRROR_OFF 0
#define MIRROR_X   1
#define MIRROR_Y   2
#define MIRROR_XY  3

#define ERR -1

/* called with OPTION in optstring.  Compares optstring against
validopts and returns ERR if invalid option, or if invalid
scan of option value.  Updates opt structure if OK */

int opt_parse(optstring, validopts, popt) 
char *optstring;
char *validopts;	/* a string like "WSRMYZF" */
OPTS *popt;
{

    double optval;
    int i;

    if (optstring[0] == '.') {
	/* got a component name */
    	popt->cname = strsave(optstring);
    } else if (optstring[0] == '@') {
	/* got a signal name */
    	popt->sname = strsave(optstring);
    } else if (optstring[0] == ':') {
        /* got a regular option */

	if (index(validopts, toupper(optstring[1])) == NULL) {
	    weprintf("invalid option (2) %s\n", optstring); 
	    return(ERR);
	}

	switch (toupper(optstring[1])) {
	    case 'F': 		/* :F(fontsize) */
		if(sscanf(optstring+2, "%lf", &optval) != 1) {
		    weprintf("invalid option argument: %s\n", optstring+2); 
		    return(ERR);
		}
		if (optval < 0.0 ) {
		    weprintf("fontsize must be positive: %s\n", optstring+2); 
		    return(ERR);
		}
		popt->font_size = optval;
		break;
	    case 'M':		/* mirror X,Y,XY */
		/* Note: these comparisons must be done in this order */
		if (strncasecmp(optstring+1,"MXY",3) == 0) {
		    popt->mirror = MIRROR_XY;
		} else if (strncasecmp(optstring+1,"MY",2) == 0) {
		    popt->mirror = MIRROR_Y;
		} else if (strncasecmp(optstring+1,"MX",2) == 0) {
		    popt->mirror = MIRROR_X;
		} else {
		    weprintf("unknown mirror option %s\n", optstring); 
		    return(ERR);
		}
		break;
	    case 'N': 		/* :N(font_num) */
		if(sscanf(optstring+2, "%lf", &optval) != 1) {
		    weprintf("invalid option argument: %s\n", optstring+2); 
		    return(ERR);
		}
		i = (int) optval;
		if (i !=0 && i != 1) {
		    weprintf("font_num must be 0 or 1: %s\n", optstring+2); 
		    return(ERR);
		}
		popt->font_num = i;
		break;
	    case 'R':		/* resolution or rotation angle +/- 180 */
		if(sscanf(optstring+2, "%lf", &optval) != 1) {
		    weprintf("invalid option argument: %s\n", optstring+2); 
		    return(ERR);
		}
		if (optval < -180.0 || optval > 180.0 ) {
		    weprintf("option out of range: %s\n", optstring+2); 
		    return(ERR);
		}
		popt->rotation = optval;
		break;
	    case 'S':		/* step an instance */
		weprintf("option ignored: instance stepping not implemented\n");
		return(ERR);
		break;
	    case 'W':		/* width */
		if(sscanf(optstring+2, "%lf", &optval) != 1) {
		    weprintf("invalid option argument: %s\n", optstring+2); 
		    return(ERR);
		}
		if (optval < 0.0 ) {
		    weprintf("width must be positive: %s\n", optstring+2); 
		    return(ERR);
		}
		popt->width = optval;
		break;
	    case 'X':		/* scale */
		if(sscanf(optstring+2, "%lf", &optval) != 1) {
		    weprintf("invalid option argument: %s\n", optstring+2); 
		    return(ERR);
		}
		if (optval < 0.0 ) {
		    weprintf("scale must be positive: %s\n", optstring+2); 
		    return(ERR);
		}
		popt->scale = optval;
		break;
	    case 'Y':		/* yx ratio */
		if(sscanf(optstring+2, "%lf", &optval) != 1) {
		    weprintf("invalid option argument: %s\n", optstring+2); 
		    return(ERR);
		}
		if (optval < 0.01 || optval > 100.0 ) {
		    weprintf("out of range (0.01 < Y < 100.0): %s\n", optstring+2); 
		    return(ERR);
		}
		popt->aspect_ratio = optval;
		break;
	    case 'Z':		/* slant +/-45 */
		if(sscanf(optstring+2, "%lf", &optval) != 1) {
		    weprintf("invalid option argument: %s\n", optstring+2); 
		    return(ERR);
		}
		if (optval < -45.0 || optval > 45.0 ) {
		    weprintf("out of range (+/-45 degrees): %s\n", optstring+2); 
		    return(ERR);
		}
		popt->slant = optval;
		break;
	    default:
		weprintf("unknown option ignored: %s\n", optstring);
		return(ERR);
		break;
	}
    } else {
	weprintf("unknown option ignored: %s\n", optstring);
    } 
    return(1);
}

