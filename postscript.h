//
// To use this library, first call ps_set_file() with a file descriptor to write to
//
// then call ps_set_outputtype() to select type of output file
// the call ps_preamble() to set up scaling
// after that, you can change pen,line and fill at will
// 
// start simple lines with ps_start_line(), extend them with ps_continue_line()
// if you want a filled path, then surround your list of points
// with ps_start_poly() and ps_end_poly();
//
// when done plotting all the geometries, call ps_postamble();
//

typedef enum {AUTOPLOT, POSTSCRIPT, GERBER} OMODE;

extern void ps_set_file(FILE *fp);		// set file descriptor for output
extern void ps_set_outputtype(OMODE mode);	// 0=postscript; 1=gerber

extern void ps_preamble();			// output standard header

extern void ps_set_layer();			// set layer 
extern void ps_set_line();			// change line type
extern void ps_set_pen();			// change pen color
extern void ps_set_fill();			// change fill pattern

extern void ps_start_poly();			// start a poly
extern void ps_start_line();			// start a new line
extern void ps_continue_line();			// continue a line
extern void ps_end_poly();			// end a poly

extern void ps_postamble();			// wrap it up
