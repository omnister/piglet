/* ************************************************************* 

this set of routines manages the rubber band drawing function for
interactive point selection.  The rubber band shapes are drawn with an
xor function such that drawing a line twice will erase it.  We need to
insure that the first shape drawn after rubber_set_callback() gets
erased before the second shape is drawn, and so on, until the last call
is made by rubber_clear_callback(). 

    rubber_set_callback();
    First call (x1,y1,0):
	    draw x1, y1

    Second call (x2,y2,1)
	    draw x1, y1			# erase last shape
	    draw x2, y2			# drawn new shape

    (N-1)th call (x(n-1),y(n-1), n-1)
	    draw x(n-2), y(n-2)		# erase last shape
	    draw x(n-1), y(n-1)		# draw new shape

    rubber_clear_callback();
    Last call (xn, yn, -1)
	    draw x(n-1), y(n-1)		# erase last shape
	                                # and DONT draw new shape


The callback function manages this behavior by the count variable passed
in the third argument.  When count==0, it only draws the specified
shape.  When count>=0 it always undraws the old shape prior to drawing
the new shape.  When count<=0 it will undraw the old shape, and NOT draw
a new shape.  rubber_clear_callback() is responsible for making the last
call with count=-1. 

************************************************************* */

#include <stdio.h>
#include "rubber.h"

typedef int Function ();

static int count=0;
Function * rubber_callback = NULL;

void rubber_set_callback(func) 
Function * func;
{
    count=0;
    rubber_callback = func;
}

void rubber_clear_callback() 
{
    /* let callback do final erase */
    if (rubber_callback != NULL) {
	(*(rubber_callback)) (0.0, 0.0, -1);
    }

    /* clear the callback function */
    rubber_callback = (Function *) NULL;

    count=0;
}

/* called with count == 0 on first call, count++ subsequent calls, and
 *  -1 on final call.  The drawing routine must erase previous draws on
 *  all calls with count >0.  
 */

void rubber_draw(x,y)
double x, y;
{
    int debug=0;
    if (rubber_callback != NULL) {
	if (debug) printf("rubber_draw: drawing: %g %g %d\n", x,y, count); 
	(*(rubber_callback)) (x, y, count);
	count++;
    }
}

