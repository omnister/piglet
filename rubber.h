
/* modes for draw() function */

#define D_NORM   0  	/* normal drawing */
#define D_RUBBER 1	/* xor rubber band drawing */
#define D_BB     2	/* free drawing dont update bb */
#define D_PICK   3	/* screen points for pick checking */

extern void rubber_set_callback();	/* register callback function */
extern void rubber_clear_callback();	/* withdraw callback function */
extern void rubber_draw();		/* execute callback function */
