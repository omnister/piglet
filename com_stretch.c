#include "db.h"
#include "xwin.h"
#include "token.h"
#include "lex.h"
#include "rlgetc.h"

int com_stretch(lp, arg)
LEXER *lp;
char *arg;
{
    int debug=1;

    rl_saveprompt();
    rl_setprompt("STR> ");
    if (debug) printf("    com_stretch (not implemented)\n");

    rl_restoreprompt();

    return (0);
}
