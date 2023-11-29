#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "jsh.h"

#define NORMAL "\033[00m"
#define BLEU "\033[01;34m"

int lastReturn = 0;
int nbJobs = 0;
char* current_folder ;

char* pwd () {
    lastReturn = -1;
    int size = 30;;
    char* buf = malloc(sizeof(char)*(size));
    if (buf == NULL) {
        fprintf(stderr,"ERROR IN MALLOC : DONT HAVE ENOUGH SPACE !");
        exit(-1);
    }
    char* returnValue = malloc(sizeof(char)*(size));
    if (returnValue == NULL) {
        fprintf(stderr,"ERROR IN MALLOC : DONT HAVE ENOUGH SPACE !");
        exit(-1);
    }

    returnValue = getcwd(buf,size);
    while (returnValue == NULL && errno == ERANGE) {
        size++;
        buf = malloc(sizeof(char)*(size));
        if (buf == NULL) {
            fprintf(stderr,"ERROR IN MALLOC : DONT HAVE ENOUGH SPACE !");
            exit(-1);
        }
        returnValue = malloc(sizeof(char)*(size));
        if (returnValue == NULL) {
            fprintf(stderr,"ERROR IN MALLOC : DONT HAVE ENOUGH SPACE !");
            exit(-1);
        }
        returnValue = getcwd(buf,size);
    }
    if (returnValue == NULL) {
        fprintf(stderr,"ERROR IN pwd");
        return buf;
    }
    lastReturn = 0;
    return buf;
}

/*
This function returns the error of execvp and is exuting the command "command_name" with the arguments "arguments".
*/
int external_command(char** command) {
    pid_t pid = fork();

    if (pid == 0) {
        int tmp = execvp(command[0],command);
        fprintf(stderr,"Wrong command name\n");
        exit(tmp);
    }
    else {
        int status;
        waitpid(pid,&status,0);
        return WEXITSTATUS(status);//return the exit value of the son
    }
}

void cd (char* pathname) {
    lastReturn = chdir(pathname);
    if (lastReturn == -1) {
        switch (errno) {
            case (ENOENT) : fprintf(stderr,"cd : non-existent folder\n");break;
            case (EACCES) : fprintf(stderr,"cd : Access restricted\n");break;
            case (ENAMETOOLONG) : fprintf(stderr,"cd : Folder name too long\n");break;
            case (ENOTDIR) : fprintf(stderr,"cd : An element is not a dir\n");break;
            case (ENOMEM) : fprintf(stderr,"cd : Not enough memory for the core\n");break;
            default : fprintf(stderr,"Unknown error !\n");break;
        }
    }
}

int question_mark() {
    return lastReturn;
}

void print_path (){
    char * jobs = malloc(sizeof(char));
    char *temp = malloc(sizeof(char));
    *jobs = '[';
    sprintf(temp,"%d",nbJobs);
    int length = strlen(temp)+2;// length of "[nbJobs]" 
    *(jobs+1) = *temp;
    *(jobs+1+length-2) = ']';
    printf(BLEU"%s",jobs);
    
    if (strlen(current_folder)<(28-(length))) printf(NORMAL "%s$ ", current_folder);
    else{
        
        int x = strlen(current_folder)-28+length+3;
        // size of the 30 char- size of "$ " - size of "[jobs]" - size of "..." 
        char *path = malloc(sizeof(char)*30);
        *path = '.';
        *(path+1)= '.';
        *(path+2) = '.';
        for (int i = x ; i <= strlen(current_folder); i++){
            *(path+i-x+3) = *(current_folder+i);
        }
        printf(NORMAL "%s$ ", path);
        free(path);
    }
    free(temp);
    free(jobs);
}
/*
void read_file(){
    char * input = malloc (sizeof(char));
    using_history();
    char * delimitor = " ";
    while ((input = readline(""))) {

        if (strlen(input) > 0 ){
            add_history (input);
            /* char * function = strtok(input,delimitor);
            // la fonction
            if (strcmp(function,"pwd")) {
                printf("%s\n",current_folder);
                lastReturn = 0;
                }
            */
            
            }
            
        }
        
       
    }
     free (input);
}
*/
int main(int argc, char** argv) {
    current_folder = pwd();
    /*
    printf("pwd command = \n%s\n\n",current_folder);
    free(current_folder);
    char* test[] = {"dune","--version",NULL};//we need to have a NULL at the end of the list for the execvp to work
    printf("test dune command =\n");
    lastReturn = external_command(test);
    printf("? command = %d\n",question_mark());
    //Tests cd
    cd("test");
    current_folder = pwd();
    printf("pwd command = \n%s\n\n",current_folder);*/
    printf(BLEU "j'écrit en bleu\n");
    printf(NORMAL"j'écrit en blanc\n");
    print_path();
}
