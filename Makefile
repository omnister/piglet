 
OBJS= db.o xwin.o readfont.o rubber.o opt_parse.o \
       eprintf.o geom_circle.o geom_rect.o geom_line.o \
       geom_poly.o geom_text.o geom_inst.o \
       rlgetc.o token.o lex.o coords.o com_window.o \
       com_point.o com_show.o

TARS= Makefile NOTEDATA.F token.c token.h lex.c xwin.c db.c db.h readfont.c readfont.h \
rlgetc.h eprintf.c rlgetc.c eprintf.h eventnames.h xwin.h  

CC=cc -ggdb

pig: $(OBJS)
	cc -ggdb $(OBJS) -o pig -lreadline -lX11 -L/usr/X11R6/lib -lm -lcurses


#-------------- dependencies ----------------------------------

coords.o:      db.h
db.o:          db.h token.h xwin.h
eprintf.o:     eprintf.h
com_window.o:  db.h xwin.h token.h rubber.h
com_point.o:   db.h xwin.h token.h
com_show.o:    db.h xwin.h token.h
geom_circle.c: db.h xwin.h token.h rubber.h
geom_inst.c:   db.h xwin.h token.h rubber.h opt_parse.h
geom_line.c:   db.h xwin.h token.h rubber.h
geom_text.c:   db.h xwin.h token.h rubber.h
geom_poly.c:   db.h xwin.h token.h rubber.h
geom_rect.c:   db.h xwin.h token.h rubber.h
lex.o:         rlgetc.h db.h token.h xwin.h lex.h
opt_parse.c:   db.h opt_parse.h
readfont.o:    db.h readfont.h
rlgetc.o:      db.h xwin.h eprintf.h
rubber.o:      rubber.h
token.o:       token.h xwin.h
token.o:       token.h
xwin.o:        db.h lex.h xwin.h eprintf.h rubber.h

clean: 
	rm -f *.o pig

pig.tar: $(TARS)
	tar cvf - $(TARS) > pig.tar

