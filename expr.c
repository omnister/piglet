/* 
* Simple recursive descent parser for numerical expressions
*
* int errcode = parse_exp( double *retval, char * expression);
*
* expr :        mulexp
*               | expr + mulexp
*               | expr - mulexp 
*
*  mulexp :     unary
*               | mulexp + unary
*               | mulexp / unary
*               | mulexp ^ unary
*
*  unary :      primary
*               | + unary
*               | - unary
*
*  primary :    NUMBER
*               ( expr )
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "expr.h"

double expr(void);
double mulexp(void);
double unary(void);
double primary(void);
double getval();
void eatblanks();

int main()
{                               // testing stub
    double val;
    for (;;) {
        printf("> ");
        val = expr();
        if (getnext() != '\n') {
            printf("error\n");
            eatblanks();
        } else {
            printf("    %g\n", val);
        }
    }
    exit(1);
}

double expr(void)
{
    double val;
    int c;
    val = mulexp();
    for (;;) {
        switch (c = getchar()) {
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
            ungetc(c, stdin);
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
        switch (c = getchar()) {
        case '*':
            val *= unary();
            break;
        case '/':
            val /= unary();
            break;
        default:
            ungetc(c, stdin);
            return (val);
            break;
        }
    }
}

double unary(void)
{
    double val;
    int c;
    switch (c = getchar()) {
    case '+':
        val = unary();
        break;
    case '-':
        val = -unary();
        break;
    default:
        ungetc(c, stdin);
        val = primary();
        break;
    }
    return (val);
}

double primary(void)
{
    double val;
    int c;
    c = getchar();
    if ((c >= '0' && c <= '9') || c == '.') {
        ungetc(c, stdin);
        val = getval();
        goto out;
    }
    if (c == '(') {
        val = expr();
        getchar();              /* eat closing ')' */
        goto out;
    }
    printf("error: primary read %d\n", c);
  out:
    return (val);
}

double getval()
{
    double val = 0.0;
    double power = 1.0;
    int c;
    int sawpoint = 0;

    c = getchar();
    for (val = 0; (c >= '0' && c <= '9') || c == '.'; c = getchar()) {
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

    if (c != EOF)
        ungetc(c, stdin);
    return (val / power);
}

void eatblanks(void)
{
    while (getchar() != '\n') {
        ;
    }
}
