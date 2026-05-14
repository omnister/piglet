#pragma once

#include "db.h"

void db_set_fill(int fill);
void db_set_physical(int physical);
void db_set_nest(int nest);

DB_DEFLIST *db_ident(
    DB_TAB *cell,               /* device being edited */
    double x, double y,         /* pick point */
    int mode,                   /* 0=ident (any visible), 1=pick (pick restricted to modifiable objects) */
    int pick_layer,             /* layer restriction, or 0=all */
    int comp,                   /* comp restriction */
    char *name                  /* instance name restrict or NULL */
);

SELPNT *db_ident2(
    DB_TAB *cell,               /* device being edited */
    double x, double y,         /* pick point */
    int mode,                   /* 0=ident (any visible), 1=pick (only modifiable objects) */
    int pick_layer,             /* layer restriction, or 0=all */
    int comp,                   /* comp restriction */
    char *name                  /* instance name restrict or NULL */
);

void db_notate(DB_DEFLIST *p);
void db_highlight(DB_DEFLIST *p);
int db_list(DB_TAB *cell);
int db_plot(char *name, OMODE plottype, double dx, double dy, int color, double pen);
void startpoly(BOUNDS *bb, int mode);
void endpoly(BOUNDS *bb, int mode);
void savepoly(int x, int y);

void clipl( int init, double x, double y, BOUNDS *bb, int mode);
void clipr( int init, double x, double y, BOUNDS *bb, int mode);
void clipt( int init, double x, double y, BOUNDS *bb, int mode);
void clipb( int init, double x, double y, BOUNDS *bb, int mode);

void draw(
    NUM x, NUM y,          /* location in real coordinate space */
    BOUNDS *bb,            /* bounding box */
    int mode               /* D_NORM=regular, D_RUBBER=rubberband, */
);

void set_layer(
    int lnum,   /* layer number */
    int comp    /* component type */
);

double max(double a, double b);
double min(double a, double b);
void jump(BOUNDS *bb, int mode);


                 





