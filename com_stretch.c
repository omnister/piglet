#include "db.h"
#include "xwin.h"
#include "token.h"
#include "lex.h"

com_stretch(lp, arg)
LEXER *lp;
char *arg;
{
    int debug=0;

    rl_saveprompt();
    if (debug) printf("    com_stretch\n", arg);
    rl_restoreprompt();

    return (0);
}
