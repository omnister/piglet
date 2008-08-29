
/* modes for draw() function */

#define D_NORM   0  	/* normal drawing */
#define D_RUBBER 1	/* xor rubber band drawing */
#define D_BB     2	/* free drawing dont update bb */
#define D_PICK   3	/* screen points for pick checking */
#define D_READIN 4	/* do bounding box but no displaying */

extern void rubber_set_callback();	/* register callback function */
extern void rubber_clear_callback();	/* withdraw callback function */
extern void rubber_draw();		/* execute callback function */
