#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parsing_jsh.h"

// ls | pwd | cmd <() <() | ls
// cmd <() <( cmd2 <() <() | ls) | ls | pwd
// ./my_cat <( ./my_cat <( ./my_cat <( ./my_cat <( echo 123 | cat ) | tail ) ) )

Command* getCommand(char* input) {
    Command* pipeline[MAX_LENGTH_PIPELINE]; // Tableau pour stocker temporairement les commandes de la pipeline.
    unsigned index = 0;
    char* firstCommand = first_command(input);
    do { // Pour chaque commande de la pipeline.
        pipeline[index] = malloc(sizeof(Command));
        pipeline[index] -> strComm = malloc(MAX_NB_ARGS * 10);
        strcpy(pipeline[index] -> strComm, firstCommand);
        free(firstCommand);
        parse_command(pipeline[index]);
        index++;
        firstCommand = first_command(input);
    } while (firstCommand != NULL);
    // Liaison des commandes de la pipeline entre elles via leur champ input.
    for (int i = 1; i < index; ++i) {
        pipeline[i] -> input = pipeline[i-1];
    }
    return pipeline[index-1];
}

// Renvoie la première commande de la ligne de commande (éventuellement une pipeline) passée en argument.
char* first_command(char* input) {
    unsigned len_input = strlen(input);
    if (len_input == 0) return NULL;
    unsigned len_command = len_input; // Par défaut, on considère que l'input est constituée d'une seule commande.
    char* strComm = malloc(sizeof(input));
    char* first_bar_adress = (char*) NULL;
    // Recherche de la première occurence du caractère | qui ne soit pas à l'intérieur d'une substitution.
    unsigned nb_parentheses_ouvrantes = 0;
    unsigned nb_parentheses_fermantes = 0;
    for (unsigned i = 0; i < len_command; ++i) {
        if ((input[i] == '|') && (nb_parentheses_fermantes == nb_parentheses_ouvrantes)) {
            first_bar_adress = input+i;
            break;
        }
        if (input[i] == '(') nb_parentheses_ouvrantes++;
        if (input[i] == ')') nb_parentheses_fermantes++;
    }
    if (first_bar_adress != NULL) {
        len_command = len_input - strlen(first_bar_adress);
    }
    strncpy(strComm, input, len_command); // copie de la première commande dans strComm.
    memmove(input, input+len_command+1, (len_input - len_command)+1); // troncage de l'input.
    // au niveau de la fin de la première commande.
    return strComm;
}

/* Prend une commande en argument.
Stocke les différents arguments de la commande dans le champ argsComm. */
void parse_command(Command* command) {
    char* substitutions_tmp[MAX_NB_SUBSTITUTIONS];
    char* cpy = malloc(sizeof(command -> strComm)+1); /* On opère le parsing sur une copie de la string
    de commande originelle */
    strcpy(cpy, command -> strComm);
    char* tmp = malloc(50); // Stocke temporairement les tokens.
    char* inside_parentheses = malloc(100); // Stocke la commande qui constitue une substitution.
    command -> argsComm = malloc(MAX_NB_ARGS * sizeof(char*));
    unsigned index = 0; // Nombre de tokens.
    command -> argsComm[index] = malloc(50);
    strcpy(command -> argsComm[0], strtok(cpy, " "));
    index++;
    while (1) { // Boucle sur les mots de la commande.
        if (index == (MAX_NB_ARGS)-1) { // Erreur si la commande contient trop de mots.
            fprintf(stderr,"bash : %s: too many arguments\n", command -> argsComm[0]);
        }
        tmp = strtok(NULL, " ");
        if (tmp == NULL) break;
        command -> argsComm[index] = malloc(50);
        // if (strcmp(tmp, "<(") == 0) {
        //     unsigned nb_parentheses_ouvrantes = 1;
        //     unsigned nb_parentheses_fermantes = 0;
        //     while(1) {
        //         tmp = strtok(NULL, " ");
        //         if (tmp == NULL) {
        //             fprintf(stderr,"Commande %s: Parenthèses mal formées.\n", command -> argsComm[0]);
        //             break;
        //         }
        //         printf("%s,\n", tmp);
        //         if (strcmp(tmp, "<(") == 0) nb_parentheses_ouvrantes++;
        //         if (strcmp(tmp, ")") == 0) nb_parentheses_fermantes++;
        //         if (nb_parentheses_ouvrantes != nb_parentheses_fermantes) {
        //             strcat(inside_parentheses, tmp);
        //             strcat(inside_parentheses, " ");
        //             printf("test24\n");
        //         } else break;
        //     }
        //     printf("test55\n");
        //     strcpy(command -> argsComm[index], "fifo");
        //     // command -> substitutions[command -> nbSubstitutions] = getCommand(inside_parentheses);
        //     substitutions_tmp[command -> nbSubstitutions] = malloc(100);
        //     strcpy(substitutions_tmp[command -> nbSubstitutions], inside_parentheses);
        //     command -> nbSubstitutions++;
        // } else
        strcpy(command -> argsComm[index],tmp);
        ++index;
    }
    command -> nbArgs = index;
    free(tmp);
    free(cpy);
    free(inside_parentheses);
    // On crée des commandes à partir des substitutions récupérées. On le fait maintenant, à la fin de la
    // fonction, pour ne pas interrompre strtok.
    for (int i = 0; i < command -> nbSubstitutions; ++i) {
        command -> substitutions[i] = getCommand(substitutions_tmp[i]);
        free(substitutions_tmp[i]);
    }
    free(substitutions_tmp);
}

void print_command(Command* command) {
    printf("strComm: %s\n", command -> strComm);
    printf("nbArgs:%i\n", command -> nbArgs);
    printf("argsComm: ");
    for (int i = 0; i < command -> nbArgs-1; ++i) {
        printf("%s,", command -> argsComm[i]);
    }
    printf("%s\n",command -> argsComm[command -> nbArgs-1]);
    printf("nbSubstitutions: %i\n", command -> nbSubstitutions);
    if (command -> input != NULL) {
        printf("INPUT:\n");
        print_command(command -> input);
    }
    if (command -> nbSubstitutions > 0) printf("SUBSTITUTIONS:\n");
    for (int i = 0; i < command -> nbSubstitutions; ++i) {
        print_command(command -> substitutions[i]);
    }
}

// Libère toute la mémoire allouée pour une structure Command.
void free_command(Command* command) {
    free(command -> strComm);
    for (int i = 0; i < command -> nbArgs; ++i) {
        free(command -> argsComm[i]);
    }
    free(command -> argsComm);
    free(command);
}

/* Modifie la string passée en argument pour qu'elle pointe vers le caractère suivant la parenthèse matchante,
et renvoie la string qui était dans les parenthèses. */
// char* find_matching_parenthese(char* buffer) {
//     char* after_first = strstr(buffer, "<(")+2;
//     unsigned len_after_first = strlen(after_first);
//     unsigned nb_parentheses_ouvrantes = 1, nb_parentheses_fermantes = 0;
//     for (int i = 0; i < len_after_first; ++i) {
//         if (after_first[i] == '(') nb_parentheses_ouvrantes++;
//         if (after_first[i] == ')') {
//             nb_parentheses_fermantes++;
//             if (nb_parentheses_ouvrantes == nb_parentheses_fermantes) {
//                 buffer = after_first[i]+1;
//                 after_first[i] = '\0';
//                 break;
//             }
//         }
//     }
//     return after_first;
// }

// Nettoie le tableau stockant les arguments d'une commande.
// void reset(char** args, size_t* len) {
//     for (int i = 0; i < *len; i++) {
//         args[i] = NULL;
//     }
// }