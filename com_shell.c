#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rlgetc.h"
#include "ev.h"


typedef void (*sighandler_t)(int);

/* Execute a command with /bin/sh after setting stdin,stdout,stderr = /dev/tty */

int pig_system(char *s) {
   int status, pid, w, tty;
   sighandler_t istat, qstat;

   fflush(NULL);	/* clear out any i/o from parent */
   usleep(500);
   tty = open("/dev/tty", O_RDWR);
   if (tty == -1) {
      fprintf(stderr, "error: can't open /det/tty\n");
      return(-1);
   }
   if ((pid = fork()) == 0) {	/* in child */
      close(0); dup(tty);
      close(1); dup(tty);
      close(2); dup(tty);
      close(tty);
      EVupdate();
      execlp("sh", "sh", "-c", s, (char *) 0);
      exit(127);
   }

   close(tty); 			/* in parent */
   istat = signal(SIGINT, SIG_IGN);	/* save sighandlers */
   qstat = signal(SIGQUIT, SIG_IGN);
   while ((w = wait(&status)) != pid && w != -1) {
      ;
   }
   if (w == -1) {
      status = -1;
   }
   signal(SIGINT, istat);	/* restore sighandlers */
   signal(SIGQUIT, qstat);
   return(status);
}

int com_echo(LEXER *lp, char *arg)	/* echo variables */
{
    char *word;	
    TOKEN token;
    int debug=0;

    if (debug) printf("    com_echo\n");

    while((token=token_get(lp, &word)) != EOF && token != EOL) {
    	printf("%s ",word);
    }
    printf("\n");

    return (0);
}

int com_define(LEXER *lp, char *arg)	/* set macro definition */
{
    int debug = 0;
    char *word;
    char var[128];
    char val[128];
    char *str;
    TOKEN token;
    int i;

    if (debug) printf("    com_def\n");

    i=0;
    val[0]='\0';
    while((token=token_get(lp, &word)) != EOF &&  token != EOL && token != EOC) {
        if (i==0) {
	    if (debug) printf("setting var %s\n", word);
	    strncpy(var,word, 128);
	} else {
	    if (debug) printf("concatenating val %s\n", word);
            strncat(val,word, 128);
	}
	i++;
    }

    if (i==0) { 
	EVprint(4);
    } else if (i==1) { 
	if ((str=Macroget(var)) == NULL) {
	   str="";
	}
        printf("\"%s\"\n", str);
    } else if (i>1) {
        Macroset(var,val);
    }

    return (0);
}

int com_set(LEXER *lp, char *arg)	/* set environment variables */
{
    int debug = 0;
    char *word;
    char var[128];
    char val[128];
    char *str;
    TOKEN token;
    int mode=0;
    int n=0;
    int i;

    if (debug) printf("    com_set\n");

    while((token=token_get(lp, &word)) == OPT) {
	switch (toupper((unsigned char)word[1])) {
	    case 'E':
		mode |= 1;	/* limit to exported vars */
		break;
	    case 'P':
		mode |= 2;	/* limit to private vars */
		break;
	    case 'M':
		mode |= 4;	/* limit to macro definitions */
		break;
	    default:
		printf("SET: bad option: %s\n", word);
		return(1);
	}
	n++;
    }
    token_unget(lp, token, word);	


    i=0;
    while((token=token_get(lp, &word)) == IDENT || token == QUOTE) {
        if (i==0) strncpy(var,word, 128);
        if (i==1) strncpy(val,word, 128);
	i++;
    }

    if (debug) printf("got var=%s, val=%s\n", var, val);

    if ((token != EOF && token != EOL && token != EOC) || i>2) {
       printf("SET: bad number of arguments\n");
    } else if (i==0) { 
	if (n==0) {		/* default is to print all env variables */
	   mode = 3;
	}
	EVprint(mode);
    } else if (i==1) { 
	if ((str=EVget(var)) == NULL) {
	   str="";
	}
        printf("%s\n", str);
    } else if (i==2) {
        EVset(var,val);
	if (mode & 1) {
	    EVexport(var);
	}
    }
    return (0);
}

int com_shell(LEXER *lp, char *arg)		/* run a program from within the editor */
{
    int debug = 0;
    // char *shell;
    char cmd[1024];
    char *word;
    TOKEN token;

    if (debug) printf("    com_shell\n");
    // if ((shell=EVget("SHELL")) == NULL) {
    //    shell="/bin/sh";
    // }

    cmd[0] = '\0';
    while((token=token_get(lp, &word)) != EOF  && token != EOL) {
        strncat(cmd, word, 1024);
        strncat(cmd, " ", 1024);
    }

    // if (token == EOL) { token_unget(lp, token, word); }

    pig_system(cmd);

    return (0);
}
