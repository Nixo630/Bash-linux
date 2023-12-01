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
    // Initialisation variables globales
    previous_folder = pwd();
    current_folder = pwd();
    running = 1;
    lastReturn = 0;
    nbJobs = 0;

    main_loop(); // récupère et découpe les commandes entrées.

    // Libération des buffers
    free(previous_folder);
    free(current_folder);
    return lastReturn;
}

void main_loop() {

    // Initialisation buffers.
    char* strCommand = (char*)NULL; // Stocke la commande entrée par l'utilisateur.
    size_t wordsCapacity = 15; // Capacité initiale de stockage d'arguments.
    char** argsComm = malloc(wordsCapacity * sizeof(char*)); // Stocke les différents morceaux (arguments)
    // de la commande entrée.
    unsigned index; // Compte le nombre d'arguments dans la commande entrée.

    // Paramétrage readline.
    rl_outstream = stderr;
    using_history();

    // Boucle de récupération et de découpe des commandes.
    while (running) {
        reset(argsComm, wordsCapacity);
        free(strCommand);

        char* tmp = getPrompt();
        strCommand = readline(tmp); // Récupère la commande entrée en affichant le prompt.
        free(tmp);

        if (strCommand == NULL) {
            exit(lastReturn);
        } 
        else if (strlen(strCommand) == 0) {
            continue;
        }

        else {
            add_history(strCommand);
            argsComm[0] = strtok(strCommand, " ");
            index = 1;
            while (1) { // Boucle sur les mots d'une commande.
                if (index == wordsCapacity-1) { // Si une commande contient au moins wordsCapacity mots, (le
                // dernier pointeur doit être laissé à NULL pour external_command): allocation supplémentaire.
                    wordsCapacity *= 2;
                    argsComm = realloc(argsComm, wordsCapacity * sizeof(char*));
                }
                argsComm[index] = strtok(NULL, " ");
                if (argsComm[index] == NULL) break;
                ++index;
            }
            if (argsComm[0] != NULL) callRightCommand(argsComm);
        }
    }

    // Libération de la mémoire allouée pour les buffers après terminaison.
    free(strCommand);
    free(argsComm);
}

// Nettoie les buffers de mots d'une commande.
void reset(char** args, size_t len) {
    for (int i = 0; i < len; i++) {
        args[i] = NULL;
    }
}


// Exécute la bonne commande à partir des mots donnés en argument.
void callRightCommand(char** argsComm) {
    // Commande pwd
    if (strcmp(argsComm[0], "pwd") == 0) {
        if (correct_nbArgs(argsComm, 1, 1)) {
            char* path = pwd();
            printf("%s\n",path);
            free(path);
        }
    }
    // Commande cd
    else if (strcmp(argsComm[0], "cd") == 0) {
        if (correct_nbArgs(argsComm, 1, 2)) {
            if (argsComm[1] == NULL || strcmp(argsComm[1],"$HOME") == 0) {
                char* home = getenv("HOME");
                cd(home);
            }
            else if (strcmp(argsComm[1],"-") == 0) cd(previous_folder);
            else cd(argsComm[1]);
        }
    }
    // Commande ?
    else if (strcmp(argsComm[0],"?") == 0) {
        if (correct_nbArgs(argsComm, 1, 1)) {
            printf("%d\n",question_mark());
            lastReturn = 0;
        }
    }
    // Commande exit
    else if (strcmp(argsComm[0], "exit") == 0) {
        if (correct_nbArgs(argsComm, 1, 2)) {
            if (argsComm[1] == NULL) {
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
    }
    // Commandes externes
    else {
        lastReturn = external_command(argsComm);
    }
}

// Retourne true si le nombre d'arguments de la commande passée en argument est correct, 
// retourne false et affiche un message d'erreur sinon.
bool correct_nbArgs(char** argsComm, unsigned min_nbArgs, unsigned max_nbArgs) {
    bool correct_nb = true;
    if (argsComm[min_nbArgs-1] == NULL) {
        fprintf(stderr,"bash : %s: too few arguments\n", argsComm[0]);
        correct_nb = false;
    } else if (argsComm[max_nbArgs] != NULL) {
        fprintf(stderr,"bash : %s: too many arguments\n", argsComm[0]);
        correct_nb = false;
    }
    if (!(correct_nb)) lastReturn = -1;
    return correct_nb;
}

// Vérifie que le pointeur passé en argument est différent de NULL.
void checkAlloc(void* ptr) {
    if (ptr == NULL) {
        fprintf(stderr,"ERROR IN MALLOC : NOT ENOUGH SPACE !");
        exit(-1);
    }
}

char* pwd() {
    unsigned size = 30;
    char* buf = malloc(size * sizeof(char));
    checkAlloc(buf);
    while (getcwd(buf,size) == NULL) { // Tant que getwd produit une erreur.
        if (errno == ERANGE) { // Si la taille de la string représentant le chemin est plus grande que
        // size, on augmente size et on réalloue.
            size *= 2;
            buf = realloc(buf, size * sizeof(char));
            checkAlloc(buf);
        }
        else { // Si l'erreur dans getwd n'est pas dûe à la taille du buffer passé en argument, 
        // on affiche une erreur.
            lastReturn = -1;
            fprintf(stderr,"ERROR IN pwd");
            return buf;
        }
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
                char* home = getenv("HOME");
                cd(home);
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
        lastReturn = 1;
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
        fprintf(stderr,"There are other jobs running.");
        main_loop();
        running = 0;
    }
    else {
        lastReturn = val;
        running = 0;
    }
}

// Retourne le nombre de chiffres dans l'écriture en base 10 du nombre de jobs en cours
// (pour l'affichage).
int length_nbJobs(){
    int i = 1;
    int x = nbJobs;
    while (x>= 10){
        i++;
        x = x/10;
    }
    return i;
}

// Crée et retourne la string correspondant au prompt qui doit être affiché au moment où la fonction
// est appelée.
char* getPrompt() {
    char* prompt = malloc(sizeof(char)* 50);
    int l_nbJobs = length_nbJobs();
    if (strlen(current_folder) == 1) {
        sprintf(prompt, BLEU"[%d]" NORMAL "~$ ", nbJobs);
    }
    else if (strlen(current_folder) <= (26-l_nbJobs)) {
        sprintf(prompt, BLEU"[%d]" NORMAL "%s$ ", nbJobs, current_folder);
    }
    else{
        char* path = malloc(sizeof(char)*(27));
        strncpy(path, (current_folder + (strlen(current_folder) - (23 - l_nbJobs))), (25 - l_nbJobs));
        sprintf(prompt, BLEU"[%d]" NORMAL "...%s$ ", nbJobs, path);
        free(path);
    }
    return prompt;
}