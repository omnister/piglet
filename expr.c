#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "expr.h"

#define EXPR_TEST 0				// set to zero to include test main()
						// can use expr.test for vectors

/* 
* Simple recursive descent parser for numerical expressions
*
* int errcode = parse_exp( double *retval, char * expression);
*
* expr : 	mulexp
*		| expr + mulexp
*		| expr - mulexp	
*
*  mulexp :	unary
*  		| mulexp + unary
*		| mulexp / unary
*
*  unary :	primary
*  		| + unary
*		| - unary
*
*  primary :	NUMBER
*  		( expr )
*
*  tolerates leading/trailing blank spaces, returns 0 on success
*  non-zero on any errors.  It is an error to have any trailing
*  non-converted non-space characters in the expression.
*
*/

double expr(void);
double mulexp(void);
double unary(void);
double primary(void);
double getval();
int getnext(int eatblanks);
void ungetnext();

static int offset=0;
static int errflag=0;
static char *pc;

#if EXPR_TEST

static char buf[1024];
int main()
{                               // testing stub
    double val;
	int retval;
    for (;;) {
        // printf("> ");
	    if (fgets(buf, 1024, stdin) == NULL) {
			exit(1);
		}

		if (buf[strlen(buf)-1] == '\n') {
			buf[strlen(buf)-1] = '\0';
		}
	    retval=parse_exp(&val, buf);
		if (retval) {
			printf("%-20s:    ERROR\n",buf);
		} else {
			printf("%-20s:    %g\n",buf, val);
		}
    }
    exit(0);
}
#endif 

int getnext(int eatblanks) {
    int c;

	while ((((c = pc[offset++]) == ' ') && eatblanks ) && c!= '\0') {
		;
	}

    if (c !='\0') {
        return(c);
    } else {
        return(EOF);
    }
}

void ungetnext() {
    if (offset > 0) {
        offset--;
    } else {
		errflag++;
	}
}

int parse_exp(double *val, char * expression) {
        pc = expression;	// save calling string
	errflag=0;
	offset=0;

	*val = expr(); 

	// printf("called with %s, %g %d %d\n", expression, *val, errflag, strlen(pc)-offset);

	if (strlen(pc)-offset) errflag++;

	if (errflag) {
		*val = 0.0;
	}
	return(errflag);
}

double expr(void)
{
    double val;
    int c;
    val = mulexp();
    for (;;) {
        switch (c = getnext(1)) {
        case '+':
            val += mulexp();
            break;
        case '-':
            val -= mulexp();
            break;
        default:
            ungetnext();
            return (val);
            break;
        }
    }
}

double mulexp(void)
{
    double val;
    int c;
    val = unary();
    for (;;) {
        switch (c = getnext(1)) {
        case '*':
            val *= unary();
            break;
        case '/':
            val /= unary();
            break;
        default:
            ungetnext();
            return (val);
            break;
        }
    }
}

double unary(void)
{
    double val;
    int c;
    switch (c = getnext(1)) {
    case '+':
        val = unary();
        break;
    case '-':
        val = -unary();
        break;
    default:
        ungetnext();
        val = primary();
        break;
    }
    return (val);
}

double primary(void)
{
    double val=0.0;
    int c;
	int debug=0;

    c = getnext(1);
    if ((c >= '0' && c <= '9') || c == '.') {
        ungetnext();
        val = getval();
    } else if (c == '(') {
        val = expr();
        if ((c=getnext(1)) != ')') {		// eat closing ')'
			errflag++;
		}              
    } else {
		ungetnext();
    	if (debug) printf("error: primary read <%c> = %d\n", c, c);
		errflag++;
	}

    return (val);
}

// parses floating point number (no exponential notation) 
// with optional decimal point
// called with nextchar pointing at a digit or a decimal pt.
//

double getval()
{
    double val = 0.0;
    double power = 1.0;
    int c;
    int sawpoint = 0;

    c = getnext(0);
    for (val = 0; (c >= '0' && c <= '9') || c == '.'; c = getnext(0)) {
        if (sawpoint) {
            power *= 10.0;
        }
        if (c == '.') {
            power = 1.0;
            sawpoint++;
        } else {
            val = 10 * val + (double) (c - '0');
        }
    }

    if (c != '\0') { ungetnext(); }

    return (val / power);
}
