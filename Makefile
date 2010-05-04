MANDIR=/usr/local/man/man1p
BINDIR=/usr/local/bin
LIBDIR=/usr/local/lib/piglet

# for Fedora core 3, x-64:
#XLIB=-lX11 -L/usr/X11R6/lib64
# for x86
XLIB=-lX11 -L/usr/X11R6/lib 

 
OBJS=db.o draw.o equate.o xwin.o readfont.o rubber.o opt_parse.o \
   eprintf.o geom_circle.o geom_rect.o geom_line.o geom_poly.o \
   geom_text.o geom_inst.o geom_arc.o rlgetc.o token.o lex.o \
   coords.o com_change.o com_add.o com_delete.o com_distance.o \
   com_equate.o com_ident.o com_copy.o com_move.o com_window.o \
   com_point.o com_show.o com_area.o com_wrap.o com_smash.o \
   readmenu.o com_edit.o com_stretch.o com_undo.o postscript.o \
   lock.o path.o stack.o selpnt.o license.o stipple.o com_purge.o \
   ev.o com_shell.o expr.o
   

TARS=QUICKSTART AAA_README COPYING \
changes com_add.c com_delete.c com_distance.c com_equate.c com_ident.c \
com_copy.c com_move.c  com_point.c com_show.c com_window.c com_purge.c \
coords.c db.c db.h com_wrap.c com_smash.c draw.c eprintf.c \
eprintf.h eventnames.h equate.c equate.h geom_circle.c geom_inst.c \
geom_line.c geom_arc.c  geom_poly.c geom_rect.c geom_text.c \
lex.c Makefile PROCDATA.P MENUDATA_V NOTEDATA.F \
opt_parse.c opt_parse.h com_change.c readfont.c readfont.h rlgetc.c \
rlgetc.h rubber.c rubber.h TEXTDATA.F token.c token.h xwin.c xwin.h \
postscript.c pig postscript.h readmenu.h readmenu.c \
com_area.c com_edit.c com_stretch.c stack.h com_undo.c \
selpnt.c path.h path.c lock.c lock.h stack.c piglogo.d license.c stipple.c \
ev.c ev.h com_shell.c expr.c expr.h pigrc .git

CELLS=cells/tone_I cells/slic_I cells/GLINKV3_I cells/H20919M1_I \
cells/PLAN_I cells/ALL_I cells/smorgasboard_I cells/schem2_I

MANS=man/commandlist man/makemans man/seealso 

# use "-O0" for valgrind memory usage checking
# use "-ggdb" for gnu debugger

#CC=cc -O0 -ggdb -Wall
CC=cc -ggdb -Wall

# we currently make pig.bin, and then run the binary under the
# wrapper "pig" which catches and error backtrace with gdb

pig.bin: $(OBJS) man/piglet.1p
	$(CC) $(OBJS) -o pig.bin -lreadline $(XLIB) -lm -lcurses

clean: 
	rm -f *.o pig.bin
	rm -rf man/*1p man/*html

tar: $(TARS)
	(  d=`date +%F`;\
	   q=`pwd`;\
	   p=`basename $$q`;\
	   rm -rf $$p$$d;\
	   mkdir $$p$$d;\
	   cp -rp $(TARS) $$p$$d;\
	   mkdir $$p$$d/cells;\
	   cp -rp $(CELLS) $$p$$d/cells;\
	   mkdir $$p$$d/man;\
	   cp -rp $(MANS) $$p$$d/man;\
	   tar czvf - $$p$$d >$$p$$d.tar.gz;\
	)


man/piglet.1p: man/commandlist
	(cd man; ./makemans)


install: man/piglet.1p pig.bin
	@echo "########################################################"
	@echo "installing man pages in $(MANDIR)"
	@echo "you may wish to add $(MANDIR) to your MANPATH"
	@echo "########################################################"
	mkdir -p $(MANDIR)
	cp man/*1p $(MANDIR)
	-mv $(BINDIR)/pig.bin $(BINDIR)/pig.bin.bak
	cp pig.bin  $(BINDIR)
	cp pig  $(BINDIR)
	mkdir -p $(LIBDIR)
	cp NOTEDATA.F $(LIBDIR)
	cp TEXTDATA.F $(LIBDIR)
	cp PROCDATA.P $(LIBDIR)
	cp MENUDATA_V $(LIBDIR)
	cp piglogo.d $(LIBDIR)
	cp pigrc $(LIBDIR)

depend: ${OBJ}
	cp Makefile Makefile.bak
	sed -n -e '1,/^# DO NOT DELETE OR MODIFY THIS LINE/p' Makefile \
                > newmakefile
	grep '^#include[ 	]*"' *.c \
                | sed -e 's/:#include[  "]*\([a-z0-9\._A-Z]*\).*/: \1/' \
                | sed -e 's/\.c:/.o:/' \
                | sort | uniq >> newmakefile
	mv Makefile Makefile.bak
	mv newmakefile Makefile

#-----------------------------------------------------------------
# DO NOT PUT ANY DEPENDENCIES AFTER THE NEXT LINE -- they will go away
# DO NOT DELETE OR MODIFY THIS LINE -- make depend uses it
com_add.o: db.h
com_add.o: rlgetc.h
com_add.o: token.h
com_add.o: xwin.h
com_area.o: db.h
com_area.o: rlgetc.h
com_area.o: rubber.h
com_area.o: token.h
com_area.o: xwin.h
com_change.o: db.h
com_change.o: eprintf.h
com_change.o: opt_parse.h
com_change.o: rlgetc.h
com_change.o: token.h
com_change.o: xwin.h
com_copy.o: db.h
com_copy.o: rlgetc.h
com_copy.o: rubber.h
com_copy.o: token.h
com_copy.o: xwin.h
com_delete.o: db.h
com_delete.o: rlgetc.h
com_delete.o: rubber.h
com_delete.o: token.h
com_delete.o: xwin.h
com_distance.o: db.h
com_distance.o: rlgetc.h
com_distance.o: rubber.h
com_distance.o: token.h
com_distance.o: xwin.h
com_edit.o: db.h
com_edit.o: ev.h
com_edit.o: rlgetc.h
com_edit.o: token.h
com_edit.o: xwin.h
com_equate.o: db.h
com_equate.o: equate.h
com_equate.o: rlgetc.h
com_equate.o: token.h
com_equate.o: xwin.h
com_ident.o: db.h
com_ident.o: rlgetc.h
com_ident.o: rubber.h
com_ident.o: token.h
com_ident.o: xwin.h
com_move.o: db.h
com_move.o: rlgetc.h
com_move.o: rubber.h
com_move.o: token.h
com_move.o: xwin.h
com_point.o: db.h
com_point.o: rlgetc.h
com_point.o: token.h
com_point.o: xwin.h
com_purge.o: db.h
com_purge.o: rlgetc.h
com_purge.o: rubber.h
com_purge.o: token.h
com_purge.o: xwin.h
com_shell.o: db.h
com_shell.o: ev.h
com_shell.o: rlgetc.h
com_shell.o: token.h
com_shell.o: xwin.h
com_show.o: db.h
com_show.o: rlgetc.h
com_show.o: token.h
com_show.o: xwin.h
com_smash.o: db.h
com_smash.o: rlgetc.h
com_smash.o: rubber.h
com_smash.o: token.h
com_smash.o: xwin.h
com_stretch.o: db.h
com_stretch.o: lock.h
com_stretch.o: rlgetc.h
com_stretch.o: rubber.h
com_stretch.o: token.h
com_stretch.o: xwin.h
com_undo.o: db.h
com_undo.o: rlgetc.h
com_undo.o: token.h
com_undo.o: xwin.h
com_window.o: db.h
com_window.o: eprintf.h
com_window.o: equate.h
com_window.o: rlgetc.h
com_window.o: rubber.h
com_window.o: token.h
com_window.o: xwin.h
com_wrap.o: db.h
com_wrap.o: rlgetc.h
com_wrap.o: rubber.h
com_wrap.o: token.h
com_wrap.o: xwin.h
coords.o: db.h
db.o: db.h
db.o: eprintf.h
db.o: ev.h
db.o: readfont.h
db.o: rlgetc.h
db.o: rubber.h
db.o: token.h
db.o: xwin.h
draw.o: db.h
draw.o: eprintf.h
draw.o: equate.h
draw.o: postscript.h
draw.o: rubber.h
draw.o: xwin.h
eprintf.o: eprintf.h
equate.o: db.h
equate.o: equate.h
ev.o: ev.h
expr.o: expr.h
geom_arc.o: db.h
geom_arc.o: opt_parse.h
geom_arc.o: rlgetc.h
geom_arc.o: rubber.h
geom_arc.o: token.h
geom_arc.o: xwin.h
geom_circle.o: db.h
geom_circle.o: opt_parse.h
geom_circle.o: rlgetc.h
geom_circle.o: rubber.h
geom_circle.o: token.h
geom_circle.o: xwin.h
geom_inst.o: db.h
geom_inst.o: opt_parse.h
geom_inst.o: rlgetc.h
geom_inst.o: rubber.h
geom_inst.o: token.h
geom_inst.o: xwin.h
geom_line.o: db.h
geom_line.o: lock.h
geom_line.o: opt_parse.h
geom_line.o: rlgetc.h
geom_line.o: rubber.h
geom_line.o: token.h
geom_line.o: xwin.h
geom_poly.o: db.h
geom_poly.o: opt_parse.h
geom_poly.o: rlgetc.h
geom_poly.o: rubber.h
geom_poly.o: token.h
geom_poly.o: xwin.h
geom_rect.o: db.h
geom_rect.o: opt_parse.h
geom_rect.o: rlgetc.h
geom_rect.o: rubber.h
geom_rect.o: token.h
geom_rect.o: xwin.h
geom_text.o: db.h
geom_text.o: opt_parse.h
geom_text.o: rlgetc.h
geom_text.o: rubber.h
geom_text.o: token.h
geom_text.o: xwin.h
lex.o: db.h
lex.o: eprintf.h
lex.o: equate.h
lex.o: ev.h
lex.o: path.h
lex.o: readfont.h
lex.o: rlgetc.h
lex.o: rubber.h
lex.o: token.h
lex.o: xwin.h
lock.o: db.h
lock.o: lock.h
lock.o: xwin.h
opt_parse.o: db.h
opt_parse.o: eprintf.h
opt_parse.o: opt_parse.h
path.o: path.h
postscript.o: postscript.h
readfont.o: db.h
readfont.o: readfont.h
readmenu.o: db.h
readmenu.o: readmenu.h
rlgetc.o: db.h
rlgetc.o: eprintf.h
rlgetc.o: token.h
rlgetc.o: xwin.h
rubber.o: rubber.h
selpnt.o: db.h
stack.o: stack.h
stipple.o: db.h
stipple.o: xwin.h
token2.o: db.h
token2.o: eprintf.h
token2.o: ev.h
token2.o: expr.h
token2.o: rlgetc.h
token2.o: token.h
token2.o: xwin.h
token.o: db.h
token.o: eprintf.h
token.o: ev.h
token.o: expr.h
token.o: rlgetc.h
token.o: token.h
token.o: xwin.h
xwin.o: db.h
xwin.o: eprintf.h
xwin.o: eventnames.h
xwin.o: ev.h
xwin.o: path.h
xwin.o: readmenu.h
xwin.o: rubber.h
xwin.o: token.h
xwin.o: xwin.h
