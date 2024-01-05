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

    main_loop(); // récupère et traite les commandes entrées.

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
    char* strInput = (char*) NULL; /* Stocke la ligne de commande entrée par l'utilisateur (allocation
    espace mémoire faite par readline). */
    // Paramétrage readline.
    rl_outstream = stderr;
    using_history();
    // Boucle de récupération et de traitement des commandes.
    while (running) {
        // Libération de la mémoire allouée par readline.
        free(strInput);
        // Récupération de la commande entrée et affichage du prompt.
        char* tmp = getPrompt();
        strInput = readline(tmp);
        free(tmp);
        // Tests commande non vide.
        if (strInput == NULL) {
            exit(lastReturn);
        } 
        else if (strlen(strInput) == 0) {
            continue;
        }
        // Traitement de la ligne de commande entrée.
        else {
            add_history(strInput); // Ajoute la ligne de commande entrée à l'historique.
            Command* command = getCommand(strInput);
            if (command != NULL) executeCommand(command, NULL);
        }
    }
    // Libération de la mémoire allouée par readline.
    free(strInput);
}

void executeCommand(Command* command, char* fifo_out_name) {
    char* fifo_in_name = (char*) NULL;
    if (command -> input != NULL) { // Si la commande a une input (dans le contexte d'une pipeline).
        fifo_in_name = malloc(20);
        strcpy(fifo_in_name, "fifo_in");
        mkfifo(fifo_in_name, 0666);
        executeCommand(command -> input, fifo_in_name);
    }
    unsigned cpt = 0;
    for (int i = 0; i < command -> nbArgs; ++i) { // Pour toutes les éventuelles substitutions que la commande utilise.
        if (!strcmp(command -> argsComm[i], "fifo")) {
            char* fifo_name = malloc(20);
            sprintf(fifo_name, "fifo%i", cpt);
            mkfifo(fifo_name, 0666);
            strcpy(command -> argsComm[i], fifo_name);
            executeCommand(command -> substitutions[i], fifo_name);
            free(fifo_name);
        }
        cpt++;
    }
    apply_redirections(command, fifo_in_name, fifo_out_name);
    if (fifo_in_name != NULL) {
        unlink(fifo_in_name);
        free(fifo_in_name);
    }
    // Libération de la mémoire allouée pour la commande.
    free_command(command);
}

void apply_redirections(Command* command, char* fifo_in_name, char* fifo_out_name) {
    int cpy_stdin, cpy_stdout, cpy_stderr;
    int fd_in = -1, fd_out = -1, fd_err = -1;

    // Redirection entrée.
    if (fifo_in_name != NULL) { // Si l'entrée est sur un tube nommé.
        if (command -> in_redir != NULL) {
            fprintf(stderr, "command %s: redirection entrée impossible", command -> argsComm[0]);
        } else {
            cpy_stdin = dup(0);
            fd_in = open(fifo_in_name, O_RDONLY | O_NONBLOCK, 0777);
            dup2(fd_in, 0);
            close(fd_in);
        }
    } else if (command -> in_redir != NULL) { // Si l'entrée est sur un fichier.
        cpy_stdin = dup(0);
        if (!strcmp(command -> in_redir[0], "<")) {
            fd_in = open(command -> in_redir[1], O_RDONLY, 0777);
        }
        dup2(fd_in, 0);
        close(fd_in);
    }

    // Redirection sortie.
    if (fifo_out_name != NULL) { // Si la sortie est un tube nommé.
        if (command -> out_redir != NULL) {
            fprintf(stderr, "command %s: redirection sortie impossible", command -> argsComm[0]);
        } else {
            if (is_internal(command -> argsComm[0])) {
                cpy_stdout = dup(1);
                fd_out = open(fifo_out_name, O_WRONLY | O_NONBLOCK, 0777);
                dup2(fd_out, 1);
                close(fd_out);
            } else { // Cas où la commande est externe.
                cpy_stdout = dup(1);
                // Le reste de la redirection est faite dans external_command().
            }
        }
    } else if (command -> out_redir != NULL) { // Si la sortie est un fichier.
        cpy_stdout = dup(1);
        if (!strcmp(command -> out_redir[0], ">")) {
            fd_out = open(command -> out_redir[1], O_WRONLY|O_APPEND|O_CREAT|O_EXCL, 0777);
            if (fd_out == -1) {
                fprintf(stderr,"bash : %s: file already exist.\n", command -> argsComm[0]);
                lastReturn =  1;
                return;
            }
        } else if (!strcmp(command -> out_redir[0], ">|")) {
            fd_out = open(command -> out_redir[1], O_WRONLY|O_CREAT|O_TRUNC, 0777);
        } else if (!strcmp(command -> out_redir[0], ">>")) {
            fd_out = open(command -> out_redir[1], O_WRONLY|O_APPEND|O_CREAT, 0777);
        }
        dup2(fd_out, 1);
        close(fd_out);
    }

    // Redirection sortie erreur.
    if (command -> err_redir != NULL) { // Si la sortie erreur est un fichier.
        cpy_stderr = dup(2);
        if (!strcmp(command -> err_redir[0], "2>")) {
            fd_err = open(command -> err_redir[1], O_WRONLY|O_APPEND|O_CREAT|O_EXCL, 0777);
            if (fd_err == -1) {
                fprintf(stderr,"bash : %s: file already exists.\n", command -> argsComm[0]);
                lastReturn =  1;
                return;
            }
        } else if (!strcmp(command -> err_redir[0], "2>|")) {
            fd_err = open(command -> err_redir[1], O_WRONLY|O_CREAT|O_TRUNC, 0777);
        } else if (!strcmp(command -> err_redir[0], "2>>")) {
            fd_err = open(command -> err_redir[1], O_WRONLY|O_APPEND|O_CREAT, 0777);
        }
        dup2(cpy_stderr, 2);
        close(fd_err);
    }

    // Appel à l'exécution de la commande.
    if (is_internal(command -> argsComm[0])) {
        callRightCommand(command);
    }
    else external_command(command, fifo_out_name);

    // Remise en état.
    if (fd_in != -1) dup2(cpy_stdin, 0);
    if (fd_out != -1) dup2(cpy_stdout, 1);
    if (fd_err != -1) dup2(cpy_stderr, 2);
}

/* Renvoie true ou false suivant si la commande dont le nom est passé en argument est interne
(ne nécessite pas d'appel à execvp) ou non. */
bool is_internal(char* command_name) {
    if (!strcmp(command_name, "pwd") || !strcmp(command_name, "cd") || !strcmp(command_name, "?") ||
    !strcmp(command_name, "jobs") || !strcmp(command_name, "kill") || !strcmp(command_name, "exit")) return true;
    else return false;
}

// Exécute la bonne commande à partir des mots donnés en argument.
void callRightCommand(Command* command) {
    // Commande pwd
    if (strcmp(command -> argsComm[0], "pwd") == 0) {
        if (correct_nbArgs(command -> argsComm, 1, 1)) {
            char* path = pwd();
            printf("%s\n",path);
            free(path);
        }
    }
    // Commande cd
    else if (strcmp(command -> argsComm[0], "cd") == 0) {
        if (correct_nbArgs(command -> argsComm, 1, 2)) {
            if (command -> argsComm[1] == NULL || strcmp(command -> argsComm[1],"$HOME") == 0) {
                char* home = getenv("HOME");
                cd(home);
            }
            else if (strcmp(command -> argsComm[1],"-") == 0) cd(previous_folder);
            else cd(command -> argsComm[1]);
        }
    }
    // Commande ?
    else if (strcmp(command -> argsComm[0],"?") == 0) {
        if (correct_nbArgs(command -> argsComm, 1, 1)) {
            printf("%d\n",question_mark());
            lastReturn = 0;
        }
    }
    // Commande jobs
    else if (strcmp(command -> argsComm[0],"jobs") == 0) {
        if (correct_nbArgs(command -> argsComm, 1, 1)) { // pour le deuxième jalon pas besoin d'arguments pour jobs
            print_jobs();
            lastReturn = 0;
        }
    }
    // Commande kill
    else if (strcmp(command -> argsComm[0],"kill") == 0) {
        if (correct_nbArgs(command -> argsComm, 2, 3)) {
            lastReturn = killJob(command -> argsComm[1],command -> argsComm[2]);
            if (lastReturn == -1) {
                perror(NULL);
            }
        }
    }
    // Commande exit
    else if (strcmp(command -> argsComm[0], "exit") == 0) {
        if (correct_nbArgs(command -> argsComm, 1, 2)) {
            if (command -> argsComm[1] == NULL) {
                exit_jsh(lastReturn);
            }
            else {
                int int_args = convert_str_to_int(command -> argsComm[1]);
                if (int_args == INT_MIN) {//we check the second argument doesn't contain some chars
                    fprintf(stderr,"Exit takes a normal integer as argument\n");
                }
                else {
                    exit_jsh(int_args);
                }
            }
        }
    }
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

/*
This function returns the error of execvp and is exuting the command "command_name" with the arguments "arguments".
*/
int external_command(Command* command, char* fifo_out_name) {
    pid_t pid = fork();

    if (pid == 0) { // processus enfant
        int tmp = 0;
        if (nbJobs < 40) {
            if (fifo_out_name != NULL) {
                int fd_out = open(fifo_out_name, O_WRONLY);
                dup2(fd_out, 1);
                close(fd_out);
            }
            tmp = execvp(command -> argsComm[0], command -> argsComm);
            fprintf(stderr,"%s\n", strerror(errno)); // Ne s'exécute qu'en cas d'erreur dans l'exécution de execvp.
        }
        exit(tmp);
    }
    else { // processus parent
        int status;
        if (!command -> background) {
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
                char* command_name = malloc(sizeof(char)*strlen(command -> strComm));
                strcpy(command_name,command -> strComm);
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