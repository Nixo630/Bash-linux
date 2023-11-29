#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "jsh.h"

#define NORMAL "\033[00m"
#define BLEU "\033[01;34m"

int main(int argc, char** argv) {
   // ----- Tests -----
    current_folder = pwd();
    running = 1;
    lastReturn = 0;
    nbJobs = 0;

    char* buffer = (char*)NULL; // Stocke la commande entrée par l'utilisateur.
    char** argsComm = calloc(15, sizeof(char*)); // Stocke les différents morceaux de la commande entrée.
    unsigned index;
    using_history();

    // Boucle de récupération et de découpe des commandes.
    while (running) {
        print_path();
        buffer = readline(NULL); // Récupère la commande entrée (allocation dynamique).
        if (buffer && *buffer) {
            add_history(buffer);
            argsComm[0] = strtok(buffer, " ");
            index = 1;
            while (1) { // Boucle sur les mots d'une commande.
                if (index == 15) {
                    perror("Too many arguments");
                    exit(EXIT_FAILURE);
                }
                add_history(buffer);
                argsComm[index] = strtok(NULL, " ");
                if (argsComm[index] == NULL) break;
                ++index;
            }
            argsComm[index-1][strlen(argsComm[index-1])] = '\0'; // Enlève le \n de la fin du dernier mot.
            if (strcmp(argsComm[0], "") != 0) callRightCommand(argsComm, index+1);
            }
    }
    // Libération de la mémoire après terminaison.
    free(buffer);
    for (unsigned i = 0; i < index-1; ++i) {
        free(argsComm[i]);
    }
    free(argsComm);
}

// Exécute la bonne commande à partir des mots donnés en argument.
void callRightCommand(char** argsComm, unsigned nbArgs) {
    if (strcmp(argsComm[0], "cd") == 0) {
        if (argsComm[1] == NULL) {
            char* currentFolder = pwd();
            cd("..");
            while(strcmp(currentFolder,pwd()) != 0) {
                currentFolder = pwd();
                cd("..");
            }
            free(currentFolder);
        }
        else {
            cd(argsComm[1]);
        }
    }
    else if (strcmp(argsComm[0], "pwd") == 0) {
        char* folder = pwd();
        fprintf(stderr,"%s\n",folder);
        free(folder);
    }
    else if (strcmp(argsComm[0], "exit") == 0) exit_jsh();
    else {
        argsComm[nbArgs] = "NULL";
        external_command(argsComm);
    }
}

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
    current_folder = pwd();
}

int question_mark() {
    return lastReturn;
}

void exit_jsh() {
    running = 0;
}

void print_path (){
    fprintf(stderr,BLEU"[%d]",nbJobs);
    // size of the 30 char- size of "$ " - size of "[jobs]" - size of "..." 
    if (strlen(current_folder) == 1) fprintf(stderr,NORMAL "~$ ");
    else if (strlen(current_folder)<=30) fprintf(stderr,NORMAL "%s$ ", current_folder);
    else{
        char *path = malloc(sizeof(char)*(strlen(current_folder)));
        *path = '.';
        *(path+1)= '.';
        *(path+2) = '.';
        for (int i = strlen(current_folder)-30; i <= strlen(current_folder); i++){
            *(path+i-(strlen(current_folder)-30)) = *(current_folder+i);
        }
        fprintf(stderr,NORMAL "%s$ ", path);
        free(path);
    }
}
