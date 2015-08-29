#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "lock.h"
#include "db.h"
#include "xwin.h"

static double xsnap, ysnap;

void setlockpoint(double x,double y) 
{
    int debug = 0;

    if (debug) printf("setting snap to %g %g\n", x, y);

    xsnap = x;
    ysnap = y;
}


void getlockpoint(double *px,double *py) 
{
    *px = xsnap;
    *py = ysnap;
}

void lockpoint(double *px, double *py, double lock) 
{
    double dx, dy;
    double theta;
    double locktheta;
    double snaptheta;
    double radius;

    if (lock != 0.0) {

	dx = *px - xsnap;
	dy = *py - ysnap;
	radius = sqrt(dx*dx + dy*dy);

	/* compute angle (range is +/- M_PI) */
	theta = atan2(dy,dx);
	if (theta < 0.0) {
	    theta += 2.0*M_PI;
	}

	locktheta = 2.0*M_PI*lock/360.0;
	snaptheta = locktheta*floor((theta+(locktheta/2.0))/locktheta);

	// printf("angle = %g, locked %g\n", 
	// 	theta*360.0/(2.0*M_PI), snaptheta*360.0/(2.0*M_PI));
	// fflush(stdout);

    	/* overwrite px, py, to produce vector with same radius, but proper theta */
	*px = xsnap+radius*cos(snaptheta);
	*py = ysnap+radius*sin(snaptheta);

	snapxy(px, py);	   /* snap computed points to grid */
    }
}

