#pragma once

//
// To use this library, first call ps_set_file() with a file descriptor to write to
//
// then call ps_set_outputtype() to select type of output file
// the call ps_preamble() to set up scaling
// after that, you can change pen,line and fill at will
// 
// start simple lines with ps_start_line(x,y,fill), 
// extend them with ps_continue_line(x,y)
// if fill=1 the lines will be treated as polygons and filled
//
// when done plotting all the geometries, call ps_postamble();
//

typedef enum {AUTOPLOT, POSTSCRIPT, GERBER, DXF, HPGL, SVG, WEB} OMODE;

extern void ps_set_file(FILE *fp);		// set file descriptor for output
extern void ps_set_outputtype(OMODE mode);	// 0=postscript; 1=gerber
extern void ps_set_linewidth(double width);	// linewidth for ps
extern void ps_set_color(int color);		// 1==color 0=bw

extern void ps_set_pagesize(double x, double y);  // set pagesize
void ps_preamble(char *dev, char *prog, double pdx, double pdy, double llx, double lly, double urx, double ury);

void ps_set_layer(int layernumber);
void ps_set_line(int line);
void ps_set_pen(int pen); 
void ps_set_fill(int fill);
extern void ps_comment(char *comment);		// print an inline comment

extern void ps_link(int nest, char *name, double xmin, double ymin, double xmax, double ymax);
extern void ps_flush(void);


void ps_start_line(double x1, double y1, int filled);   // start a new line
void ps_continue_line(double x1, double y1); // continut a line
extern void ps_postamble(void);			// wrap it up

