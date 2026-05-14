 /* Example program fragment to read a PAM or PNM image
  * from stdin, add up the values of every sample in it
  * and write the image unchanged to stdout. */

// cc pbm.c -o pbm  -lnetpbm -lm

#include <netpbm/pam.h>

struct pam inpam, outpam;
tuple *tuplerow;
unsigned int row;
int total=0;

main(int argc, char **argv)
{
    unsigned int column;
    unsigned int plane;
    tuple **image;

    pm_init(argv[0], 0);

    // read in the image, setting values in inpam
    image=pnm_readpam(stdin, &inpam, sizeof(struct pam));

    // make the output same size, but different file
    outpam = inpam;
    outpam.file = stdout;
    outpam.width = inpam.width/2;

    pnm_writepaminit(&outpam); 		// write header
    pnm_writepam(&outpam, image);	// write image

}
