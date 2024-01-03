#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parsing_jsh.h"

// ls | pwd | cmd <() <() | ls
// cmd <() <( cmd2 <() <() | ls) | ls | pwd
// ./my_cat <( ./my_cat <( ./my_cat <( ./my_cat <( echo 123 | cat ) | tail ) ) )
// char** parse_substitutions(char* input) {

// }

Command* getCommand(char* input) {
    printf("test2\n");
    Command* pipeline[MAX_LENGTH_PIPELINE]; // Tableau pour stocker temporairement les commandes de la pipeline.
    unsigned index = 0;
    char* firstCommand = (char*) NULL;
    firstCommand = first_command(input);
    do { // Pour chaque commande de la pipeline.
        pipeline[index] = malloc(sizeof(Command));
        printf("test16,%i\n",pipeline[index] -> nbArgs);
        printf("test17,%s\n", firstCommand);
        pipeline[index] -> strComm = malloc(100);
        strcpy(pipeline[index] -> strComm, firstCommand);
        free(firstCommand);
        // memmove(pipeline[index] -> strComm, firstCommand, strlen(firstCommand)+1);
        printf("test17\n");
        parse_command(pipeline[index]);
        index++;
        firstCommand = first_command(input);
    } while (firstCommand != NULL);
    // Liaison des commandes de la pipeline entre elles via leur champ input.
    printf("test15\n");
    for (int i = 1; i < index; ++i) {
        pipeline[i] -> input = pipeline[i-1];
    }
    return pipeline[index-1];
}

// Renvoie la première commande de la ligne de commande (éventuellement une pipeline) passée en argument.
char* first_command(char* input) {
    char* s = "stringtest";
    printf("test3,%c\n", s[0]);
    unsigned len_input = strlen(input);
    if (len_input == 0) return NULL;
    unsigned len_command = len_input; // Par défaut, on considère que l'input est constituée d'une seule commande.
    printf("test7,%s,%c\n", input, input[0]);
    char* strComm = malloc(sizeof(input));
    char* first_bar_adress = malloc(sizeof(input));
    // Recherche de la première occurence du caractère | qui ne soit pas à l'intérieur d'une substitution.
    unsigned nb_parentheses_ouvrantes = 0;
    unsigned nb_parentheses_fermantes = 0;
    printf("test8,%i\n", len_input);
    for (unsigned i = 0; i < len_command; ++i) {
        printf("test10\n");
        if ((input[i] == '|') && (nb_parentheses_fermantes == nb_parentheses_ouvrantes)) {
            printf("test5\n");
            strcpy(first_bar_adress,input+i);
            printf("test6\n");
            break;
        }
        if (input[i] == '(') nb_parentheses_ouvrantes++;
        if (input[i] == ')') nb_parentheses_fermantes++;
    }
    if (first_bar_adress != NULL) {
        printf("test11\n");
        len_command = len_input - strlen(first_bar_adress);
    }
    printf("test12\n");
    strncpy(strComm, input, len_command); // copie de la première commande dans strComm.
    printf("test13\n");
    memmove(input, input+len_command+1, (len_input - len_command)+1); // troncage de l'input.
    printf("test14\n");
    // au niveau de la fin de la première commande.
    return strComm;
}

// parse_redirections () {

// }

/* Prend une commande en argument.
Stocke les différents arguments de la commande dans le champ argsComm. */
void parse_command(Command* command) {
    printf("test4\n");
    char* cpy = malloc(sizeof(command -> strComm)); // On opère le parsing sur une copie de la string
    strcpy(cpy, command -> strComm); // de commande originelle.
    char* tmp = malloc(100);
    printf("test41\n");
    command -> argsComm = malloc(MAX_NB_ARGS * sizeof(char*));
    unsigned index = 0; // Nombre d'arguments.
    command -> argsComm[index] = malloc(30);
    // command -> argsComm[0] = strtok(cpy, " ");
    // printf("argsComm[0]:%s\n", command -> argsComm[0]);
    strcpy(command -> argsComm[0], strtok(cpy, " "));
    index++;
    bool found_substitution = false;
    while (1) { // Boucle sur les mots de la commande.
        if (index == (MAX_NB_ARGS)-1) { // Erreur si la commande contient trop de mots.
            fprintf(stderr,"bash : %s: too many arguments\n", command -> argsComm[0]);
        }
        if (found_substitution) {
            // TODO comment passer à strtok la suite de la commande.
            printf("COPY:%s\n", cpy);
            tmp = strtok(cpy, " ");
        } else tmp = strtok(NULL, " ");
        printf("test22\n");
        if (tmp == NULL) break;
        command -> argsComm[index] = malloc(30);
        printf("test23\n");
        if (strcmp(tmp, "<(") == 0) {
            char* inside_parentheses = malloc(100);
            unsigned nb_parentheses_ouvrantes = 1;
            unsigned nb_parentheses_fermantes = 0;
            while(1) {
                tmp = strtok(NULL, " ");
                if (strcmp(tmp, "<(") == 0) nb_parentheses_ouvrantes++;
                if (strcmp(tmp, ")") == 0) nb_parentheses_fermantes++;
                if (nb_parentheses_ouvrantes != nb_parentheses_fermantes) {
                    strcat(inside_parentheses, tmp);
                    strcat(inside_parentheses, " ");
                } else break;
            }
            printf("test55\n");
            strcpy(command -> argsComm[index], "fifo");
            command -> substitutions[command -> nbSubstitutions] = getCommand(inside_parentheses);
            found_substitution = true;
            printf("test15suite?\n");
            command -> nbSubstitutions++;
            free(inside_parentheses);
        } else {
            strcpy(command -> argsComm[index],tmp);
            found_substitution = false;
        }
        printf("test33\n");
        ++index;
    }
    printf("test44\n");
    command -> nbArgs = index;
    free(tmp);
    free(cpy);

    printf("argsComm[0]:%s\n", command -> argsComm[0]);
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