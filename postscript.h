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

typedef enum {AUTOPLOT, POSTSCRIPT, GERBER, DXF} OMODE;

extern void ps_set_file(FILE *fp);		// set file descriptor for output
extern void ps_set_outputtype(OMODE mode);	// 0=postscript; 1=gerber

extern void ps_set_pagesize(double x, double y);  // set pagesize
extern void ps_preamble();			// output standard header

extern void ps_set_layer();			// set layer 
extern void ps_set_line();			// change line type
extern void ps_set_pen();			// change pen color
extern void ps_set_fill();			// change fill pattern

extern void ps_start_line();			// start a new line
extern void ps_continue_line();			// continue a line
extern void ps_postamble();			// wrap it up

