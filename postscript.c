#include <stdio.h> 
#include <time.h> 
#include <pwd.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "db.h"
#include "xwin.h"
#include "postscript.h"
#include "ev.h"

#define MAXBUF 256

int layer=0;
int linetype=0;
int newtype=1;
int pennum=0;
int newpen=1;
int filltype=0;
int newfill=1;
int in_line=0;		/* set to 1 when working on a line */
int in_poly=0;
int colorflag=1;	// 0=black; 1=color; 2=greyscale
double linewidth=1.0;

int this_pen=0;		// we need to save state at start of line
int this_line=0;	// because lines are implicitly terminated
int this_fill=0;	// by starting a new line which overwrites
int this_isfilled=0;	// pen, line and fill

int debug=0;

FILE *fp=NULL;		// file descriptor for output file
OMODE outputtype=POSTSCRIPT;	// 0=postscript, 1=gerber

double bbllx, bblly, bburx, bbury; // bounding box in screen coords


void ps_set_color(int color) {
   if (debug) printf("ps_set_color: %d\n",color);
   colorflag=color;
}

void ps_set_linewidth(double width) {
   if (debug) printf("ps_set_linewidth: %f\n", width);
   linewidth=width;
}

void ps_set_outputtype(OMODE mode) {
   if (debug) printf("ps_set_outputtype: %d\n",mode);
   outputtype=mode;
}

void ps_set_file(FILE *fout) 
{
  if (debug) printf("ps_set_file:\n");
  fp=fout; 
}

void ps_set_layer(int layernumber) {
    if (debug) printf("setting layer :%d\n", layernumber);
    layer=layernumber;
}

void ps_set_fill(int fill)
{
    if (debug) printf("setting fill:%d\n", fill);
    if (fill != filltype) {
       newfill=1;
    }
    filltype=fill;
}


void ps_set_line(int line)
{
    if (debug) printf("setting line:%d\n", line);
    if (line != linetype) {
       newtype=1;
    }
    linetype=line;
}

int hpgl_penmap[] = {
    1,2,3,5,
    7,6,4,1,
    2,3,5,7,
    6,4,1,2
};

int dxf_penmap[] = {
    7,1,3,5,
    4,6,2,7,
    9,8,7,1,
    3,5,4,6
};


void ps_set_pen(int pen) 
{
    if (debug) printf("setting pen:%d\n", pen); 

    if (!colorflag) {
    	pen = 0;	// when !colorflag force to black
    }

    if (pennum != pen) {
       newpen=1;
    }

    if (outputtype == DXF) {
       pennum=dxf_penmap[pen%16];
    } else if (outputtype == HPGL) {
       pennum=hpgl_penmap[pen%16];
    } else {
       pennum=pen;
    }

    if (colorflag != 1) {
       pennum=0;		// force black and white
    }
}

char *pen_to_svg_color(int pen) {
    switch (pen) {
       default:
       case 0:
	return("black");
	break;
       case 1:
	return("red");
	break;
       case 2:
	return("green");
	break;
       case 3:
	return("blue");
	break;
       case 4:
	// return("cyan");
	return("slateblue");
	break;
       case 5:
	return("magenta");
	break;
       case 6:
	// return("yellow");
	return("gold");
	break;
       case 7:
	return("black");
	break;
       case 8:
	return("slategrey");
	break;
       case 9:
	return("darkgrey");
	break;
    }
}

/*
Letter  0 0 8.5i 11i 
Legal   0 0 8.5i 14i
Tabloid 0 0  11i 17i
ANSI-C  0 0  17i 22i
ANSI-D  0 0  22i 34i
ANSI-E  0 0  34i 44i
*/

/* the ps interpreter evidently pays attention to pagesize labels... */
/* can't say Letter and give Ansi-A page sizes... */


void ps_preamble(
    char *dev,
    char *prog,
    double pdx, double pdy, 	/* page size in inches */
    double llx, double lly,	/* drawing bounding box in screen coords */ 
    double urx, double ury  
) {

    time_t timep;
    char buf[MAXBUF];
    double xmid, ymid;
    double xdel, ydel;
    double s1, s2, scale;
    double xmax, ymax;
    int landscape;
    double tmp;
    int debug=1;

    if (debug) printf("ps_preamble:\n");

    /* some preliminaries... */
    /* create transform commands based on bb and paper size */

    bbllx=llx; bblly=lly; bburx=urx; bbury=ury;
    if (debug) printf("llx:%f lly:%f urx:%f ury:%f\n", llx, lly, urx, ury);

    /* FIXME, check that bb is in canonic order */
    if ((llx > urx) || (lly > ury)) {
    	printf("bounding box error in postscript.c\n");
    }

    xmid = (llx+urx)/2.0;
    ymid = (lly+ury)/2.0;
    xdel = 1.05*fabs(urx-llx);
    ydel = 1.05*fabs(ury-lly);

    if ( outputtype == HPGL) {
	// 0.0025mm per step = 1016 ticks/in
	xmax=pdx*1016.0;
	ymax=pdy*1016.0;
    } else if ( outputtype == POSTSCRIPT) {
	// 72 pts/in
	xmax=pdx*72.0;
	ymax=pdy*72.0;

	printf("aspect is xdel: %f, ydel: %f\n", xdel, ydel);
	if (xdel > ydel) { /* flip aspect */
	    landscape=1;
	    printf("setting landscape\n");
	    tmp=xmax;  xmax=ymax; ymax=tmp;
	} else {
	    printf("setting portrait\n");
	    landscape=0;
	}
    }

    s1 = xmax/xdel;
    s2 = ymax/ydel;

    scale = s2;
    if (s1 < s2) {
    	scale = s1;
    }

    if ( outputtype == AUTOPLOT) {
       fprintf(fp,"back\n");
       fprintf(fp,"isotropic\n");
       return;
    } else if ( outputtype == SVG || outputtype == WEB) {
        lly = bblly-(lly-bbury);
        ury = bblly-(ury-bbury);
	if (outputtype == WEB) {
	    fprintf(fp,"<html>\n");
	    fprintf(fp,"<body>\n");
	}	
	fprintf(fp,"<?xml version=\"1.0\" standalone=\"no\"?>\n");
	fprintf(fp,"<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"\n");
	fprintf(fp,"\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n");
	if (outputtype == WEB) {
	    fprintf(fp,"<title> %s </title>\n", dev);
	}
	fprintf(fp,"<svg xmlns=\"http://www.w3.org/2000/svg\"\n");
	fprintf(fp,"xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n");

	V_to_R(&llx,&lly);
	V_to_R(&urx,&ury);

	// bounding box for output geometries
	// printf("<!-- llx:%g lly:%g urx:%g ury:%g -->\n", llx, lly, urx, ury );

	// set default pen width to be a fraction diagonal
	ps_set_linewidth(sqrt(pow(fabs(urx-llx),2.0)+pow(fabs(ury-lly),2.0))/700.0);

	if (outputtype == SVG) {
	    fprintf(fp,"width=\"100%%\" height=\"100%%\" viewBox=\"%g %g %g %g\" preserveAspectRatio=\"xMinYMin meet\">\n", 
		llx, lly, urx-llx, ury-lly);
	}
	if (outputtype == WEB) {
	    fprintf(fp,"width=\"100%%\" viewBox=\"%g %g %g %g\" preserveAspectRatio=\"xMinYMin meet\">\n", 
		llx, lly, urx-llx, ury-lly);
	    fprintf(fp,"<style>");
	    fprintf(fp,"   rect:hover {");
	    fprintf(fp,"      opacity: 0.5");
	    fprintf(fp,"   }\n");
	    fprintf(fp,"</style>\n");
	}
	// fprintf(fp,"<h1> %s </h1>\n", dev);
	// fprintf(fp,"<!-- program %s -->\n", prog);
    } else if (outputtype == GERBER) {
       fprintf(fp,"G4 Title: %s*\n", dev);
       fprintf(fp,"G4 Creator: %s*\n", prog);
       timep=time(&timep);
       ctime_r(&timep, buf);
       buf[strlen(buf)-1]='\0';
       fprintf(fp,"G4 CreationDate: %s*\n", buf);
       gethostname(buf,MAXBUF);
       fprintf(fp,"G4 For: %s@%s (%s)*\n", 
       		getpwuid(getuid())->pw_name, buf, getpwuid(getuid())->pw_gecos );
       fprintf(fp,"G01*\n");	// linear interpolation
       fprintf(fp,"G70*\n");	// inches
       fprintf(fp,"G90*\n");	// absolute
       fprintf(fp,"%%MOIN*%%\n");	// absolute
       fprintf(fp,"G04 Gerber Fmt 3.4, Leading zero omitted, Abs format*\n");
       fprintf(fp,"%%FSLAX34Y34*%%\n");
       fprintf(fp,"G04 Define D10 to be a 5mil circle*\n");
       fprintf(fp,"%%ADD10C,.005*%%\n");
       fprintf(fp,"G04 Make D10 the default aperture*\n");
       fprintf(fp,"G54D10*\n");
       return;
    } else if (outputtype == DXF) {
       fprintf(fp, "  0\n");
       fprintf(fp, "SECTION\n");
       fprintf(fp, "  2\n");
       fprintf(fp, "ENTITIES\n");
       return;
    } else if (outputtype == HPGL) {
       fprintf(fp, "IN;PU;\n");
       fprintf(fp, "SP%d;\n", pennum);

       // HPGL plots in units of 0.025 mm = 1016 ticks per inch...
       // fprintf(fp, "page size is %f %f\n", pdx*1016.0, pdy*1016.0);
       // fprintf(fp, "bb is llx:%f lly:%f urx:%f ury%f\n", llx, lly, urx, ury);
       fprintf(fp, "IP %d,%d,%d,%d;\n", 0, 0, (int) ((urx-llx)*scale), (int) ((ury-lly)*scale) );

       // flip y axis (Xwin puts 0,0 at top, increments going down)
       fprintf(fp, "SC %.4f,%.4f,%.4f,%.4f;\n", llx, urx, ury, lly);

       fprintf(fp, "PA0,0;\n");

	//int i,j; char *ps;
	//fprintf(fp,"CO \"define stipple patterns\";\n");
	//for (i=0;(ps=get_stipple_bits(i))!=NULL;i++) {
	//    fprintf(fp,"RF%d,8,8",i);
	//    for (j=0; j<=7; j++) {
	//	for(k=0; k<=7; k++) {
	//	   fprintf(fp, ",%c", ((ps[j]>>k)&1)?'1':'0');
	//	}
	//    }
	//    fprintf(fp,";\n");
	//}

       return;
    } else if (outputtype == POSTSCRIPT) {
	fprintf(fp,"%%!PS-Adobe-2.0\n");
	fprintf(fp,"%%%%Title: %s\n", dev);
	fprintf(fp,"%%%%Creator: %s\n", prog);
	// timep=time(&timep);
	fprintf(fp,"%%%%CreationDate: %s", ctime(&timep));
	gethostname(buf,MAXBUF);
	fprintf(fp,"%%%%For: %s@%s (%s)\n", 
	    getpwuid(getuid())->pw_name, 
	    buf,
	    getpwuid(getuid())->pw_gecos );
	if (landscape) {
	    fprintf(fp,"%%%%Orientation: Landscape\n");
	} else {
	    fprintf(fp,"%%%%Orientation: Portrait\n");
	}
	fprintf(fp,"%%%%Pages: (atend)\n");
	fprintf(fp,"%%%%BoundingBox: 0 0 %d %d\n", 
	    (int) (pdx*72.0), (int) (pdy*72.0));
	fprintf(fp,"%%%%DocumentPaperSizes: custom\n");
	fprintf(fp,"%%%%Magnification: 1.0000\n");
	fprintf(fp,"%%%%EndComments\n");

	fprintf(fp,"%%%%BeginSetup\n");
	fprintf(fp,"mark {\n");
	fprintf(fp,"%%BeginFeature: *PageRegion custom\n");
	fprintf(fp,"<</PageSize [ %d %d ] /ImagingBBox null >> setpagedevice\n",
	    (int)( pdx*72.0), (int) (pdy*72.0));
	fprintf(fp,"%%EndFeature\n");
	fprintf(fp,"} stopped cleartomark\n");
	fprintf(fp,"%%%%EndSetup\n");

	fprintf(fp,"%%%%BeginProlog\n");
	fprintf(fp,"/$Pig2psDict 200 dict def\n");
	fprintf(fp,"$Pig2psDict begin\n");
	fprintf(fp,"$Pig2psDict /mtrx matrix put\n");
        if (colorflag==1) { 	// color
	    fprintf(fp,"/c-1 {0 setgray} bind def\n");
	    fprintf(fp,"/c0 {0.000 0.000 0.000 srgb} bind def\n");
	    fprintf(fp,"/c1 {1.000 0.000 0.000 srgb} bind def\n");
	    fprintf(fp,"/c2 {0.000 1.000 0.000 srgb} bind def\n");
	    fprintf(fp,"/c3 {0.000 0.000 1.000 srgb} bind def\n");
	    fprintf(fp,"/c4 {0.000 1.000 1.000 srgb} bind def\n");
	    fprintf(fp,"/c5 {1.000 0.000 1.000 srgb} bind def\n");
	    fprintf(fp,"/c6 {1.000 0.700 0.000 srgb} bind def\n");
	    fprintf(fp,"/c7 {0.000 0.000 0.000 srgb} bind def\n");
	    fprintf(fp,"/c8 {0.300 0.300 0.300 srgb} bind def\n");
	    fprintf(fp,"/c9 {0.600 0.600 0.600 srgb} bind def\n");
	} else if (colorflag==0) {	// black
	    fprintf(fp,"/c-1 {0 setgray} bind def\n");
	    fprintf(fp,"/c0 {0.000 0.000 0.000 srgb} bind def\n");
	    fprintf(fp,"/c1 {0.000 0.000 0.000 srgb} bind def\n");
	    fprintf(fp,"/c2 {0.000 0.000 0.000 srgb} bind def\n");
	    fprintf(fp,"/c3 {0.000 0.000 0.000 srgb} bind def\n");
	    fprintf(fp,"/c4 {0.000 0.000 0.000 srgb} bind def\n");
	    fprintf(fp,"/c5 {0.000 0.000 0.000 srgb} bind def\n");
	    fprintf(fp,"/c6 {0.000 0.000 0.000 srgb} bind def\n");
	    fprintf(fp,"/c7 {0.000 0.000 0.000 srgb} bind def\n");
	    fprintf(fp,"/c8 {0.300 0.300 0.300 srgb} bind def\n");
	    fprintf(fp,"/c9 {0.600 0.600 0.600 srgb} bind def\n");
	} else {			// gray
	    fprintf(fp,"/c-1 {0 setgray} bind def\n");
	    fprintf(fp,"/c0 {0.000 0.000 0.000 srgb} bind def\n");
	    fprintf(fp,"/c1 {0.200 0.200 0.200 srgb} bind def\n");
	    fprintf(fp,"/c2 {0.300 0.300 0.300 srgb} bind def\n");
	    fprintf(fp,"/c3 {0.400 0.400 0.400 srgb} bind def\n");
	    fprintf(fp,"/c4 {0.500 0.500 0.500 srgb} bind def\n");
	    fprintf(fp,"/c5 {0.600 0.600 0.600 srgb} bind def\n");
	    fprintf(fp,"/c6 {0.700 0.700 0.700 srgb} bind def\n");
	    fprintf(fp,"/c7 {0.000 0.000 0.000 srgb} bind def\n");
	    fprintf(fp,"/c8 {0.300 0.300 0.300 srgb} bind def\n");
	    fprintf(fp,"/c9 {0.600 0.600 0.600 srgb} bind def\n");
	}
	fprintf(fp,"/cp {closepath} bind def\n");
	fprintf(fp,"/ef {eofill} bind def\n");
	fprintf(fp,"/gr {grestore} bind def\n");
	fprintf(fp,"/gs {gsave} bind def\n");
	fprintf(fp,"/sa {save} bind def\n");
	fprintf(fp,"/rs {restore} bind def\n");
	fprintf(fp,"/kc { currentrgbcolor [/Pattern /DeviceRGB] setcolorspace } bind def\n");
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

	fprintf(fp,"%% start stipple patterns\n");
	int i,j; unsigned char *ps;
	for (i=0;(ps=get_stipple_bits(i))!=NULL;i++) {
	    fprintf(fp,"/p%d {\n",i);
	    fprintf(fp,".5 .5 scale 8 8 true  matrix {<");
	    for (j=0; j<=7; j++) {
		fprintf(fp,"%02x",ps[j]&255);
	    }
	    fprintf(fp,">} imagemask\n");
	    fprintf(fp,"} bind def\n");
	    fprintf(fp,"<< /PatternType 1\n");
	    fprintf(fp,"   /PaintType 2\n");
	    fprintf(fp,"   /TilingType 1\n");
	    fprintf(fp,"   /XStep 4 /YStep 4\n");
	    fprintf(fp,"   /BBox [0 0 8 8]\n");
	    fprintf(fp,"   /PaintProc { p%d }\n",i);
	    fprintf(fp,">>\n");
	    fprintf(fp,"matrix makepattern /f%d exch def\n",i);
	}
	fprintf(fp,"%% end stipple patterns\n");

	fprintf(fp,"end\n");
	fprintf(fp,"save\n");

	// fprintf(fp,"newpath\n");
	// fprintf(fp,"0 %d moveto\n", (int) (pdy*72.0));
	// fprintf(fp,"0 0 lineto\n");
	// fprintf(fp,"%d 0 lineto\n", (int) (pdx*72.0));
	// fprintf(fp,"%d %d lineto closepath clip newpath\n", 
	// 	(int) (pdx*72.0), (int) (pdy*72.0));

	fprintf(fp,"/$Pig2psBegin\n");
	fprintf(fp,"{$Pig2psDict begin /$Pig2psEnteredState save def}def\n");
	fprintf(fp,"/$Pig2psEnd {$Pig2psEnteredState restore end} def\n");

	fprintf(fp,"%%BeginPageSetup\n");
	fprintf(fp,"%%BB is %f,%f %f,%f\n", llx, lly, urx, ury);	
	if (landscape) {
	    fprintf(fp,"%f %f scale\n", scale, scale);
	    fprintf(fp,"%f %f translate\n", 
		(ymax/(2.0*scale))+ymid, (xmax/(2.0*scale))-xmid);
	    fprintf(fp,"90 rotate\n");
	} else {
	    fprintf(fp,"%f %f scale\n", scale, scale);
	    fprintf(fp,"%f %f translate\n",
		(xmax/(2.0*scale))-xmid,  (ymax/(2.0*scale))-ymid);
	}
	fprintf(fp,"%%EndPageSetup\n");
	fprintf(fp,"%%%%EndProlog\n");

	fprintf(fp,"$Pig2psBegin\n");
	fprintf(fp,"10 setmiterlimit\n");
	fprintf(fp,"1 slj 1 slc\n");
	fprintf(fp,"%.1f slw\n",linewidth);

	fprintf(fp,"%% here starts figure;\n");
	return;

    } else {
        printf("unrecognized plot type in postscript.c\n");
	return;
    }
}

void ps_comment(char *comment)
{
    if ((fp != NULL)) {
	if (outputtype == GERBER) {		// GERBER
	    ;
	} else if (outputtype == HPGL) {	// HPGL
	    ;
	} else if (outputtype == DXF) {		// DXF
	    ;
	} else if (outputtype == POSTSCRIPT) {	// PS
	    ;
	} else if (outputtype == SVG) {		// Scalable vector graphics
	    // fprintf(fp, "<!--%s-->\n",comment);
	} else if (outputtype == WEB) {		// WEB = html
	    // fprintf(fp, "<!--%s-->\n",comment);
	} else if (outputtype == AUTOPLOT) {	// AUTOPLOT
	    fprintf(fp, "#%s\n",comment);
	}
    }
}


void ps_end_line()
{
    extern int this_pen;
    extern int this_line;
    extern int in_line;
    extern int this_isfilled;
    int debug = 0;

    if (debug) printf("ps_end_line:\n");

    if (outputtype == GERBER) {			// GERBER
       if (in_poly) {
	   fprintf(fp, "G37*\n");
       }
    } else if (outputtype == SVG || outputtype == WEB) {			// SVG
       fprintf(fp,
       "\"\nstyle=\"fill:none;stroke:%s;stroke-width:%g;stroke-linejoin:round;\"/>\n",
	pen_to_svg_color(this_pen),linewidth);
    } else if (outputtype == HPGL) {			// HPGL
       if (in_poly) {
	   fprintf(fp, "PM2;\n");
	   if (this_isfilled) {
	       fprintf(fp, "%s\n", get_hpgl_fill(this_fill));
	       fprintf(fp, "LT; FP;\n");
	   }
	   if (this_line == 0) {
	       fprintf(fp, "LT;EP;\n");
	   } else {
	       fprintf(fp, "LT%d;EP;\n", this_line);
	   }
	}
    } else if (outputtype == POSTSCRIPT) {
	fprintf(fp, "c%d ", this_pen);
	if (in_poly) {
	   fprintf(fp, "gs kc f%d setpattern fill gr\n",
	   get_stipple_index(this_fill,this_pen));
	}
	fprintf(fp, "%s ", xwin_ps_dashes(this_line));
	fprintf(fp, "s\n");
    }
    in_poly=0;
    in_line=0;
}

void ps_flush() {
    if (in_line) {
    	ps_end_line();
    } 
    in_line=0;
}

double xold, yold;
int in_progress=0;

void ps_start_line(double x1, double y1, int filled)
{
    extern int pennum;
    extern int linetype;
    extern int in_line;
    extern int this_pen;
    extern int this_line;
    extern int this_fill;
    extern int this_isfilled;

    if (debug) printf("ps_start_line:\n");

    if (in_line) {
    	ps_end_line();
    } 
    in_line++;

    if (filled) {
	if (outputtype == GERBER) {			// GERBER
	   fprintf(fp,"G04 LAYER %d *\n", layer);
	   fprintf(fp, "G36*\n");
	}
	in_poly++;
    }


    this_pen=pennum;		/* save characteristics at start of line */
    this_line=linetype;
    this_fill=filltype;
    this_isfilled=filled;

    if (outputtype == SVG || outputtype == WEB) {				// SVG
	fprintf(fp, "<polyline points=\"\n");
        y1 = bblly-(y1-bbury);
	V_to_R(&x1,&y1);
	fprintf(fp, "  %g,%g\n", x1, y1);
    } else if (outputtype == POSTSCRIPT) {		// Postscript
	fprintf(fp, "n\n");
        // flip y coordinate from Xwin coords
        y1 = bblly-(y1-bbury);
	fprintf(fp, "%.10g %.10g m\n",x1, y1);
    } else if (outputtype == AUTOPLOT) {		// AUTOPLOT
	fprintf(fp, "jump\n");
	fprintf(fp, "pen %d\n", pennum);
	V_to_R(&x1, &y1);
	fprintf(fp, "%.10g %.10g\n",x1, y1);
    } else if (outputtype == GERBER) {			// GERBER
	V_to_R(&x1, &y1);
        if (!in_poly) {
	   fprintf(fp,"G04 LAYER %d *\n", layer);
	}
	fprintf(fp, "X%04dY%04dD02*\n",(int)((x1*10000.0)+0.5), (int)((y1*10000.0)+0.5));
    } else if (outputtype == DXF) {			// DXF
        V_to_R(&x1, &y1);    
	// fprintf(fp, "  0\n");		// start of entity
	// fprintf(fp, "LINE\n");		// line type entity
	// fprintf(fp, "  8\n");		// layer name to follow
	// fprintf(fp, "%d\n",layer);	// layer name 
	// fprintf(fp, "  62\n");		// color flag
	// fprintf(fp, "%d\n", pennum);	// color
	// fprintf(fp, "  10\n%.10g\n", x1);	// initial x value
	// fprintf(fp, "  20\n%.10g\n", y1);	// initial y value
	xold=x1;
	yold=y1;
	in_progress=0;
    } else if (outputtype == HPGL) {
	if (!filled) {
	   if (this_line == 0) {
	       fprintf(fp, "LT;\n");
	   } else {
	       fprintf(fp, "LT%d;\n", this_line);
	   }
	}
	fprintf(fp, "SP%d;\n", this_pen);
        fprintf(fp, "PU %.4f,%.4f;\n",x1,y1);
        if (filled) {
	   fprintf(fp,"PM0;\n");
	}
    }
}


void ps_continue_line(double x1, double y1)
{
    if (debug) printf("ps_continue_line:\n");

    if (outputtype == SVG || outputtype == WEB) {				// SVG
        y1 = bblly-(y1-bbury);
	V_to_R(&x1, &y1);
	fprintf(fp, "  %g,%g\n", x1, y1);
    } else if (outputtype == POSTSCRIPT) {
	// flip y coordinate from Xwin coords
	y1 = bblly-(y1-bbury);
	fprintf(fp, "%.10g %.10g l\n",x1, y1);
    } else if (outputtype == AUTOPLOT) {		// AUTOPLOT
	V_to_R(&x1, &y1);
	fprintf(fp, "%.10g %.10g\n",x1, y1);
    } else if (outputtype == GERBER) {			// GERBER
	V_to_R(&x1, &y1);
	fprintf(fp, "X%04dY%04dD01*\n",(int)((x1*10000.0)+0.5), (int)((y1*10000.0)+0.5));
    } else if (outputtype == DXF) {
	V_to_R(&x1, &y1);
	if (!in_progress) {
	    fprintf(fp, "  0\n");		// start of entity
	    fprintf(fp, "LINE\n");		// line type entity
	    fprintf(fp, "  8\n");		// layer name to follow
	    fprintf(fp, "%d\n",layer);	// layer name 
	    fprintf(fp, "  62\n");		// color flag
	    fprintf(fp, "%d\n", pennum);	// color
	    fprintf(fp, "  10\n%.10g\n", xold);	// initial x value
	    fprintf(fp, "  20\n%.10g\n", yold);	// initial y value
	    fprintf(fp, "  11\n%.10f\n", x1);	// initial x value
	    fprintf(fp, "  21\n%.10f\n", y1);	// initial y value
	    in_progress++;
	} else {
	    fprintf(fp, "  0\n");		// start of entity
	    fprintf(fp, "LINE\n");		// line type entity
	    fprintf(fp, "  8\n");		// layer name to follow
	    fprintf(fp, "%d\n",layer);		// layer name 
	    fprintf(fp, "  62\n");		// color flag
	    fprintf(fp, "%d\n", pennum);	// color
	    fprintf(fp, "  10\n%.10f\n", xold);	// initial x value
	    fprintf(fp, "  20\n%.10f\n", yold);	// initial y value
	    fprintf(fp, "  11\n%.10f\n", x1);	// initial x value
	    fprintf(fp, "  21\n%.10f\n", y1);	// initial y value
	}
	xold = x1;
	yold = y1;
    } else if (outputtype == HPGL) {			// HPGL
        fprintf(fp, "PD %.4f,%.4f;\n",x1,y1);
    }
    in_line++;
}

void ps_postamble()
{
    // time_t timep;
    // char buf[MAXBUF];

    if (debug) printf("ps_postamble:\n");
    if (in_line) {
    	ps_end_line();
	in_line=0;
    } 
    if (outputtype == POSTSCRIPT) {
	fprintf(fp,"%% here ends figure;\n");
	fprintf(fp,"$Pig2psEnd\n");
	fprintf(fp,"showpage\n");
	fprintf(fp,"%%%%EOF\n");
    } else if (outputtype == GERBER) {
        fprintf(fp,"G04 LAYER 999 *\n");
	fprintf(fp,"M02*\n");
    } else if (outputtype == DXF) {
        fprintf(fp,"  0\nENDSEC\n");
        fprintf(fp,"  0\nEOF\n");
    } else if (outputtype == HPGL) {
        fprintf(fp,"PU;\n");
        fprintf(fp,"SP;\n");
        fprintf(fp,"IN;\n");
    } else if (outputtype == SVG || outputtype == WEB) {
        fprintf(fp,"</svg>\n");
	if (outputtype == WEB) {
	    fprintf(fp,"</body>\n");
	    fprintf(fp,"</html>\n");
	}
	//fprintf(fp,"<hr>\n");
        //timep=time(&timep);
        //ctime_r(&timep, buf);
        //buf[strlen(buf)-1]='\0';
	////fprintf(fp,"<p> creation date %s<br>\n", buf);
        //gethostname(buf,MAXBUF);
        //fprintf(fp,"<p> For: %s@%s (%s) <br>\n",
        //    getpwuid(getuid())->pw_name,
        //    buf,
        //    getpwuid(getuid())->pw_gecos );
	
    } 
    fclose(fp); fp=NULL;
}


void ps_link(int nest, char *name, double xmin, double ymin, double xmax, double ymax) {

    // FIXME: protect against null pointer...
    char *p;	// link to prefix
    char *s;	// link to suffix
    char *e;
    char buf[MAXBUF];

    strncpy(buf, name, MAXBUF);

    if ((fp != NULL) && outputtype == WEB && nest==1) {

	if ((e=strstr(buf, "_sym"))!=NULL) {	// check if sym
	    *e = '\0';				// truncate suffix
	    p=EVget("PIG_HTML_SYM_PREFIX");
	    s=EVget("PIG_HTML_SYM_SUFFIX");
	} else {
	    p=EVget("PIG_HTML_PREFIX");
	    s=EVget("PIG_HTML_SUFFIX");
	}

        ymin = bblly-(ymin-bbury);
        ymax = bblly-(ymax-bbury);
	V_to_R(&xmin,&ymin);
	V_to_R(&xmax,&ymax);
	// fprintf(fp, "\n<!-- %g %g %g %g-->\n", xmin, ymin, xmax, ymax);
	fprintf(fp, "\n<a xlink:href=\"%s%s%s\">\n", p, buf, s);
	fprintf(fp, "    <rect x=\"%g\" y=\"%g\" fill=\"#ff0\" opacity=\"0.2\" width=\"%g\" height=\"%g\"/>\n",
	xmin,ymax,xmax-xmin,ymin-ymax);
	fprintf(fp, "</a>\n");
    }
}

