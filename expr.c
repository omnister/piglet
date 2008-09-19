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
*		| mulexp ^ unary
*
*  unary :	primary
*  		| + unary
*		| - unary
*
*  primary :	NUMBER
*  		( expr )
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

double expr(void);
double mulexp(void);
double unary(void);
double primary(void);
double getval();
int parse_exp();
int getnext();
void ungetnext();
void eatblanks();

char buf[1024];
int offset=0;
int errflag=0;
char *pc;
int debug=0;


int main()
{                               // testing stub
    double val;
	int retval;
    for (;;) {
        printf("> ");
	    fgets(buf, 1024, stdin);
		if (buf[strlen(buf)-1] == '\n') {
			buf[strlen(buf)-1] = '\0';
		}
	    retval=parse_exp(&val, buf);
		printf("    %g	(%d) %d\n", val, retval, strlen(buf)-offset);
    }
    exit(1);
}

int getnext() {
   int c;
   if ((c = pc[offset++])!='\0') {
       return(c);
   } else {
       offset--;
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
	errflag=0;
    pc = expression;	// save calling string
	offset=0;
	*val = expr();
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
        switch (c = getnext()) {
        case '+':
            val += mulexp();
            break;
        case '-':
            val -= mulexp();
            break;
        case '^':
            val = pow(val, mulexp());
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
        switch (c = getnext()) {
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
    switch (c = getnext()) {
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
    double val;
    int c;
    c = getnext();
    if ((c >= '0' && c <= '9') || c == '.') {
        ungetnext();
        val = getval();
        goto out;
    }
    if (c == '(') {
        val = expr();
        getnext();              /* eat closing ')' */
        goto out;
    }
	errflag++;
    if (debug) printf("error: primary read <%c> = %d\n", c, c);
  out:
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

    c = getnext();
    for (val = 0; (c >= '0' && c <= '9') || c == '.'; c = getnext()) {
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

    if (c != '\0')
        ungetnext();
    return (val / power);
}

void eatblanks(void)
{
    while (getnext() != '\n') {
        ;
    }
}
