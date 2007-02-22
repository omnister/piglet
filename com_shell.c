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
#include "lex.h"
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

int com_set(lp, arg)		/* set environment variables */
LEXER *lp;
char *arg;
{
    int debug = 0;
    char word[128];
    char var[128];
    char val[128];
    char *str;
    TOKEN token;
    int i;

    if (debug) printf("    com_set\n");

    i=0;
    while((token=token_get(lp, word)) == IDENT || token == QUOTE) {
        if (i==0) strncpy(var,word, 128);
        if (i==1) strncpy(val,word, 128);
	i++;
    }

    if ((token != EOF && token != EOL) || i>2) {
       printf("SET: bad number of arguments\n");
    } else if (i==0) { 
	EVprint();
    } else if (i==1) { 
	if ((str=EVget(var)) == NULL) {
	   str="";
	}
        printf("%s\n", str);
    } else if (i==2) {
        EVset(var,val);
	EVexport(var);
    }

    return (0);
}

int com_shell(lp, arg)		/* run a program from within the editor */
LEXER *lp;
char *arg;
{
    int debug = 0;
    // char *shell;
    char cmd[1024];
    char word[128];
    TOKEN token;

    if (debug) printf("    com_shell\n");
    // if ((shell=EVget("SHELL")) == NULL) {
    //    shell="/bin/sh";
    // }

    cmd[0] = '\0';
    while((token=token_get(lp, word)) != EOF  && token != EOL) {
        strncat(cmd, word, 1024);
        strncat(cmd, " ", 1024);
    }

    // if (token == EOL) { token_unget(lp, token, word); }

    pig_system(cmd);

    return (0);
}
