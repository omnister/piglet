#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "path.h"

char *home();

/*
char *PIG_PATH=".:./.pigrc:~/.pigrc:/usr/local/lib/piglet:/usr/lib/piglet";
char *FILENAME="NOTEDATA.F";
int main() {
    char buf[128];
    findfile(PIG_PATH, FILENAME, buf, R_OK);
    if (buf[0] != '\0') {
	printf("%s\n", buf);
    } else {
	printf("Could not file %s in $PIG_PATH\n", FILENAME);
	printf("PIG_PATH=\"%s\"\n", PIG_PATH);
    }
    return(0);
}
*/

/* find the first <filename> that exists in <pathlist> checking for either 
** readonly (R_OK) or executable (X_OK) access() characteristics.  Return
** full path in (char *retbuf) unless retbuf==NULL.  In which case return
** code provides a simple check for existence. 1=exists, 0=missing.
*/

int findfile(const char *pathlist, const char *filename, char *retbuf, int mode) {
    char path[1024];
    char buf[1024];
    char head[1024];
    char tail[1024];
    char full[1024];
    char *p, *q;

    if (filename[0] != '\0') {
	strcpy(path,pathlist);
	for (p=strtok(path, ":"); p!=NULL; p=strtok(NULL, ":")) {
	    strcpy(buf, p);
	    if (strlen(buf) > 0) {
	       strcat(buf,"/");
	    }

	    if (buf[0] == '~') {	/* do tilde expansion */
		strcpy(head,buf);
		if ((q = strchr(head,'/')) != NULL) {
		    *q = '\0';
		    strcpy(tail,q+1);
		}
		strcpy(full,home(head+1));
		strcat(full,"/");
		strcat(full,tail);
	    } else if (buf[0] == '.' && buf[1] == '/') { /* do dot expansion */
		getcwd(full, 128);
		strcat(full,buf+1);
	    } else {
		strcpy(full,buf);
	    }

	    strcat(full,filename);

	    if (access(full, mode) == 0) { 	    /* success */
		if (retbuf != NULL) {
		    strcpy(retbuf, full);
		}
		return(1);
	    }
	}
    }
    if (retbuf != NULL) {
	retbuf[0] = '\0';
    }
    return(0);
}


char *home(char *name) {
    struct passwd *p;
    if (name == NULL || strlen(name) == 0) {	/* return my HOME */
	return(getpwuid(getuid())->pw_dir);	
    } else {
	if((p=getpwnam(name))==NULL) {
	    return(name);			/* no expansion */
	} else {
	    return(p->pw_dir);		        /* return name's HOME */
	}
    }
}

