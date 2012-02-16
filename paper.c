#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAXBUF 128

typedef struct psize {
   char *name;
   double x;
   double y;
} PSIZE;

PSIZE psizes[] = {
  {"A0",              33.1,           46.8},
  {"A1",              23.4,           33.1},
  {"A2",              16.5,           23.4},
  {"A3",              11.7,           16.5},
  {"A4",              8.3,            11.7},
  {"A5",              5.83333,        8.26389},
  {"A6",              4.125,          5.83333},
  {"A7",              2.91667,        4.125},
  {"A8",              2.05556,        2.91667},
  {"A",               8.5,            11.0},
  {"A9",              1.45833,        2.05556},
  {"ANSIA",           8.5,            11.0},
  {"ANSIB",           11.0,           17.0},
  {"ANSIC",           17.0,           22.0},
  {"ANSID",           22.0,           34.0},
  {"ANSIE",           34.0,           44.0},
  {"ANSIF",           36.0,           48.0},
  {"ARCHA",           9.0,            12.0},
  {"ARCHB",           12.0,           18.0},
  {"ARCHC",           18.0,           24.0},
  {"ARCHD",           24.0,           36.0},
  {"ARCHE",           36.0,           48.0},
  {"atlas",           26.0,           34.0},
  {"B0",              39.4,           55.7},
  {"B",               11.0,           17.0},
  {"B1",              27.8,           39.4},
  {"B2",              19.7,           27.8},
  {"B3",              13.9,           19.7},
  {"B4",              9.8,            13.9},
  {"B5",              6.9,            9.8},
  {"B6",              4.9,            6.9},
  {"B7",              3.5,            4.9},
  {"B8",              2.4,            3.5},
  {"B9",              1.7,            2.4},
  {"brief",           13.13,          18.5},
  {"C",               17.0,           22.0},
  {"crown",           15.0,           20.0},
  {"C5Envelope",      6.40278,        9.0},
  {"Comm10Envelope",  4.125,          9.5},
  {"D",               22.0,           34.0},
  {"demy",            17.5,           22.5},
  {"DLEnvelope",      4.33333,        8.66667},
  {"E",               34.0,           44.0},
  {"elephant",        23.0,           28.0},
  {"Executive",       7.25,           10.5},
  {"Folio",           8.26389,        12.9861},
  {"foolscap",        13.5,           17.0},
  {"imperial",        22.0,           30.0},
  {"index",           8.0,            10.0},
  {"ISOA0",           33.1,           46.8},
  {"ISOA1",           23.4,           33.1},
  {"ISOA2",           16.5,           23.4},
  {"ISOA3",           11.7,           16.5},
  {"ISOA4",           8.3,            11.7},
  {"ISOA5",           5.83333,        8.26389},
  {"ISOA6",           4.125,          5.83333},
  {"ISOA7",           2.91667,        4.125},
  {"ISOA8",           2.05556,        2.91667},
  {"ISOA9",           1.45833,        2.05556},
  {"ISOB0",           39.4,           55.7},
  {"ISOB1",           27.8,           39.4},
  {"ISOB2",           19.7,           27.8},
  {"ISOB3",           13.9,           19.7},
  {"ISOB4",           9.8,            13.9},
  {"ISOB5",           6.9,            9.8},
  {"ISOB6",           4.9,            6.9},
  {"ISOB7",           3.5,            4.9},
  {"ISOB8",           2.4,            3.5},
  {"ISOB9",           1.7,            2.4},
  {"Ledger",          11.0,           17.0},
  {"legal",           8.5,            14.0},
  {"Letter",          8.5,            11.0},
  {"medium",          17.5,           23.0},
  {"post",            15.5,           19.25},
  {"royal",           20.0,           25.0},
  {"tabloid",         11.0,           17.0},
  {"", 0.0, 0.0},
};

char buf[MAXBUF];

// called with a paper size name, return 1 on success
// and update x,y with size.  Return 0 on error
// x,y set to 8.5x11...

int pnametosize(char *name, double *px, double *py)
{
    int debug=0;
    int i;
    int matched = 0;
    if (debug) printf("pnametosize called with <%s>\n", name);

    if (isalpha((unsigned char)name[0])) {
	for (i = 0; psizes[i].x != 0.0; i++) {
	    if (debug) printf("checking %s %s\n", name, psizes[i].name);
	    if (strncasecmp(name, psizes[i].name, MAXBUF) == 0) {
		matched++;
		*px = psizes[i].x;
		*py = psizes[i].y;
		break;
	    }
	}
    } else if (isdigit((unsigned char)name[0])) {
	if (sscanf(name, "%lfx%lf", px, py) == 2) {
	    matched++;
	} else if (sscanf(name, "%lf*%lf", px, py) == 2) {
	    matched++;
	}
    }
    if (!matched) {
	*px=8.5; *py=11.0;
    }
    if (debug) printf("pnametosize returning %d\n", matched);
    return(matched);
}


int testpapermain()
{
    double x, y;

    while (fgets(buf, MAXBUF, stdin) != NULL) {
	buf[strlen(buf) - 1] = '\0';
	if (pnametosize(buf, &x, &y)) {
	    printf("\t%g %g\n", x, y);
	} else {
	    printf("\tnot found: %g %g\n", x, y);
	}
    }
    return(1);
}
	
// when creating postscript, if no :P option is given
// the set paper xy to $PIG_PAPERSIZE
