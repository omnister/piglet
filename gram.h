typedef struct coord_pair { int x,y; } PAIR;
typedef struct coord_leaf { PAIR coord; struct coord_leaf *next;} COORD_LEAF, *COORD_LIST;
typedef struct rect { char *name; int x1,y1,x2,y2; struct rect *next; } RECT, *RECT_LIST;
char * strsave();
