#include <stdio.h>
#include "db.h"
#include "opt_parse.h"
#include "eprintf.h"
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MIRROR_OFF 0
#define MIRROR_X   1
#define MIRROR_Y   2
#define MIRROR_XY  3

#define ERR -1

/* called with OPTION in optstring.  Compares optstring against
validopts and returns ERR if invalid option, or if invalid
scan of option value.  Updates opt structure if OK */

int opt_parse(
    char *optstring,
    char *validopts,	/* a string like "WSRMYZF" */
    OPTS *popt
) {

    double optval, optval2;
    int i;

    if (optstring[0] == '#' || optstring[0] == '-' || optstring[0] == '+') {
    	/* got a SHOW option - should not get this here */
	weprintf("options must start with a colon: \"%s\"\n", optstring); 
	return(ERR);
    } else if (optstring[0] == '.') {
	/* got a component name */
	if (popt->cname != NULL) {
	   free (popt->cname);
	}
    	popt->cname = strsave(optstring);
    } else if (optstring[0] == '@') {
	/* got a signal name */
	if (popt->sname != NULL) {
	   free (popt->sname);
	}
    	popt->sname = strsave(optstring);
    } else if (optstring[0] == ':') {
        /* got a regular option */

	if (index(validopts, toupper((unsigned char)optstring[1])) == NULL) {
	    weprintf("invalid option (2) %s\n", optstring); 
	    return(ERR);
	}

	switch (toupper((unsigned char)optstring[1])) {
	    case 'B': 		/* :B(npoints) use bezier for line drawing */
		if(sscanf(optstring+2, "%lf", &optval) != 1) {
		    weprintf("invalid option argument: %s\n", optstring); 
		    return(ERR);
		}
	        popt->bezier=(int) fabs(optval);
		break;
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
	    case 'H': 		/* :H(substrate_height) */
		if(sscanf(optstring+2, "%lf", &optval) != 1) {
		    weprintf("invalid option argument: %s\n", optstring+2); 
		    return(ERR);
		}
		if (optval < 0.0 ) {
		    weprintf("height must be positive: %s\n", optstring+2); 
		    return(ERR);
		}
		popt->height = optval;
		break;
	    case 'J': 		/* :J(justification) */
		/* note: the order of these tests is important */
		if (strncasecmp(optstring+2,"SW",2) ==  0) {
		    popt->justification = 0;
		} else if (strncasecmp(optstring+2,"SE",2) == 0) {
		    popt->justification = 2;
		} else if (strncasecmp(optstring+2,"NE",2) == 0) {
		    popt->justification = 8;
		} else if (strncasecmp(optstring+2,"NW",2) == 0) {
		    popt->justification = 6;
		} else if (strncasecmp(optstring+2,"S" ,1) == 0) {
		    popt->justification = 1;
		} else if (strncasecmp(optstring+2,"W" ,1) == 0) {
		    popt->justification = 3;
		} else if (strncasecmp(optstring+2,"C" ,1) == 0) {
		    popt->justification = 4;
		} else if (strncasecmp(optstring+2,"E" ,1) == 0) {
		    popt->justification = 5;
		} else if (strncasecmp(optstring+2,"N" ,1) == 0) {
		    popt->justification = 7;
		} else {
		    if(sscanf(optstring+2, "%lf", &optval) != 1) {
			weprintf("invalid option argument: %s\n", optstring+2); 
			return(ERR);
		    }
		    if (optval < 0.0 || optval > 8.0 ) {
		         weprintf("justification must be between 0 and 8: %s\n", optstring+2); 
		         return(ERR);
		    }
		    popt->justification = (int) optval;
		}
		break;
	    case 'M':		/* mirror X,Y,XY */
		/* Note: these comparisons must be done in this order */
		if (strncasecmp(optstring+1,"MXY",3) == 0) {
		    popt->mirror = MIRROR_XY;
		} else if (strncasecmp(optstring+1,"MY",2) == 0) {
		    popt->mirror = MIRROR_Y;
		} else if (strncasecmp(optstring+1,"MX",2) == 0) {
		    popt->mirror = MIRROR_X;
		} else if (strncasecmp(optstring+1,"M0",2) == 0) {
		    popt->mirror = MIRROR_OFF;
		} else if (strncasecmp(optstring+1,"MOFF",4) == 0) {
		    popt->mirror = MIRROR_OFF;
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
		//if (i !=0 && i != 1) {
		//    weprintf("font_num must be 0 or 1: %s\n", optstring+2); 
		//    return(ERR);
		//}
		popt->font_num = i;
		break;
	    case 'R':		/* resolution or rotation angle +/- 360 */
		if(sscanf(optstring+2, "%lf", &optval) != 1) {
		    weprintf("invalid option argument: %s\n", optstring+2); 
		    return(ERR);
		}
		if (optval <= -370.0 || optval >= 370.0 ) {
		    weprintf("option out of range: %s\n", optstring+2); 
		    return(ERR);
		}
		popt->rotation = optval;
		break;
	    case 'S':		/* step an instance */
		if(sscanf(optstring+2, "%lf,%lf", &optval, &optval2) != 2) {
		    weprintf("invalid option argument: %s\n", optstring+2); 
		    return(ERR);
		}
		popt->cols = (int) optval;
		popt->rows = (int) optval2;
		popt->stepflag = 1;
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

