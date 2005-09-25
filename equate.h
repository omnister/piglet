/* equate bit masks */

#define DSI          3	/* 0=Detail, 1=Symbolic, 2=Interconnect */	
#define MSDB        12	/* Mask: 0=Solid, 1=Dotted, 2=Broken */
#define OUTLINE     16	/* 0=unfilled, 1=filled */
#define COLOR	   224	/* Color, 3-bits, 0=Red, 1=G, ... BYPAW */
#define PEN	  1792  /* Pen, 3-bits (0=unused) */
#define USED      2048  /* Set 1 if defined, if not defined, will not be plotted */

extern int initialize_equates();

extern void equate_set_type(int layer, int type);
extern void equate_set_mask(int layer, int mask);
extern void equate_set_fill(int layer, int fill);
extern void equate_set_color(int layer, int color);
extern void equate_set_pen(int layer, int pen);
extern void equate_set_label(int layer, char *label);

extern int  equate_get_type(int layer);
extern int  equate_get_mask(int layer);
extern int  equate_get_fill(int layer);
extern int  equate_get_color(int layer);
extern int  equate_get_pen(int layer);
extern char *equate_get_label(int layer);

extern int  equate_print();

extern int  mask2equate(char c);
extern int  type2equate(char c);
extern int  color2equate(char c);
