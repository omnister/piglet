void drawx(x, y)	/* draw x,y transformed by extern global xf */
NUM x,y;
{

    extern XFORM *transform;
    NUM xx, yy;

    /* generate a description of sparseness of array */

    int i=0;
    if (transform->r11 != 1.0) {i+=1;};
    if (transform->r12 != 0.0) {i+=2;};
    if (transform->r21 != 0.0) {i+=4;};
    if (transform->r22 != 1.0) {i+=8;};
    if (transform->dx  != 0.0) {i+=16;};
    if (transform->dy  != 0.0) {i+=32;};

    switch (i) {
	case 0: 	/* r11=1, r12=0, r21=0, r22=1, dx=0, dy=0 */
	    xx = x;
	    yy = y;
	    break;
	case 1:		/* r11=x, r12=0, r21=0, r22=1, dx=0, dy=0 */
	    xx = x*transform->r11;
	    yy = y;
	    break;
	case 2:		/* r11=1, r12=x, r21=0, r22=1, dx=0, dy=0 */
	    xx = x;
	    yy = x*transform->r12 + y;
	    break;
	case 3:		/* r11=x, r12=x, r21=0, r22=1, dx=0, dy=0 */
	    xx = x*transform->r11 ;
	    yy = x*transform->r12 + y;
	    break;
	case 4: 	/* r11=1, r12=0, r21=x, r22=1, dx=0, dy=0 */
	    xx = x + y*transform->r21;
	    yy = y;
	    break;
	case 5:		/* r11=x, r12=0, r21=x, r22=1, dx=0, dy=0 */
	    xx = x*transform->r11 + y*transform->r21;
	    yy = y;
	    break;
	case 6:		/* r11=1, r12=x, r21=x, r22=1, dx=0, dy=0 */
	    xx = x + y*transform->r21;
	    yy = x*transform->r12 + y;
	    break;
	case 7:		/* r11=x, r12=x, r21=x, r22=1, dx=0, dy=0 */
	    xx = x*transform->r11 + y*transform->r21;
	    yy = x*transform->r12 + y;
	    break;
	case 8: 	/* r11=1, r12=0, r21=0, r22=x, dx=0, dy=0 */
	    xx = x;
	    yy = y*transform->r22;
	    break;
	case 9:		/* r11=x, r12=0, r21=0, r22=x, dx=0, dy=0 */
	    xx = x*transform->r11 + transform->dx;
	    yy = y*transform->r22 + transform->dy;
	    break;
	case 10:	/* r11=1, r12=x, r21=0, r22=x, dx=0, dy=0 */
	    xx = x + transform->dx;
	    yy = x*transform->r12 + y*transform->r22;
	    break;
	case 11:	/* r11=x, r12=x, r21=0, r22=x, dx=0, dy=0 */
	    xx = x*transform->r11;
	    yy = x*transform->r12 + y*transform->r22;
	    break;
	case 12: 	/* r11=1, r12=0, r21=x, r22=x, dx=0, dy=0 */
	    xx = x + y*transform->r21;
	    yy = y*transform->r22;
	    break;
	case 13:	/* r11=x, r12=0, r21=x, r22=x, dx=0, dy=0 */
	    xx = x*transform->r11 + y*transform->r21;
	    yy = y*transform->r22;
	    break;
	case 14:	/* r11=1, r12=x, r21=x, r22=x, dx=0, dy=0 */
	    xx = x + y*transform->r21;
	    yy = x*transform->r12 + y*transform->r22;
	    break;
	case 15:	/* r11=x, r12=x, r21=x, r22=x, dx=0, dy=0 */
	    xx = x*transform->r11 + y*transform->r21;
	    yy = x*transform->r12 + y*transform->r22;
	    break;
	case 16: 	/* r11=1, r12=0, r21=0, r22=1, dx=x, dy=0 */
	    xx = x + transform->dx;
	    yy = y;
	    break;
	case 17:	/* r11=x, r12=0, r21=0, r22=1, dx=x, dy=0 */
	    xx = x*transform->r11 + transform->dx;
	    yy = y;
	    break;
	case 18:	/* r11=1, r12=x, r21=0, r22=1, dx=x, dy=0 */
	    xx = x + transform->dx;
	    yy = x*transform->r12 + y;
	    break;
	case 19:	/* r11=x, r12=x, r21=0, r22=1, dx=x, dy=0 */
	    xx = x*transform->r11 + transform->dx;
	    yy = x*transform->r12 + y;
	    break;
	case 20: 	/* r11=1, r12=0, r21=x, r22=1, dx=x, dy=0 */
	    xx = x + y*transform->r21 + transform->dx;
	    yy = y;
	    break;
	case 21:	/* r11=x, r12=0, r21=x, r22=1, dx=x, dy=0 */
	    xx = x*transform->r11 + y*transform->r21 + transform->dx;
	    yy = y;
	    break;
	case 22:	/* r11=1, r12=x, r21=x, r22=1, dx=x, dy=0 */
	    xx = x + y*transform->r21 + transform->dx;
	    yy = x*transform->r12 + y;
	    break;
	case 23:	/* r11=x, r12=x, r21=x, r22=1, dx=x, dy=0 */
	    xx = x*transform->r11 + y*transform->r21 + transform->dx;
	    yy = x*transform->r12 + y;
	    break;
	case 24: 	/* r11=1, r12=0, r21=0, r22=x, dx=x, dy=0 */
	    xx = x + transform->dx;
	    yy = y*transform->r22;
	    break;
	case 25:	/* r11=x, r12=0, r21=0, r22=x, dx=x, dy=0 */
	    xx = x*transform->r11 + transform->dx;
	    yy = y*transform->r22;
	    break;
	case 26:	/* r11=1, r12=x, r21=0, r22=x, dx=x, dy=0 */
	    xx = x + transform->dx;
	    yy = x*transform->r12 + y*transform->r22;
	    break;
	case 27:	/* r11=x, r12=x, r21=0, r22=x, dx=x, dy=0 */
	    xx = x*transform->r11 + transform->dx;
	    yy = x*transform->r12 + y*transform->r22;
	    break;
	case 28: 	/* r11=1, r12=0, r21=x, r22=x, dx=x, dy=0 */
	    xx = x + y*transform->r21 + transform->dx;
	    yy = y*transform->r22;
	    break;
	case 29:	/* r11=x, r12=0, r21=x, r22=x, dx=x, dy=0 */
	    xx = x*transform->r11 + y*transform->r21 + transform->dx;
	    yy = y*transform->r22;
	    break;
	case 30:	/* r11=1, r12=x, r21=x, r22=x, dx=x, dy=0 */
	    xx = x + y*transform->r21 + transform->dx;
	    yy = x*transform->r12 + y*transform->r22;
	    break;
	case 31:	/* r11=x, r12=x, r21=x, r22=x, dx=x, dy=0 */
	    xx = x*transform->r11 + y*transform->r21 + transform->dx;
	    yy = x*transform->r12 + y*transform->r22;
	    break;
	case 32: 	/* r11=1, r12=0, r21=0, r22=1, dx=0, dy=x */
	    xx = x;
	    yy = y + transform->dy;
	    break;
	case 33:	/* r11=x, r12=0, r21=0, r22=1, dx=0, dy=x */
	    xx = x*transform->r11;
	    yy = y + transform->dy;
	    break;
	case 34:	/* r11=1, r12=x, r21=0, r22=1, dx=0, dy=x */
	    xx = x;
	    yy = x*transform->r12 + y + transform->dy;
	    break;
	case 35:	/* r11=x, r12=x, r21=0, r22=1, dx=0, dy=x */
	    xx = x*transform->r11;
	    yy = x*transform->r12 + y + transform->dy;
	    break;
	case 36: 	/* r11=1, r12=0, r21=x, r22=1, dx=0, dy=x */
	    xx = x + y*transform->r21;
	    yy = y + transform->dy;
	    break;
	case 37:	/* r11=x, r12=0, r21=x, r22=1, dx=0, dy=x */
	    xx = x*transform->r11 + y*transform->r21;
	    yy = y + transform->dy;
	    break;
	case 38:	/* r11=1, r12=x, r21=x, r22=1, dx=0, dy=x */
	    xx = x + y*transform->r21;
	    yy = x*transform->r12 + y + transform->dy;
	    break;
	case 39:	/* r11=x, r12=x, r21=x, r22=1, dx=0, dy=x */
	    xx = x*transform->r11 + y*transform->r21;
	    yy = x*transform->r12 + y + transform->dy;
	    break;
	case 40: 	/* r11=1, r12=0, r21=0, r22=x, dx=0, dy=x */
	    xx = x;
	    yy = y*transform->r22 + transform->dy;
	    break;
	case 41:	/* r11=x, r12=0, r21=0, r22=x, dx=0, dy=x */
	    xx = x*transform->r11;
	    yy = y*transform->r22 + transform->dy;
	    break;
	case 42:	/* r11=1, r12=x, r21=0, r22=x, dx=0, dy=x */
	    xx = x;
	    yy = x*transform->r12 + y*transform->r22 + transform->dy;
	    break;
	case 43:	/* r11=x, r12=x, r21=0, r22=x, dx=0, dy=x */
	    xx = x*transform->r11;
	    yy = x*transform->r12 + y*transform->r22 + transform->dy;
	    break;
	case 44: 	/* r11=1, r12=0, r21=x, r22=x, dx=0, dy=x */
	    xx = x + y*transform->r21;
	    yy = y*transform->r22 + transform->dy;
	    break;
	case 45:	/* r11=x, r12=0, r21=x, r22=x, dx=0, dy=x */
	    xx = x*transform->r11 + y*transform->r21;
	    yy = y*transform->r22 + transform->dy;
	    break;
	case 46:	/* r11=1, r12=x, r21=x, r22=x, dx=0, dy=x */
	    xx = x + y*transform->r21;
	    yy = x*transform->r12 + y*transform->r22 + transform->dy;
	    break;
	case 47:	/* r11=x, r12=x, r21=x, r22=x, dx=0, dy=x */
	    xx = x*transform->r11 + y*transform->r21;
	    yy = x*transform->r12 + y*transform->r22 + transform->dy;
	    break;
	case 48: 	/* r11=1, r12=0, r21=0, r22=1, dx=x, dy=x */
	    xx = x + transform->dx;
	    yy = y + transform->dy;
	    break;
	case 49:	/* r11=x, r12=0, r21=0, r22=1, dx=x, dy=x */
	    xx = x*transform->r11 + transform->dx;
	    yy = y + transform->dy;
	    break;
	case 50:	/* r11=1, r12=x, r21=0, r22=1, dx=x, dy=x */
	    xx = x + transform->dx;
	    yy = x*transform->r12 + y + transform->dy;
	    break;
	case 51:	/* r11=x, r12=x, r21=0, r22=1, dx=x, dy=x */
	    xx = x*transform->r11 + transform->dx;
	    yy = x*transform->r12 + y + transform->dy;
	    break;
	case 52: 	/* r11=1, r12=0, r21=x, r22=1, dx=x, dy=x */
	    xx = x + y*transform->r21 + transform->dx;
	    yy = y + transform->dy;
	    break;
	case 53:	/* r11=x, r12=0, r21=x, r22=1, dx=x, dy=x */
	    xx = x*transform->r11 + y*transform->r21 + transform->dx;
	    yy = y + transform->dy;
	    break;
	case 54:	/* r11=1, r12=x, r21=x, r22=1, dx=x, dy=x */
	    xx = x + y*transform->r21 + transform->dx;
	    yy = x*transform->r12 + y + transform->dy;
	    break;
	case 55:	/* r11=x, r12=x, r21=x, r22=1, dx=x, dy=x */
	    xx = x*transform->r11 + y*transform->r21 + transform->dx;
	    yy = x*transform->r12 + y + transform->dy;
	    break;
	case 56: 	/* r11=1, r12=0, r21=0, r22=x, dx=x, dy=x */
	    xx = x + transform->dx;
	    yy = y*transform->r22 + transform->dy;
	    break;
	case 57:	/* r11=x, r12=0, r21=0, r22=x, dx=x, dy=x */
	    xx = x*transform->r11 + transform->dx;
	    yy = y*transform->r22 + transform->dy;
	    break;
	case 58:	/* r11=1, r12=x, r21=0, r22=x, dx=x, dy=x */
	    xx = x + transform->dx;
	    yy = x*transform->r12 + y*transform->r22 + transform->dy;
	    break;
	case 59:	/* r11=x, r12=x, r21=0, r22=x, dx=x, dy=x */
	    xx = x*transform->r11 + transform->dx;
	    yy = x*transform->r12 + y*transform->r22 + transform->dy;
	    break;
	case 60: 	/* r11=1, r12=0, r21=x, r22=x, dx=x, dy=x */
	    xx = x + y*transform->r21 + transform->dx;
	    yy = y*transform->r22 + transform->dy;
	    break;
	case 61:	/* r11=x, r12=0, r21=x, r22=x, dx=x, dy=x */
	    xx = x*transform->r11 + y*transform->r21 + transform->dx;
	    yy = y*transform->r22 + transform->dy;
	    break;
	case 62:	/* r11=1, r12=x, r21=x, r22=x, dx=x, dy=x */
	    xx = x + y*transform->r21 + transform->dx;
	    yy = x*transform->r12 + y*transform->r22 + transform->dy;
	    break;
	case 63:	/* r11=x, r12=x, r21=x, r22=x, dx=x, dy=x */
	    xx = x*transform->r11 + y*transform->r21 + transform->dx;
	    yy = x*transform->r12 + y*transform->r22 + transform->dy;
	    break;
	default:
	    xx = x*transform->r11 + y*transform->r21 + transform->dx;
	    yy = x*transform->r12 + y*transform->r22 + transform->dy;
	    break;
    }

    printf("%4.6g %4.6g\n",xx,yy);
}
