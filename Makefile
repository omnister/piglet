
SRC = scan.l gram.y 
OBJ = scan.o gram.o

pig: gram.o scan.o db.o readfont.o 
	cc -pg gram.o scan.o db.o readfont.o -o pig -lfl -lm

lexer: scan.o
	lex -il scan.l
	cc -pg -DDEBUG lex.yy.c -o lexer -lfl
	rm lex.yy.c

clean: 
	rm -f y.tab.c y.output *.o lex.yy.c pig lexer

y.tab.h: gram.o

scan.o: scan.l y.tab.h 
	lex -il scan.l
	#cc -DYYDEBUG -c lex.yy.c 
	cc -pg -c lex.yy.c 
	rm lex.yy.c
	mv lex.yy.o scan.o

gram.o: gram.y
	yacc  -dv gram.y
	#cc -DYYDEBUG -c y.tab.c
	cc -pg -c y.tab.c
	rm y.tab.c
	mv y.tab.o gram.o
