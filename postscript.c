#include <stdio.h> 
#include <time.h> 
#include <pwd.h> 
#include <sys/types.h> 
#include <unistd.h>
#include "postscript.h"

#define MAXBUF 256

int linetype=0;
int pennum=0;
int in_line=0;		/* set to 1 when working on a line */

int this_pen=0;;
int this_line=0;

void ps_set_line(line)
int line;
{
    linetype=line;
}

void ps_set_pen(pen) 
int pen;
{
    pennum=pen;
}

/*
Letter  0 0 8.5i 11i 
Legal   0 0 8.5i 14i
ANSI-E  0 0  34i 44i
Tabloid 0 0  11i 17i
ANSI-C  0 0  17i 22i
ANSI-D  0 0  22i 34i
*/

/* evidently pays attention to pagesize labels...  can't say Letter */
/* and give Ansi-A page sizes... */

void ps_preamble(fp,dev, prog, pdx, pdy, llx, lly, urx, ury) 
FILE *fp;
char *dev;
char *prog;
double pdx, pdy; 	    /* page size in inches */
double llx, lly, urx, ury;  /* drawing bounding box in user coords */
{
    time_t timep;
    char buf[MAXBUF];
    double xmid, ymid;
    double xdel, ydel;
    double s1, s2, scale;
    double xmax, ymax;
    int landscape;
    double tmp;

    /* now create transform commands based on bb and paper size */
    /* FIXME, check that bb is in canonic order */

    xmid = (llx+urx)/2.0;
    ymid = (lly+ury)/2.0;
    xdel = sqrt((urx-llx)*(urx-llx));
    ydel = sqrt((ury-lly)*(ury-lly));

    pdx=8.5;
    pdy=11.0;

    xmax=pdx*72.0;
    ymax=pdy*72.0;
    landscape=0;
    if (xdel > ydel) { /* flip aspect */
	landscape=1;
	tmp=xmax;  xmax=ymax; ymax=tmp;
    }

    s1 = xmax/xdel;
    s2 = ymax/ydel;

    scale = s2;
    if (s1 < s2) {
    	scale = s1;
    }

    fprintf(fp,"%%!PS-Adobe-2.0\n");
    fprintf(fp,"%%%%Title: %s\n", dev);
    fprintf(fp,"%%%%Creator: %s\n", prog);
    timep=time(&timep);
    fprintf(fp,"%%%%CreationDate: %s", ctime(&timep));
    gethostname(buf,MAXBUF);
    fprintf(fp,"%%%%For: %s@%s (%s)\n", 
    	getpwuid(getuid())->pw_name, 
	buf,
	getpwuid(getuid())->pw_gecos );
    if (landscape) {
	fprintf(fp,"%%%%Orientation: Portrait\n");
    } else {
	fprintf(fp,"%%%%Orientation: Portrait\n");
    }
    fprintf(fp,"%%%%Pages: 1\n");
    fprintf(fp,"%%%%BoundingBox: 0 0 %d %d\n", 
    	(int) (pdx*72.0), (int) (pdy*72.0));
    fprintf(fp,"%%%%DocumentPaperSizes: custom\n");
    fprintf(fp,"%%%%BeginSetup\n");
    fprintf(fp,"[{\n");
    fprintf(fp,"%%BeginFeature: *PageRegion custom\n");
    fprintf(fp,"<</PageSize [%d %d]>> setpagedevice\n",
    	(int)( pdx*72.0), (int) (pdy*72.0));
    fprintf(fp,"%%EndFeature\n");
    fprintf(fp,"} stopped cleartomark\n");
    fprintf(fp,"%%%%EndSetup\n");
    fprintf(fp,"%%%%Magnification: 1.0000\n");
    fprintf(fp,"%%%%EndComments\n");
    fprintf(fp,"/$Pig2psDict 200 dict def\n");
    fprintf(fp,"$Pig2psDict begin\n");
    fprintf(fp,"$Pig2psDict /mtrx matrix put\n");
    fprintf(fp,"/col-1 {0 setgray} bind def\n");
    fprintf(fp,"/col0 {0.000 0.000 0.000 srgb} bind def\n");
    fprintf(fp,"/col1 {0.500 0.000 0.000 srgb} bind def\n");
    fprintf(fp,"/col2 {0.000 0.500 0.000 srgb} bind def\n");
    fprintf(fp,"/col3 {0.200 0.200 0.500 srgb} bind def\n");
    fprintf(fp,"/col4 {0.000 0.500 0.500 srgb} bind def\n");
    fprintf(fp,"/col5 {0.500 0.000 0.500 srgb} bind def\n");
    fprintf(fp,"/col6 {0.500 0.500 0.000 srgb} bind def\n");
    fprintf(fp,"/col7 {0.200 0.200 0.500 srgb} bind def\n");
    fprintf(fp,"/col8 {0.000 0.000 0.560 srgb} bind def\n");
    fprintf(fp,"end\n");
    fprintf(fp,"save\n");
    fprintf(fp,"newpath\n");
    fprintf(fp,"0 %d moveto\n", (int) (pdy*72.0));
    fprintf(fp,"0 0 lineto\n");
    fprintf(fp,"%d 0 lineto\n", (int) (pdx*72.0));
    fprintf(fp,"%d %d lineto closepath clip newpath\n", 
    	(int) (pdx*72.0), (int) (pdy*72.0));
    /* fprintf(fp,"%%39.9 49.0 translate\n"); */
    fprintf(fp,"/cp {closepath} bind def\n");
    fprintf(fp,"/ef {eofill} bind def\n");
    fprintf(fp,"/gr {grestore} bind def\n");
    fprintf(fp,"/gs {gsave} bind def\n");
    fprintf(fp,"/sa {save} bind def\n");
    fprintf(fp,"/rs {restore} bind def\n");
    fprintf(fp,"/l {lineto} bind def\n");
    fprintf(fp,"/m {moveto} bind def\n");
    fprintf(fp,"/rm {rmoveto} bind def\n");
    fprintf(fp,"/n {newpath} bind def\n");
    fprintf(fp,"/s {stroke} bind def\n");
    fprintf(fp,"/sh {show} bind def\n");
    fprintf(fp,"/slc {setlinecap} bind def\n");
    fprintf(fp,"/slj {setlinejoin} bind def\n");
    fprintf(fp,"/slw {setlinewidth} bind def\n");
    fprintf(fp,"/srgb {setrgbcolor} bind def\n");
    fprintf(fp,"/rot {rotate} bind def\n");
    fprintf(fp,"/sc {scale} bind def\n");
    fprintf(fp,"/sd {setdash} bind def\n");
    fprintf(fp,"/ff {findfont} bind def\n");
    fprintf(fp,"/sf {setfont} bind def\n");
    fprintf(fp,"/scf {scalefont} bind def\n");
    fprintf(fp,"/sw {stringwidth} bind def\n");
    fprintf(fp,"/tr {translate} bind def\n");
    fprintf(fp,"/$Pig2psBegin\n");
    fprintf(fp,"{$Pig2psDict begin /$Pig2psEnteredState save def}def\n");
    fprintf(fp,"/$Pig2psEnd {$Pig2psEnteredState restore end} def\n");
    fprintf(fp,"$Pig2psBegin\n");
    fprintf(fp,"10 setmiterlimit\n");
    fprintf(fp,"1 slj 1 slc\n");
    fprintf(fp,"1 slw\n");
    fprintf(fp,"%%BeginPageSetup\n");
    fprintf(fp,"%BB is %g,%g %g,%g\n", llx, lly, urx, ury);	
    if (landscape) {
    	fprintf(fp,"%g %g scale\n", scale, scale);
	fprintf(fp,"%g %g translate\n", ury, -llx);
	fprintf(fp,"%%%g %g translate\n", 
	    (ymax/(2.0*scale))+ymid, (xmax/(2.0*scale))-xmid);
	fprintf(fp,"90 rotate\n");
    } else {
    	fprintf(fp,"%g %g scale\n", scale, scale);
	fprintf(fp,"%g %g translate\n", 
	    (xmax/(2.0*scale))-xmid, (ymax/(2.0*scale))-ymid);
    }
    fprintf(fp,"%%EndPageSetup\n");
    fprintf(fp,"%% here starts figure;\n");
}

void ps_end_line(fp)
FILE *fp;
{
    extern int this_pen;
    extern int in_line;

    fprintf(fp, "gs\n");
    fprintf(fp, "col%d\n", this_pen);
    fprintf(fp, "s\n");
    fprintf(fp, "gr\n");
    in_line=0;
}

void ps_continue_line(fp, x1, y1)
FILE *fp;
double x1, y1;
{
    fprintf(fp, "%g %g l\n",x1, y1);
    in_line++;
}

void ps_start_line(fp, x1, y1)
FILE *fp;
double x1, y1;
{
    extern int pennum;
    extern int linetype;
    extern int in_line;
    extern int this_pen;
    extern int this_line;

    if (in_line) {
    	ps_end_line(fp);
    } 
    in_line++;

    this_pen=pennum;		/* save characteristics at start of line */
    this_line=linetype;
    fprintf(fp, "n\n");
    fprintf(fp, "%g %g m\n",x1, y1);
}



void ps_postamble(fp)
FILE *fp;
{
    if (in_line) {
    	ps_end_line(fp);
    } 
    fprintf(fp,"%% here ends figure;\n");
    fprintf(fp,"%%Pig2psEnd\n");
    fprintf(fp,"rs\n");
    fprintf(fp,"showpage\n");
}