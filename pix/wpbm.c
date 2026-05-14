#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <netpbm/pam.h> 

// Preliminary work for adding bitmaps to piglet to allow easy tracing of
// photographic shapes...
//
// cc wpbm.c -o wpbm  -lnetpbm -lm -lX11
// 
// using this read in kernel it would be possible to give a file argument to 
// piglet's add rectangle command which would readin a pam image and fit it
// to the rectangular display area.  There could be an option to allow the rectangle
// to be freely stretched and an option to preserve the aspect ratio
// 

struct pam inpam, outpam;
tuple *tuplerow;
unsigned int row;
int total=0;
double scale=0.7;	// how much to scale output image

XImage *CreateTrueColorImage(Display *dpy, Visual *visual,
        unsigned char *image, int width, int height, struct pam inpam,
	tuple **src)
{
    int i,j;
    unsigned char *image32 = (unsigned char *) malloc(width * height * 4);
    unsigned char *p = image32;
    
    double rowscale;
    double colscale;
    unsigned int row;
    unsigned int column;
    unsigned int plane;

    rowscale = ((double) inpam.height)/((double) height); 
    colscale = ((double) inpam.width)/((double) width);

    // printf("image is h:%d w:%d\n", inpam.height, inpam.width);
    // printf("graphics is h:%d w:%d\n", height, width);
    // printf("scale is is h:%g w:%g\n", (rowscale), (colscale));

    for (row=0; row<height; row++) {
	for (column=0; column<width; column++) {
	   *p++ = src[(int)((double)row*rowscale)][(int)((double)column*colscale)][2]; // red
	   *p++ = src[(int)((double)row*rowscale)][(int)((double)column*colscale)][1]; // green
	   *p++ = src[(int)((double)row*rowscale)][(int)((double)column*colscale)][0]; // blue
	   p++;
	   // printf("doing %d %d %d %d\n", row, column,
	   // (int)((double)row*rowscale),
	   // (int)((double)column*colscale));
	}
    }

    return XCreateImage(dpy, visual, 24, ZPixmap, 0, image32, width, height, 32, 0);
}

void processEvent(Display *dpy, Window window, XImage *ximage, int *width, int *height)
{
    XEvent ev;
    XNextEvent(dpy, &ev);
    switch (ev.type) {
        case Expose:
	   XPutImage(dpy, window, DefaultGC(dpy,0), 
	   	ximage, 0,0,0,0, *width, *height);
	   break;
	case ButtonPress:
	   exit(0);
	   break;
	case ConfigureNotify:
	    *width=ev.xconfigure.width;
	    *height=ev.xconfigure.height;
	   break;
	default:
	   break;
    }
}


char *progname;

int main(int argc, char **argv) 
{
    XImage *ximage;
    int width = 500, height=500;
    Display *dpy = XOpenDisplay(NULL);
    Visual *visual = DefaultVisual(dpy, 0);
    Window window = 
    	XCreateSimpleWindow(dpy, RootWindow(dpy,0), 0, 0, width, height, 1, 0, 0);

    unsigned int row;		// variables for pam image readin
    unsigned int column;
    unsigned int plane;
    tuple **in_image;
    tuple **out_image;
    FILE *fp;

    progname = argv[0];

    if (visual->class != TrueColor) {
       fprintf(stderr, "can't handle non TrueColor visual\n");
       exit(1);
    }

    if (argc > 1) {
	printf("%s\n", argv[1]);
	if((fp = fopen(argv[1], "r"))==NULL) {
	    fprintf(stderr, "cannot open %s\n",argv[1]);
	    exit(2);
	}
    } else {
	fprintf(stderr, "error: no pam file specified for input\n");
	fprintf(stderr, "usage: %s <inputfile>\n", progname);
	exit(1);
    }

    pm_init(argv[0], 0);

    // read in the image, setting values in inpam
    // in_image=pnm_readpam(stdin, &inpam, PAM_STRUCT_SIZE(tuple_type));
    in_image=pnm_readpam(fp, &inpam, PAM_STRUCT_SIZE(tuple_type));

    ximage = CreateTrueColorImage(dpy, visual, 0, width, height ,inpam, in_image);

    XSelectInput(dpy, window, ButtonPressMask | ExposureMask | StructureNotifyMask);
    XMapWindow(dpy, window);

    // while(1) {
    //    processEvent(dpy, window, ximage, &width, &height);
    // }

    while(1) {
	XEvent ev;
	XNextEvent(dpy, &ev);
	switch (ev.type) {
	    case Expose:
	       XPutImage(dpy, window, DefaultGC(dpy,0), 
		    ximage, 0,0,0,0, width, height);
	       break;
	    case ButtonPress:
	       exit(0);
	       break;
	    case ConfigureNotify:
		ximage = CreateTrueColorImage(dpy, visual, 0, width, height ,inpam, in_image);
		// printf("got config\n");
		width=ev.xconfigure.width;
		height=ev.xconfigure.height;
	        break;
	    default:
	       break;
	}
    }
}

// simply copies upper left justified version of src to dest (stretching
// to fit) assumes tuples have same dimensions as described in
// associated pam structs.  No interpolation or smoothing is done.

void pnmcopy(struct pam pamsrc, tuple **src, struct pam pamdst, tuple **dst) {
    unsigned int row;
    unsigned int column;
    unsigned int plane;

    double rowscale;
    double colscale;

    rowscale = ((double) pamdst.height)/((double) pamsrc.height);
    colscale = ((double) pamdst.width)/((double) pamsrc.width);

    for (row=0; row<outpam.height; row++) {
	for (column=0; column<outpam.width; column++) {
	   dst[row][column][0] = src[(int)((double)row/rowscale)][(int)((double)column/colscale)][0];
	   dst[row][column][1] = src[(int)((double)row/rowscale)][(int)((double)column/colscale)][1];
	   dst[row][column][2] = src[(int)((double)row/rowscale)][(int)((double)column/colscale)][2];
	}
    }

}
