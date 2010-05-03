#include <stdio.h>
#include <string.h>		/* for strchr() */
#include <ctype.h>		/* for toupper */

#include "rlgetc.h"
#include "db.h"
#include "token.h"
#include "xwin.h" 	

//
// This is piglet's undo/redo facility.
//
// All cells are kept in a symbol table of linked DB_TAB 
// structures (see db.h) Inside the structure is a pointer to 
// the current cell definition DEFLIST chain, and a pointer to
// an UNDO and a REDO stack.  Every time we go through the 
// command evaluation parse loop, we do a checksum on the current
// definition chain and the definition chain at the top of the
// UNDO stack (db_checkpoint()).  If they differ, there has been some 
// change made, so we make a copy of the current definition list and push it on
// the UNDO stack.  
//
// When the user requests an UNDO, we pop the deflist undo stack,
// push it on the REDO stack, free the definition at currep, pop the
// undo stack again and set the currep edit pointer to point to it.
// We then make a copy of the definition list at currep and push it
// on the UNDO stack.
//
// The following table shows the state of the stacks throughout
// and edit/undo/redo cycle:
//
//           UNDO  CURREP  REDO
//           stack        stack
// READIN:     A     A      -
// CHANGE1:    AB    B      -
// CHANGE2:    ABC   C      -
// UNDO:       AB    B      C
// UNDO:       A     A      CB
// REDO:       AB    B      C
// CHANGE:     ABD   D      -
//


int com_undo(LEXER *lp, char *arg)		
{
    DB_DEFLIST *a;

    /* check that we are editing a rep */
    if (currep == NULL) {
	printf("UNDO: must do \"EDIT <name>\" before UNDO\n");
	token_flush_EOL(lp);
	return(1);
    }

    if (currep->undo == NULL || stack_depth(&(currep->undo)) <= 1) {
	printf("UNDO: nothing left to UNDO\n");
	token_flush_EOL(lp);
	return(1);
    } 

    a = (DB_DEFLIST *) stack_pop(&(currep->undo));
    stack_push(&(currep->redo), (char *) a);
    db_free(currep->dbhead);
    a = (DB_DEFLIST *) stack_pop(&(currep->undo));
    currep->dbhead = a;
    a = db_copy_deflist(currep->dbhead);
    stack_push(&(currep->undo), (char *) a);

    printf("(undo: %d, redo: %d) ", stack_depth(&(currep->undo))-1, stack_depth(&(currep->redo)));

    need_redraw++;
    return(0);
}

int com_redo(LEXER *lp, char *arg)		
{
    DB_DEFLIST *a;

    /* check that we are editing a rep */
    if (currep == NULL) {
	printf("REDO: must do \"EDIT <name>\" before REDO\n");
	token_flush_EOL(lp);
	return(1);
    }

    if (currep->redo == NULL) {
	printf("REDO: nothing left to REDO\n");
	token_flush_EOL(lp);
	return(1);
    } 

    db_free(currep->dbhead);
    a = (DB_DEFLIST *) stack_pop(&(currep->redo));
    currep->dbhead = a;
    a = db_copy_deflist(currep->dbhead);
    stack_push(&(currep->undo), (char *) a);

    printf("(undo: %d, redo: %d) ", stack_depth(&(currep->undo))-1, stack_depth(&(currep->redo)));

    need_redraw++;
    return(0);
}
