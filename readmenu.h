#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <string.h>

typedef struct menuentry {
    char *text;
    int color;
    int xloc;
    int yloc;
    int width;
    Window pane;
} MENUENTRY;

int loadmenu();

