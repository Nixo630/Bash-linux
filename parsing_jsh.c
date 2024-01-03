#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parsing_jsh.h"


/* Prend en argument une string correspondant à une commande ou à une pipeline et renvoie une structure
Command associée, avec dans ses champs les structures Command associées à son input et aux substitutions
quOB'elle utilise. */
Command* getCommand(char* input) {
    printf("test1\n");
    Command* pipeline[MAX_LENGTH_PIPELINE]; // Tableau pour stocker temporairement les commandes de la pipeline.
    unsigned index = 0;
    char* firstCommand = first_command(input);
    printf("test8,fc:%s\n", firstCommand);
    do { // Pour chaque commande de la pipeline.
        pipeline[index] = malloc(sizeof(Command));
	    printf("test8bis\n");
        pipeline[index] -> strComm = malloc(MAX_NB_ARGS * 10);
        // strcpy(pipeline[index] -> strComm, "");
        printf("test9\n");
        strcpy(pipeline[index] -> strComm, firstCommand);
        printf("test10,strComm:%s\n", pipeline[index] -> strComm);
        free(firstCommand);
        printf("test11\n");
        parse_command(pipeline[index]);
        index++;
        printf("test21\n");
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
  printf("test2, string input:%s\n", input);
    unsigned len_input = strlen(input);
    printf("test2,%i\n", len_input);
    if (len_input == 0) return NULL;
    unsigned len_command = len_input; // Par défaut, on considère que l'input est constituée d'une seule commande.
    char* strComm = malloc(MAX_NB_ARGS * 10);
    char* first_bar_adress = (char*) NULL;
    // Recherche de la première occurence du caractère | qui ne soit pas à l'intérieur d'une substitution.
    unsigned nb_parentheses_ouvrantes = 0;
    unsigned nb_parentheses_fermantes = 0;
    for (unsigned i = 0; i < len_command; ++i) {
        if ((input[i] == '|') && (nb_parentheses_fermantes == nb_parentheses_ouvrantes)) {
            printf("test3\n");
            first_bar_adress = input+i;
            break;
        }
        if (input[i] == '(') nb_parentheses_ouvrantes++;
        if (input[i] == ')') nb_parentheses_fermantes++;
    }
    if (first_bar_adress != NULL) {
        printf("test4,first_bar_adress:%s.\n", first_bar_adress);
        len_command = len_input - strlen(first_bar_adress);
        printf("test5\n");
        strncpy(strComm, input, len_command); // copie de la première commande dans strComm. 
        strComm[len_command] = '\0';
        memmove(input, input+len_command+1,strlen(first_bar_adress));
    } else {
      strcpy(strComm, input);
      memset(input,0,len_input);
    }
    //strncpy(strComm, input, len_command); // copie de la première commande dans strComm.
    //strComm[len_command] = '\0';
    //memmove(input, input+len_command+1,len_command+1); // troncage de l'input.
    // au niveau de la fin de la première commande.
    return strComm;
}

/* Prend en argument une structure Command initialisée avec seulement une string de commande, et parse
ses arguments (strings) et ses substitutions (Command), en les mettant respectivement dans les champs
argsComm et substitutions de la structure Command. */
void parse_command(Command* command) {
    char* substitutions_tmp[MAX_NB_SUBSTITUTIONS]; // Stocke temporairement les strings des substitutions.
    printf("test12\n");
    char* cpy = malloc(MAX_NB_ARGS * 10); /* On opère le parsing sur une copie de la string
    de commande originelle */
    strcpy(cpy, command -> strComm);
    printf("test13,%s\n", cpy);
    char* tmp = malloc(50 * sizeof(char)); // Stocke temporairement les tokens.
    printf("test13.1\n");
    char* inside_parentheses = malloc(MAX_NB_ARGS * 10); // Stocke la commande qui constitue une substitution.
    printf("test13.1\n");
    strcpy(inside_parentheses, "");
    printf("test13.1\n");
    command -> argsComm = malloc(MAX_NB_ARGS);
    unsigned index = 0; // Nombre de tokens.
    command -> argsComm[index] = malloc(50);
    printf("test14\n");
    strcpy(command -> argsComm[0], strtok(cpy, " "));
    printf("test15\n");
    index++;
    while (1) { // Boucle sur les mots de la commande.
        if (index == (MAX_NB_ARGS)-1) { // Erreur si la commande contient trop de mots.
            fprintf(stderr,"bash : %s: too many arguments\n", command -> argsComm[0]);
            exit(-1);
        }
        printf("test16\n");
        tmp = strtok(NULL, " ");
        if (tmp == NULL) break;
        command -> argsComm[index] = malloc(50);
        if (strcmp(tmp, "<(") == 0) {
            unsigned nb_parentheses_ouvrantes = 1;
            unsigned nb_parentheses_fermantes = 0;
            while(1) {
                printf("test17\n");
                tmp = strtok(NULL, " ");
                if (tmp == NULL) {
                    fprintf(stderr,"Commande %s: Parenthèses mal formées.\n", command -> argsComm[0]);
                    exit(-1);
                }
                if (strcmp(tmp, "<(") == 0) nb_parentheses_ouvrantes++;
                if (strcmp(tmp, ")") == 0) nb_parentheses_fermantes++;
                printf("%s,%i,%i\n",tmp, nb_parentheses_ouvrantes,nb_parentheses_fermantes);
                if (nb_parentheses_ouvrantes != nb_parentheses_fermantes) {
                    strcat(inside_parentheses, tmp);
                    strcat(inside_parentheses, " ");
		            printf("inside:%s\n", inside_parentheses);
                } else {
                    break;
                }
            }
            printf("test18\n");
            strcpy(command -> argsComm[index], "fifo");
            substitutions_tmp[command -> nbSubstitutions] = malloc(MAX_NB_ARGS * 10);
            strcpy(substitutions_tmp[command -> nbSubstitutions], inside_parentheses);
            printf("test19\n");
            command -> nbSubstitutions++;
        } else strcpy(command -> argsComm[index],tmp);
        ++index;
    }
    command -> nbArgs = index;
    free(tmp);
    free(cpy);
    free(inside_parentheses);
    printf("test20\n");
    // On crée des commandes à partir des substitutions récupérées. On le fait maintenant, à la fin de la
    // fonction, pour ne pas interrompre strtok.
    for (int i = 0; i < command -> nbSubstitutions; ++i) {
        command -> substitutions[i] = getCommand(substitutions_tmp[i]);
        free(substitutions_tmp[i]);
    }
}

/* Affiche une commande, son input (commande qui la précède dans une pipeline), et les substitutions qu'elle
utilise */
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
