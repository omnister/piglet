
MANDIR="/usr/local/man/man1p";
 
OBJS= db.o draw.o equate.o xwin.o readfont.o rubber.o opt_parse.o \
       eprintf.o geom_circle.o geom_rect.o geom_line.o \
       geom_poly.o geom_text.o geom_inst.o geom_arc.o\
       rlgetc.o token.o lex.o coords.o com_change.o\
       com_add.o com_delete.o com_distance.o com_equate.o com_ident.o \
       com_copy.o com_move.c com_window.o com_point.o com_show.o postscript.o \
       com_area.o readmenu.o com_edit.o com_stretch.o stack.o

TARS = AAA_README cells/tone_I cells/slic_I cells/GLINKV3_I \
cells/H20919M1_I cells/PLAN_I cells/ALL_I \
changes com_add.c com_delete.c com_distance.c com_equate.c com_ident.c \
com_copy.c com_move.c  com_point.c com_show.c com_window.c coords.c db.c db.h \
draw.c eprintf.c eprintf.h eventnames.h equate.c equate.h geom_circle.c \
geom_inst.c geom_line.c geom_arc.c  geom_poly.c geom_rect.c geom_text.c \
lex.c lex.h Makefile MENUDATA_V NOTEDATA.F opt_parse.c opt_parse.h com_change.c \
plan.EQUATE plan.MOVE plan.SELECT plan.STATUS plan.TODO readfont.c \
readfont.h rlgetc.c rlgetc.h rubber.c rubber.h TEXTDATA.F token.c \
token.h xwin.c xwin.h postscript.c pig postscript.h readmenu.h \
readmenu.c man/commandlist man/makemans man/seeallso com_area.c \
com_edit.c com_stretch.c stack.h stack.c

CC=cc -ggdb -Wall

# we currently make pig.bin, and then run the binary under the
# wrapper "pig" which catches and error backtrace with gdb

pig.bin: $(OBJS) man/piglet.1p
	$(CC) $(OBJS) -o pig.bin -lreadline -lX11 -L/usr/X11R6/lib -lm -lcurses

clean: 
	rm -f *.o pig.bin
	rm -r man/*1p man/*html

pig.tar: $(TARS)
	(q=`pwd`;p=`basename $$q`;cd ..;tar czvf - `for file in $(TARS);do echo $$p/$$file;done`>$$p/pig.tar.gz)

junk:
	(mkdir dist/cells; mkdir dist/man; for file in $(TARS); do cp -a $$file dist/$$file; done)

man/piglet.1p: man/commandlist
	(cd man; makemans)


install: man/piglet.1p
	echo "installing man pages in $(MANDIR)"
	echo "you man wish to add $(MANDIR) to your MANPATH"
	mkdir -p $(MANDIR)
	cp man/*1p $(MANDIR)

#------- dependencies (made with dodeps script) --------------

com_add.o:     rlgetc.h db.h token.h xwin.h lex.h 
com_area.o:    rlgetc.h db.h token.h xwin.h lex.h rubber.h 
com_change.o:  rlgetc.h db.h token.h xwin.h lex.h eprintf.h opt_parse.h 
com_copy.o:    rlgetc.h db.h token.h xwin.h lex.h rubber.h 
com_delete.o:  rlgetc.h db.h token.h xwin.h lex.h 
com_distance.o:db.h xwin.h token.h lex.h rubber.h rlgetc.h 
com_edit.o:    rlgetc.h db.h token.h xwin.h lex.h 
com_equate.o:  db.h xwin.h token.h lex.h rlgetc.h equate.h
com_ident.o:   db.h xwin.h token.h lex.h rlgetc.h 
com_move.o:    rlgetc.h db.h token.h xwin.h lex.h rubber.h 
com_point.o:   db.h xwin.h token.h lex.h rlgetc.h 
com_show.o:    db.h xwin.h token.h lex.h rlgetc.h 
com_stretch.o: db.h xwin.h token.h lex.h rlgetc.h 
com_window.o:  db.h xwin.h token.h rubber.h lex.h rlgetc.h eprintf.h 
coords.o:      db.h 
db.o:          db.h eprintf.h rlgetc.h token.h xwin.h readfont.h rubber.h 
draw.o:        db.h rubber.h xwin.h postscript.h eprintf.h 
eprintf.o:     eprintf.h 
equate.o:      equate.h
geom_arc.o:    db.h token.h lex.h xwin.h rubber.h rlgetc.h opt_parse.h 
geom_circle.o: db.h xwin.h token.h rubber.h lex.h rlgetc.h opt_parse.h 
geom_inst.o:   db.h xwin.h token.h rubber.h opt_parse.h rlgetc.h lex.h 
geom_line.o:   db.h token.h lex.h xwin.h rubber.h rlgetc.h opt_parse.h 
geom_poly.o:   db.h token.h lex.h xwin.h rubber.h rlgetc.h opt_parse.h 
geom_rect.o:   db.h xwin.h token.h rubber.h lex.h rlgetc.h opt_parse.h 
geom_text.o:   db.h xwin.h token.h rubber.h opt_parse.h rlgetc.h 
lex.o:         rlgetc.h db.h token.h xwin.h lex.h eprintf.h readfont.h equate.h
opt_parse.o:   db.h opt_parse.h eprintf.h 
postscript.o:  postscript.h 
readfont.o:    readfont.h db.h 
readmenu.o:    readmenu.h db.h 
rlgetc.o:      eprintf.h db.h xwin.h 
rubber.o:      rubber.h 
token.o:       token.h eprintf.h db.h xwin.h rlgetc.h lex.h 
xwin.o:        eventnames.h db.h xwin.h token.h eprintf.h rubber.h lex.h readmenu.h 
