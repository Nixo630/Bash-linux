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
#include <dirent.h>
#include "jobs_jsh.h"
#include "toolbox_jsh.h"

void print_job(Job job) {
    fprintf(stderr,"[%d] %d %s %s\n",job.nJob,job.pid,job.state,job.command_name);
}

void print_jobs(pid_t job,bool isJob,bool tHyphen) {
    for (int i = 0; i < nbJobs; i++) {
        if (l_jobs[i].pid != 0) {
            if (tHyphen && job && l_jobs[i].pid == job) {
                print_job(l_jobs[i]);
                inspectAllSons(l_jobs[i].pid,0,true,false);
            }
            else if (tHyphen) {
                print_job(l_jobs[i]);
                inspectAllSons(l_jobs[i].pid,0,true,false);
            }
            else if (isJob && l_jobs[i].pid == job) print_job(l_jobs[i]);
            else print_job(l_jobs[i]);
        }
    }
}

void removeJob (int n) {
    l_jobs[n].nJob = 0;
    l_jobs[n].pid = 0;
    free(l_jobs[n].state); 
    for (int i = n; i < nbJobs; i++) {
        l_jobs[i] = l_jobs[i+1];
    }
}

void create_job(char * command_name, char *status, pid_t pid, char * ground){
    nbJobs++;
    char* state = malloc(sizeof(char)*strlen(status));
    strcpy(state,status);
    Job job = {nbJobs, pid, state, command_name,ground};
    l_jobs[nbJobs-1] = job;
}

bool inspectAllSons(pid_t pid3, int sig,bool print,bool hasStopped) {
    DIR* dir = opendir("/proc");
    struct dirent* entry;
    entry = readdir(dir);
    while (entry != NULL) {
        if (entry->d_type == DT_DIR) {//si c'est un fichier
            char* aPid = entry->d_name;
            int somePid = convert_str_to_int(aPid);
            if (somePid != INT_MIN) {//si le format est le bon
                char* statFile = malloc(sizeof(char)*256);//entry->d_name fait max 256 char
                sprintf(statFile,"/proc/%s/stat",aPid);
                int stat = open(statFile,O_RDONLY);
                char* result = malloc(sizeof(char)*50);
                read(stat,result,50);
                int countSpaces = 0;
                int i = 0;
                int maxCountSpaces;
                if (!hasStopped) maxCountSpaces = 3;//troisieme argument de stat si on veut pas savoir si pid a stop donc le ppid
                else maxCountSpaces = 2;//deuxieme argument de stat donc l'etat du processus
                while (countSpaces != maxCountSpaces) {
                    if (*(result+i) == ' ') {
                        countSpaces++;
                    }
                    i++;
                }
                if (hasStopped && somePid == pid3) {
                    return 'T' == *(result+i);
                }
                int j = i;
                while (*(result+i) != ' ') {
                    i++;
                }
                char* strPpid = malloc(sizeof(char)*(i-j)+1);
                int k = 0;
                while (*(result+j+k) != ' ') {
                    *(strPpid+k) = *(result+j+k);
                    k++;
                }
                *(strPpid+k) = '\0';
                int intPpid = convert_str_to_int(strPpid);
                if (intPpid == pid3) {
                    if (print) {
                        for (int z = 0; z < nbJobs; z++) {
                            if (l_jobs[z].pid == pid3) {
                                printf("    | %d %s %s\n",somePid,l_jobs[z].state,l_jobs[z].command_name);
                            }
                        }
                    }
                    inspectAllSons(somePid,sig,print,hasStopped);//on kill aussi tous les fils des fils
                    if (!print) kill(somePid,sig);//kill le fils de -pid3
                    //printf("oh=%d\n",intPpid);
                }
                free(strPpid);
                free(result);
                free(statFile);
            }
        }
        entry = readdir(dir);
    }
    closedir(dir);
    return true;//cela ne change rien
}

int killJob (char* sig, char* pid) {
    char* pid2;
    if (pid == NULL) {
        pid2 = malloc(sizeof(char)*strlen(sig));
    }
    else if (*(pid) == '-') {
        fprintf(stderr,"error with arguments\n");
        return -1;
    }
    else {
        pid2 = malloc(sizeof(char)*strlen(pid));
    }
    char* sig2 = malloc(sizeof(char)*strlen(sig));
    char* sig3 = malloc(sizeof(char)*(strlen(sig)-1));
    if (pid == NULL) {
        strcpy(pid2,sig);//si pid est null cela signifie que nous n'avons potentiellement que le signal qui sera donc par defaut SIGTERM
        strcpy(sig2,"-15");//on met donc SIGTERM dans sig
    }
    else {
        strcpy(sig2,sig);
        strcpy(pid2,pid);
    }
    if (*sig2 != '-') {
        free(pid2);
        free(sig2);
        free(sig3);
        fprintf(stderr,"wrong command : maybe you forgot a '-'\n");
        return -1;
    }
    else {
        for (int i = 1; i < strlen(sig2); i++) {
            *(sig3+i-1) = *(sig2+i);//on copie sans le tiret du debut
        }
    }
    int sig4 = convert_str_to_int(sig3);
    if (sig4 == INT_MIN) {
        free(pid2);
        free(sig2);
        free(sig3);
        fprintf(stderr,"wrong command\n");
        return -2;
    }
    int pid3 = convert_str_to_int(pid2);
    if (pid3 == INT_MIN) {
        free(pid2);
        free(sig2);
        free(sig3);
        fprintf(stderr,"wrong command\n");
        return -2;
    }
    if (pid3 < 40) {//car maximum de 40 jobs simultanément
    //et de toute maniere ce programme n'a pas les droits pour envoyer des signaux aux jobs < 40
        for (int i = 0; i < nbJobs; i++) {
            if (l_jobs[i].nJob == pid3) {
                if (*(pid2) == '%') {
                    pid3 = -(l_jobs[i].pid);
                }
                else {
                    pid3 = l_jobs[i].pid;//si on voit que le pid donné n'est pas un pid
                    //mais un numéro de jobs alors on met le pid du numéro de job concerné
                }
            }
        }
    }

    //printf("pid=%d,sig=%d\n",pid3,sig4);
    bool tmp = inspectAllSons(pid3,0,false,true);
    int returnValue;
    if (pid3 < 0) {
        inspectAllSons(-pid3,sig4,false,false);//respectivement les arguments print et hasStopped pour les deux booléens
        returnValue = kill(-pid3,sig4);//on kill aussi le pid apres avoir tuer tous ses fils
    }
    else {
        returnValue = kill(pid3,sig4);
    }

    if (returnValue == 0 && (sig4 == 9 || sig4 == 15 || sig4 == 19)) {
        int i = 0;
        while (l_jobs[i].pid != pid3) {
            i++;
        }
        free(l_jobs[i].state);
        char* state = malloc(sizeof(char)*11);
        strcpy(state,"Killed");
        l_jobs[i].state = state;
        removeJob(i);
        nbJobs--;
    }
    else if (returnValue == 0 && sig4 == 18 && tmp){
        int i = 0;
        while (l_jobs[i].pid != pid3) {
            i++;
        }
        free(l_jobs[i].state);
        char* state = malloc(sizeof(char)*11);
        strcpy(state,"Running");
        l_jobs[i].state = state;
        printf("%s\n",l_jobs[i].state);
    }
    free(pid2);
    free(sig2);
    free(sig3);
    return returnValue;
}

// int bg(int job_num) {
//     if (job_num > nbJobs) {
//         fprintf(stderr, "Could not run bg, process not found.\n");
//         return 1;
//     }


//     Job *job = & l_jobs[job_num];
//     printf("ground b4 = %s\n", job->ground);
//     if(strcmp(job->ground,"Background")==0){
//         fprintf(stderr, "Process already running in Background.\n");
//         return 1;
//     }
    

//     if (kill(job->pid, SIGCONT) < 0) {
//         perror("Could not run bg ");
//         return 1;
//     }
    
//     job->ground = "Background";

//     pid_t pid = fork();
//     if (pid == 0){

//         execute_command(getCommand(job->command_name+'&'),NULL);

//         exit(0);
//     }
//     else{
//         int status;

//         printf("[%d] %s %d running in Background\n", nbJobs, job->command_name, job->pid);
//         waitpid(pid,&status,0);
            
        
//     }

//     removeJob(job_num);
//     return 0;
// }



// int fg(int job_num) {
//     if (job_num > nbJobs) {
//         fprintf(stderr, "Could not run bg, process not found.\n");
//         return 1;
//     }

//     Job *job = & l_jobs[job_num];
//     if(strcmp(job->ground,"Foreground")==0){
//         fprintf(stderr, "Process already running in Foreground.\n");
//         return 1;
//     }
//     //l_jobs[0].ground = "Background"; //shell is in background while process end his execution

//     job->ground = "Foreground";

//     printf("%s\n", job->command_name);
//     //callRightCommand(getCommand(job->command_name));
//     //int status;
//      //waitpid(job->pid, &status,WUNTRACED);//

//     //l_jobs[0].ground = "Foreground";
//     printf("[%d]%s %d running in Foreground\n", nbJobs, job->command_name, job->pid);

//     execute_command(getCommand(job->command_name),NULL);


//     removeJob(job_num);

//     return 0;
// }

void check_sons_state() {
    int status;
    int i = 0;
    while(i < nbJobs) {
        int tmp = inspectAllSons(l_jobs[i].pid,0,false,true);
        if (waitpid(l_jobs[i].pid,&status,WNOHANG) != 0 || (tmp && nTimesPrintStop == 0)) {
            if ((tmp && nTimesPrintStop == 0) || WIFSTOPPED(status)) {
                if (tmp) nTimesPrintStop++;
                free(l_jobs[i].state);
                char* state = malloc(sizeof(char)*8);
                strcpy(state,"Stopped");
                l_jobs[i].state = state;
                print_job(l_jobs[i]);
                i++;
            }
            else if (WIFEXITED(status)) {
                kill(l_jobs[i].pid,SIGKILL);
                free(l_jobs[i].state);
                char* state = malloc(sizeof(char)*5);
                strcpy(state,"Done");
                l_jobs[i].state = state;

                print_job(l_jobs[i]);
                removeJob(i);
                nbJobs--;
            }
            else if (WIFCONTINUED(status)) {
                nTimesPrintStop = 0;
                free(l_jobs[i].state);
                char* state = malloc(sizeof(char)*10);
                strcpy(state,"Continued");
                l_jobs[i].state = state;
                print_job(l_jobs[i]);
                i++;
            }
            else if (WIFSIGNALED(status)) {
                free(l_jobs[i].state);
                char* state = malloc(sizeof(char)*7);
                strcpy(state,"Killed");
                l_jobs[i].state = state;
                print_job(l_jobs[i]);
                removeJob(i);
                nbJobs--;
            }
            else {
                i++;
            }
        }
        else {
            i++;
        }
    }
}