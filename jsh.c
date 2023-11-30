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
#include <limits.h>
#include "jsh.h"

#define NORMAL "\033[00m"
#define BLEU "\033[01;34m"

int main(int argc, char** argv) {
    previous_folder = pwd();
    current_folder = pwd();
    running = 1;
    lastReturn = 0;
    nbJobs = 0;

    main_loop();
    //printf("return value of jsh = %d\n",lastReturn);
    return lastReturn;
}

void main_loop() {
    char* buffer = (char*)NULL; // Stocke la commande entrée par l'utilisateur.
    char** argsComm = calloc(15, sizeof(char*)); // Stocke les différents morceaux de la commande entrée.
    unsigned index;
    using_history();
    // Boucle de récupération et de découpe des commandes.
    while (running) {
        reset(argsComm);
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
    free(argsComm);
}

// Exécute la bonne commande à partir des mots donnés en argument.
void callRightCommand(char** argsComm, unsigned nbArgs) {
    if (strcmp(argsComm[0], "cd") == 0) {
        if (argsComm[2] != NULL) {
            fprintf(stderr,"bash : cd: too many arguments\n");
            lastReturn = -1;
        }
        else if (argsComm[1] == NULL) {
            char* currentFolder = pwd();
            cd("..");
            while(strcmp(currentFolder,pwd()) != 0) {
                currentFolder = pwd();
                cd("..");
            }
            free(currentFolder);
        }
        else if (strcmp(argsComm[1],"-") == 0) {
            fprintf(stderr,"%s\n",previous_folder);
            cd(previous_folder);
        }
        else {
            cd(argsComm[1]);
        }
    }
    else if (strcmp(argsComm[0], "pwd") == 0) {
        if (argsComm[1] != NULL) {
            fprintf(stderr,"bash : pwd: too many arguments\n");
            lastReturn = -1;
        }
        else {
            char* folder = pwd();
            printf("%s\n",folder);
            free(folder);
        }
    }
    else if (strcmp(argsComm[0], "exit") == 0) {
        if (argsComm[2] != NULL) {
            fprintf(stderr,"bash : exit: too many arguments\n");
            lastReturn = -1;
        }
        else if (argsComm[1] == NULL) {
            exit_jsh(lastReturn);
        }
        else {
            char** tmp = malloc(sizeof(char)*50);
            int int_args = strtol(argsComm[1],tmp,10);//base 10 and we store invalids arguments in tmp
            if ((strcmp(tmp[0],"") != 0 && strlen(tmp[0]) > 0) || int_args == LONG_MIN || int_args == LONG_MAX) {//we check the second argument doesn't contain some chars
                fprintf(stderr,"Exit takes an normal integer as argument\n");
            }
            else {
                exit_jsh(int_args);
            }
            free(tmp);
        }
    }
    else if (strcmp(argsComm[0],"?") == 0) {
        if (argsComm[1] != NULL) {
            fprintf(stderr,"bash : ?: too many arguments");
            lastReturn = -1;
        }
        else {
            fprintf(stderr,"%d\n",question_mark());
        }
    }
    else {
        argsComm[nbArgs] = "NULL";
        lastReturn = external_command(argsComm);
    }
}

void reset(char** args) {
    for (int i = 0; i < 15; i++) {
        args[i] = NULL;
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
    char* tmp = pwd();
    lastReturn = chdir(pathname);
    if (lastReturn == -1) {
        switch (errno) {
            case (ENOENT) : {
                char* currentFolder = pwd();
                cd("..");
                while(strcmp(currentFolder,pwd()) != 0) {
                    currentFolder = pwd();
                    cd("..");
                }
                free(currentFolder);
                lastReturn = chdir(pathname);//we returned to the root and try again
                if (lastReturn == -1) {
                    if (errno == ENOENT) {
                        cd(tmp);//if this doesn't work we return where we were
                        fprintf(stderr,"cd : non-existent folder\n");break;
                    }
                    else {
                        cd(tmp);//if this doesn't work we return where we were
                    }
                }
                else {
                    strcpy(previous_folder,tmp);
                    char* tmp2 = pwd();
                    strcpy(current_folder,tmp2);
                    free(tmp2);
                    break;
                }
            }
            case (EACCES) : fprintf(stderr,"cd : Access restricted\n");break;
            case (ENAMETOOLONG) : fprintf(stderr,"cd : Folder name too long\n");break;
            case (ENOTDIR) : fprintf(stderr,"cd : An element is not a dir\n");break;
            case (ENOMEM) : fprintf(stderr,"cd : Not enough memory for the core\n");break;
            default : fprintf(stderr,"Unknown error !\n");break;
        }
        free(tmp);
    }
    else {
        strcpy(previous_folder,tmp);
        free(tmp);
        char* tmp2 = pwd();
        strcpy(current_folder,tmp2);
        free(tmp2);
    }
}

int question_mark() {
    return lastReturn;
}

void exit_jsh(int val) {
    if (nbJobs > 0) {
        lastReturn = 1;
        fprintf(stderr,"There is other jobs running");
        main_loop();
        running = 0;
    }
    else {
        lastReturn = val;
        running = 0;
    }
}

void print_path (){
    fprintf(stderr,BLEU"[%d]",nbJobs);
    // size of the 30 char- size of "$ " - size of "[jobs]" - size of "..." 
    if (strlen(current_folder) == 1) fprintf(stderr,NORMAL "~$ ");
    else if (strlen(current_folder)<=25) fprintf(stderr,NORMAL "%s$ ", current_folder);
    else{
        char *path = malloc(sizeof(char)*27);
        *path = '.';
        *(path+1)= '.';
        *(path+2) = '.';
        for (int i = strlen(current_folder)-22; i <= strlen(current_folder); i++){
            *(path+i-(strlen(current_folder)-25)) = *(current_folder+i);
        }
        fprintf(stderr,NORMAL "%s$ ", path);
        free(path);
    }
}
