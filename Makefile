 
OBJS= db.o draw.o xwin.o readfont.o rubber.o opt_parse.o \
       eprintf.o geom_circle.o geom_rect.o geom_line.o \
       geom_poly.o geom_text.o geom_inst.o \
       rlgetc.o token.o lex.o coords.o \
       com_add.o com_delete.o com_distance.o com_ident.o \
       com_copy.c com_move.c com_window.o com_point.o com_show.o postscript.o \
       readmenu.o

TARS = AAA_README cells/tone_I cells/slic_I cells/GLINKV3_I cells/H20919M1_I \
changes com_add.c com_delete.c com_distance.c com_ident.c com_copy.c com_move.c \
com_point.c com_show.c com_window.c coords.c db.c db.h draw.c \
eprintf.c eprintf.h eventnames.h geom_circle.c geom_inst.c \
geom_line.c  geom_poly.c geom_rect.c geom_text.c lex.c lex.h \
Makefile MENUDATA_V NOTEDATA.F opt_parse.c opt_parse.h \
plan.EQUATE plan.MOVE plan.SELECT plan.STATUS plan.TODO readfont.c \
readfont.h rlgetc.c rlgetc.h rubber.c rubber.h TEXTDATA.F token.c \
token.h xwin.c xwin.h postscript.o readmenu.c web RCS

CC=cc -ggdb

# we currently make pig.bin, and then run the binary under the
# wrapper "pig" which catches and error backtrace with gdb

pig.bin: $(OBJS)
	cc -ggdb $(OBJS) -o pig.bin -lreadline -lX11 -L/usr/X11R6/lib -lm -lcurses


#-------------- dependencies ----------------------------------

coords.o:      db.h
db.o:          db.h token.h xwin.h
draw.o:        db.h token.h xwin.h
eprintf.o:     eprintf.h
com_ident.o:   db.h xwin.h token.h
com_add.o:     db.h token.h
com_copy.o:    db.h token.h
com_delete.o:  db.h token.h
com_move.o:    db.h token.h
com_window.o:  db.h xwin.h token.h rubber.h
com_point.o:   db.h xwin.h token.h
com_show.o:    db.h xwin.h token.h
geom_circle.o: db.h xwin.h token.h rubber.h
geom_inst.o:   db.h xwin.h token.h rubber.h opt_parse.h
geom_line.o:   db.h xwin.h token.h rubber.h
geom_text.o:   db.h xwin.h token.h rubber.h
geom_poly.o:   db.h xwin.h token.h rubber.h
geom_rect.o:   db.h xwin.h token.h rubber.h
lex.o:         rlgetc.h db.h token.h xwin.h lex.h
opt_parse.o:   db.h opt_parse.h
postscript.o:
readfont.o:    db.h readfont.h
readmenu.o:    db.h readmenu.h
rlgetc.o:      db.h xwin.h eprintf.h
rubber.o:      rubber.h
token.o:       token.h xwin.h
token.o:       token.h
xwin.o:        db.h lex.h xwin.h eprintf.h rubber.h readmenu.h

clean: 
	rm -f *.o pig

pig.tar: $(TARS)
	tar cvf - $(TARS) > pig.tar

