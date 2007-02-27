#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "path.h"

char *home();

/*
char *PATH=".:./.pigrc:~/.pigrc:/usr/local/lib/piglet:/usr/lib/piglet";
char *FILENAME="NOTEDATA.F";
int main() {
    char buf[128];
    findfile(PATH, FILENAME, buf);
    if (buf[0] != '\0') {
	printf("%s\n", buf);
    } else {
	printf("Could not file %s in $PATH\n", FILENAME);
	printf("PATH=\"%s\"\n", PATH);
    }
    return(0);
}
*/

void findfile(const char *pathlist, const char *filename, char *retbuf) {
    char path[128];
    char buf[128];
    char head[128];
    char tail[128];
    char full[128];
    char *p, *q;
    FILE *fp;

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

	if ((fp=fopen(full, "r")) != NULL) {
	    fclose(fp);
	    strcpy(retbuf, full);
	    return;
	} 
    }
    retbuf[0] = '\0';
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

