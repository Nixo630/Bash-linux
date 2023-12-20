#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <limits.h>
#include <signal.h>
#include <sys/stat.h>
#include "toolbox_jsh.h"
#include "parsing_jsh.h"
#include "jobs_jsh.h"
#include "jsh.h"

extern int nbJobs;
extern Job* l_jobs;

#define NORMAL "\033[00m"
#define BLEU "\033[01;34m"

int main(int argc, char** argv) {
    // Initialisation variables globales
    previous_folder = pwd();
    current_folder = pwd();
    running = 1;
    lastReturn = 0;
    nbJobs = 0;
    l_jobs = malloc(sizeof(Job)*40); //maximum de 40 jobs simultanément

    main_loop(); // récupère et découpe les commandes entrées.

    // Libération des buffers
    free(previous_folder);
    free(current_folder);
    free(l_jobs);
    for (int i = 0; i < nbJobs; i++) {
        kill(l_jobs[i].pid,SIGKILL);
    }
    return lastReturn;
}

void main_loop() {
    // Initialisation buffers.
    char* strCommand = (char*)NULL; // Stocke la commande entrée par l'utilisateur.
    size_t* argsCapacity = malloc(sizeof(size_t)); // Taille du tableau récupérant les arguments d'une commande.
    (*argsCapacity) = STARTING_ARGS_CAPACITY; // Est augmentée si nécessaire par parse_command.
    char** argsComm = malloc((*argsCapacity) * sizeof(char*)); // Stocke les différents arguments de la commande entrée.
    unsigned index; // Compte le nombre d'arguments dans la commande entrée.
    // Paramétrage readline.
    rl_outstream = stderr;
    using_history();
    // Boucle de récupération et de découpe des commandes.
    while (running) {
        // Nettoyage buffers.
        reset(argsComm, argsCapacity);
        free(strCommand);
        // Récupération de la commande entrée et affichage du prompt.
        char* tmp = getPrompt();
        strCommand = readline(tmp);
        free(tmp);
        // Tests commande non vide.
        if (strCommand == NULL) {
            exit(lastReturn);
        } 
        else if (strlen(strCommand) == 0) {
            continue;
        }
        // Traitement de la commande entrée.
        else {
            add_history(strCommand);
            index = parse_command(strCommand, argsComm, argsCapacity); // Découpage de la commande.
            callRightCommand(argsComm, index, strCommand); //dans la commande jobs on a besoin du buffer
        }
    }
    // Libération de la mémoire allouée pour les buffers après terminaison.
    free(strCommand);
    free(argsComm);
}





// Exécute la bonne commande à partir des mots donnés en argument.
void callRightCommand(char** argsComm, unsigned nbArgs, char* buffer) {
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
    // Commande jobs
    else if (strcmp(argsComm[0],"jobs") == 0) {
        if (correct_nbArgs(argsComm, 1, 1)) { // pour le deuxième jalon pas besoin d'arguments pour jobs
            print_jobs();
            lastReturn = 0;
        }
    }
    // Commande kill
    else if (strcmp(argsComm[0],"kill") == 0) {
        if (correct_nbArgs(argsComm, 2, 3)) {
            lastReturn = killJob(argsComm[1],argsComm[2]);
            if (lastReturn == -1) {
                perror(NULL);
            }
        }
    }
    // Commande exit
    else if (strcmp(argsComm[0], "exit") == 0) {
        if (correct_nbArgs(argsComm, 1, 2)) {
            if (argsComm[1] == NULL) {
                exit_jsh(lastReturn);
            }
            else {
                int int_args = convert_str_to_int(argsComm[1]);
                if (int_args == INT_MIN) {//we check the second argument doesn't contain some chars
                    fprintf(stderr,"Exit takes a normal integer as argument\n");
                }
                else {
                    exit_jsh(int_args);
                }
            }
        }
    }
    // Commandes externes
    else {
        if (strcmp(argsComm[nbArgs-1],"&") == 0 && nbArgs-1 == 0) {
            fprintf(stderr,"Wrong command name\n");
        }
        else if (strcmp(argsComm[nbArgs-1],"&") == 0) {
            argsComm[nbArgs-1] = NULL;
            lastReturn = external_command(argsComm,true,buffer);
        }
        else {
            lastReturn = external_command(argsComm,false,buffer);
        }
    }
}

void entry_redirection(char** argsComm, unsigned nbArgs, char* buffer, char * pathname ){
    int cpy_stin = dup(STDIN_FILENO);
    int fd = open(pathname,O_WRONLY|O_APPEND);
        dup2(fd,STDIN_FILENO);
        callRightCommand(argsComm,nbArgs,buffer);
        dup2(cpy_stin,0);
}

void simple_redirection(char** argsComm, unsigned nbArgs, char* buffer,bool error, char * pathname ){
    int flow;
    int second_flow;
    if (error) {
        flow = STDERR_FILENO;
        second_flow = 2;
    }
    else {
        flow = STDOUT_FILENO;
        second_flow = 1;
    }
    int cpy_stdout = dup(flow);
    int fd = open(pathname,O_WRONLY|O_APPEND|O_CREAT|O_EXCL);
    if (fd == -1){
        fprintf(stderr,"bash : %s: file does not exist\n", argsComm[0]);
        lastReturn = 1;
    }
    else{
        dup2(fd,flow);
        callRightCommand(argsComm,nbArgs,buffer);
        dup2(cpy_stdout,second_flow);
    }
}


void overwritte_redirection(char** argsComm, unsigned nbArgs, char* buffer, char * pathname ){
    int cpy_stdout = dup(STDOUT_FILENO);
    int fd = open(pathname,O_WRONLY|O_APPEND|O_TRUNC);
    dup2(fd,STDOUT_FILENO);
    callRightCommand(argsComm,nbArgs,buffer);
    dup2(cpy_stdout,1);
}

void concat_redirection(char** argsComm, unsigned nbArgs, char* buffer, char * pathname ){
    int cpy_stdout = dup(STDOUT_FILENO);
    int fd = open(pathname,O_WRONLY|O_APPEND|O_APPEND);
    dup2(fd,STDOUT_FILENO);
    callRightCommand(argsComm,nbArgs,buffer);
    dup2(cpy_stdout,1);
}


void error_overwritte_redirection(char** argsComm, unsigned nbArgs, char* buffer, char * pathname ){
    int cpy_stderr = dup(STDERR_FILENO);
    int fd = open(pathname,O_WRONLY|O_APPEND|O_TRUNC);
    dup2(fd,STDERR_FILENO);
    callRightCommand(argsComm,nbArgs,buffer);
    dup2(cpy_stderr,2);
}

void error_concat_redirection(char** argsComm, unsigned nbArgs, char* buffer, char * pathname ){
    int cpy_stderr = dup(STDERR_FILENO);
    int fd = open(pathname,O_WRONLY|O_APPEND|O_APPEND);
    dup2(fd,STDERR_FILENO);
    callRightCommand(argsComm,nbArgs,buffer);
    dup2(cpy_stderr,2);
}

void cmd_redirection (char** argsComm_1, unsigned nbArgs_1, char* buffer_1,char** argsComm_2, unsigned nbArgs_2, char* buffer_2){
    int t[2];
    pipe(t);
    int cpy_1 = dup(1);
    int cpy_2 = dup(0);
    dup2(t[1],STDOUT_FILENO);
    dup2(t[0],STDIN_FILENO);
    callRightCommand(argsComm_1,nbArgs_1,buffer_1);
    int n = read(t[0],argsComm_2[0],100);
    if (n<0) {
        dup2(cpy_1,1);
        dup2(cpy_2,0);
        perror("argument invalids, no OUT value of cm1"); 
        lastReturn = 1;
    }
    else{
        callRightCommand(argsComm_2,nbArgs_1,buffer_1);
    }
    dup2(cpy_1,1);
    dup2(cpy_2,0);
}




/* Retourne true si le nombre d'arguments de la commande passée en argument est correct, 
affiche un message d'erreur et retoure false sinon. */
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

char* pwd() {
    unsigned size = 30;
    char* buf = malloc(size * sizeof(char));
    checkAlloc(buf);
    while (getcwd(buf,size) == NULL) { // Tant que getwd produit une erreur.
        if (errno == ERANGE) { /* Si la taille de la string représentant le chemin est plus grande que
        size, on augmente size et on réalloue. */
            size *= 2;
            buf = realloc(buf, size * sizeof(char));
            checkAlloc(buf);
        }
        else { /* Si l'erreur dans getwd n'est pas dûe à la taille du buffer passé en argument, 
        on affiche une erreur. */
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
int external_command(char** command, bool create_new_job, char* buffer) {
    pid_t pid = fork();

    if (pid == 0) {
        int tmp = 0;
        if (nbJobs < 40) {
            tmp = execvp(command[0],command);
            fprintf(stderr,"Wrong command name\n");
        }
        exit(tmp);
    }
    else {
        int status;
        if (!create_new_job) {
            waitpid(pid,&status,0);
        }
        else {
            sleep(1);
            int status;
            pid_t state = waitpid(pid,&status,WNOHANG);
            if (nbJobs == 40) {
                fprintf(stderr,"Too much jobs running simultaneously\n");
                return -1;
            }
            else if (state != 0) {
                if (WIFEXITED(status)) {//si le fils s'est terminé normalement c'est qu'en parametre il y avait une fonction instantané
                    fprintf(stderr,"[%d] %d\n",nbJobs,pid);
                }
                return -1;
            }
            else {
                nbJobs++;
                char* command_name = malloc(sizeof(char)*strlen(buffer));
                strcpy(command_name,buffer);
                int i = strlen(command_name)-1;
                while (true) {//supprimer le & a la fin de la commande
                    if (*(command_name+i) == '&') {
                        *(command_name+i) = ' ';
                        break;
                    }
                    *(command_name+i) = ' ';
                    i--;
                }
                char* state = malloc(sizeof(char)*8);
                strcpy(state,"Running");
                Job tmp = {nbJobs, pid, state, command_name};
                l_jobs[nbJobs-1] = tmp;
                fprintf(stderr,"[%d] %d\n",nbJobs,pid);
                return 0;
            }
        }
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
        //char* tmp[] = {"clear",NULL};
        //external_command(tmp,false,"clear");
        fprintf(stderr,"There are other jobs running.\n");
        //main_loop();
        running = 0;
    }
    else {
        lastReturn = val;
        running = 0;
    }
}

char* getPrompt() {
    char* prompt = malloc(sizeof(char)* 50);
    int l_nbJobs = length_base10(nbJobs);
    int status;
    for (int i = 0; i < nbJobs; i++) {
        if (waitpid(l_jobs[i].pid,&status,WNOHANG) != 0) {
            if (WIFEXITED(status)) {
                kill(l_jobs[i].pid,SIGKILL);
                free(l_jobs[i].state);
                char* state = malloc(sizeof(char)*5);
                strcpy(state,"Done");
                l_jobs[i].state = state;
                print_job(l_jobs[i]);
                removeJob(i);
                nbJobs--;
            }
            else if (WIFSTOPPED(status)) {
                free(l_jobs[i].state);
                char* state = malloc(sizeof(char)*8);
                strcpy(state,"Stopped");
                l_jobs[i].state = state;
                print_job(l_jobs[i]);
            }
            else if (WIFCONTINUED(status)) {
                free(l_jobs[i].state);
                char* state = malloc(sizeof(char)*10);
                strcpy(state,"Continued");
                l_jobs[i].state = state;
                print_job(l_jobs[i]);
            }
            else if (WIFSIGNALED(status)) {
                free(l_jobs[i].state);
                char* state = malloc(sizeof(char)*11);
                strcpy(state,"Terminated");
                l_jobs[i].state = state;
                print_job(l_jobs[i]);
                removeJob(i);
                nbJobs--;
            }
        }
    }
    if (strlen(current_folder) == 1) {
        sprintf(prompt, BLEU"[%d]" NORMAL "c$ ", nbJobs);
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
