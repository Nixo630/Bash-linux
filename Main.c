#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "Main.h"

char* pwd () {
    int* size = malloc(sizeof(int));
    char* buf = malloc(sizeof(char)*(*size));
    if (buf == NULL || size == NULL) {
        dprintf(STDERR_FILENO,"ERROR IN MALLOC : DONT HAVE ENOUGH SPACE !");
        exit(0);
    }
    *size = 30;
    char* returnValue = malloc(sizeof(char)*(*size));
    if (returnValue == NULL) {
        dprintf(STDERR_FILENO,"ERROR IN MALLOC : DONT HAVE ENOUGH SPACE !");
        exit(0);
    }

    returnValue = getcwd(buf,*size);
    while (returnValue == NULL && errno == ERANGE) {
        *size = (*size)+1;
        buf = malloc(sizeof(char)*(*size));
        if (buf == NULL) {
            dprintf(STDERR_FILENO,"ERROR IN MALLOC : DONT HAVE ENOUGH SPACE !");
            exit(0);
        }
        returnValue = malloc(sizeof(char)*(*size));
        if (returnValue == NULL) {
            dprintf(STDERR_FILENO,"ERROR IN MALLOC : DONT HAVE ENOUGH SPACE !");
            exit(0);
        }
        returnValue = getcwd(buf,*size);
    }
    if (returnValue == NULL) {
        dprintf(STDERR_FILENO,"ERROR IN pwd");
        exit(0);
    }
    //free(returnValue);
    free(size);
    return buf;
}

int external_command(char* command_name, char** arguments) {
    int* pid = malloc(sizeof(int));
    *pid = fork();
    if (*pid < 0) {
        dprintf(STDERR_FILENO,"ERROR IN FORK !");
        exit(0);
    }
    else if (*pid == 0) {
        int* buf = malloc(sizeof(int));
        *buf = execvp(command_name,arguments);
        if (*buf == -1) {
            free(buf);
            exit(errno);
        }
        free(buf);
        exit(0);
    }
    else {
        int* status = malloc(sizeof(int));
        waitpid(*pid,status,0);
        free(pid);
        return WEXITSTATUS(*status);//return the exit value of the son
    }
}

int main(int argc, char** argv) {
    char* current_folder = pwd();
    printf("pwd command = \n%s\n\n",current_folder);
    free(current_folder);
    char* test[] = {NULL};//we need to have a NULL at the end of the list for the execvp to work
    printf("test dune command = \n");
    external_command("dune",test);
}
