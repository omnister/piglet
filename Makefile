
SRC = eprintf.c
OBJ = eprintf.o
TARS = Makefile NOTEDATA.F token.c token.h lex.c xwin.c db.c db.h readfont.c readfont.h \
rlgetc.h eprintf.c rlgetc.c eprintf.h eventnames.h xwin.h  y.tab.h
CC=cc -ggdb

pig: geom_rect.o geom_line.o rlgetc.o token.o lex.o eprintf.o db.o xwin.o \
readfont.o rubber.o
	cc -ggdb geom_rect.o geom_line.o rlgetc.o eprintf.o xwin.o db.o \
	readfont.o token.o lex.o rubber.o  \
	-o pig -lreadline -lX11 -L/usr/X11R6/lib -lm -lcurses

db.o: db.h  y.tab.h

readfont.o: readfont.h

token.o: token.h

eprintf.o: eprintf.h

clean: 
	rm -f *.o pig

pig.tar: $(TARS)
	tar cvf - $(TARS) > pig.tar
