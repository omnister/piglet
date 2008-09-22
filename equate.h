/* equate bit masks */

extern int initialize_equates();

extern void equate_clear_overrides(void);
extern void equate_toggle_override(int layer);
extern void equate_set_linetype(int layer, int type);
extern void equate_set_masktype(int layer, int mask);
extern void equate_set_fill(int layer, int fill);
extern void equate_set_color(int layer, int color);
extern void equate_set_pen(int layer, int pen);
extern void equate_set_label(int layer, char *label);

extern int  equate_get_override(int layer);
extern int  equate_get_linetype(int layer);
extern int  equate_get_masktype(int layer);
extern int  equate_get_fill(int layer);
extern int  equate_get_color(int layer);
extern int  equate_get_pen(int layer);
extern char *equate_get_label(int layer);

extern int  equate_print();

extern int  masktype2equate(char c);
extern int  linetype2equate(char c);
extern int  color2equate(char c);
