#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "jsh.h"

int lastReturn;
bool running;

// Fonctions auxilliaires
void callRightCommand(char(*)[100]);

// Fonctions de commande
char* pwd();
void cd(char*);
int question_mark();
int external_command(char**);
void exit_jsh();

int main(int argc, char** argv) {
   // ----- Tests -----
    char* current_folder = pwd();
    printf("pwd command = \n%s\n\n",current_folder);
    free(current_folder);
    char* test[] = {"dune","--version",NULL};//we need to have a NULL at the end of the list for the execvp to work
    printf("test dune command =\n");
    lastReturn = external_command(test);
    printf("? command = %d\n",question_mark());
    //Tests cd
    cd("test");
    current_folder = pwd();
    printf("pwd command = \n%s\n\n",current_folder);

    // Initialisation des variables globales.
    lastReturn = 0;
    running = 1;

    size_t buffSize = 150;
    char* buffer; // Stocke la commande entrée par l'utilisateur.
    char argsComm[3][100] = {"", "", ""}; // Stocke les différents morceaux de la commande entrée.
    char* arg; // Stocke temporairement chaque mot de la commande.

    // Boucle de récupération et de découpe des commandes.
    while (running) {
        getline(&buffer, &buffSize, stdin); // Récupère la commande entrée (allocation dynamique).
        arg = strtok(buffer, " "); // allocation dynamique de l'espace pointé par arg.
        strcpy(argsComm[0], arg);
        arg = strtok(NULL, " ");
        unsigned index = 0;
        while (arg != NULL) { // Boucle sur les mots d'une commande.
            ++index;
            if (index == 3) {
                perror("Trop d'arguments");
                exit(EXIT_FAILURE);
            }
            strcpy(argsComm[index], arg);
            arg = strtok(NULL, " ");
        }
        argsComm[index][strlen(argsComm[index]) - 1] = '\0'; // Enlève \n de la fin du dernier mot.
        if (strcmp(argsComm[0], "") != 0) callRightCommand(argsComm);
    }
    // Libération de la mémoire après terminaison.
    free(buffer);
    free(arg);
}

// Exécute la bonne commande à partir des mots donnés en argument.
void callRightCommand(char argsComm[3][100]) {
    if (strcmp(argsComm[0], "cd") == 0) {
        if (strcmp(argsComm[0], "")) strcpy(argsComm[1], ".");
        cd(argsComm[1]);
    }
    else if (strcmp(argsComm[0], "pwd") == 0) pwd();
    else if (strcmp(argsComm[0], "exit") == 0) exit_jsh();
    else {
        perror("Commande invalide");
        exit(EXIT_FAILURE);
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
}

int question_mark() {
    return lastReturn;
}

void exit_jsh() {
    running = 0;
}