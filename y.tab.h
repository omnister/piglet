#ifndef YYERRCODE
#define YYERRCODE 256
#endif

#define FILES 257
#define FILE_NAME 258
#define EDIT 259
#define SHOW 260
#define LOCK 261
#define GRID 262
#define LEVEL 263
#define WINDOW 264
#define ADD 265
#define SAVE 266
#define QUOTED 267
#define ALL 268
#define LINE 269
#define RECTANGLE 270
#define POLYGON 271
#define CIRCLE 272
#define NUMBER 273
#define EXIT 274
#define OPTION 275
#define UNKNOWN 276
#define TEXT 277
#define NOTE 278
typedef union {
	int num;
	PAIR pair;
	COORD_LIST pairs;
	char *name;
	} YYSTYPE;
extern YYSTYPE yylval;
