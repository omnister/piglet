typedef struct Symbol {     /* symbol table entry */
    char    *name;
    short   type;
    union {
        double  val;        /* VAR */
        double  (*ptr)();   /* BLTIN */
        int     (*defn)();  /* FUNCTION, PROCEDURE */
        char    *str;       /* STRING */
    } u;
    struct Symbol   *next;  /* to link to another */
} Symbol;

Symbol  *install(), *lookup();
int printdefs();

extern  eval(), add(), sub(), mul(), div(), negate(), power(), modulo();

typedef int (*Inst)();

extern  assign(), bltin(), varpush(), constpush(), print(), varread();
extern  prexpr(), prstr();
extern  gt(), lt(), eq(), ge(), le(), ne(), and(), or(), not();
extern  ifcode(), whilecode(), call(), arg(), argassign();
extern  funcret(), procret();

