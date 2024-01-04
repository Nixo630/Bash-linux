#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parsing_jsh.h"


/* Prend en argument une string correspondant à une commande ou à une pipeline et renvoie une structure
Command associée, avec dans ses champs les structures Command associées à son input et aux substitutions
quOB'elle utilise. */
Command* getCommand(char* input) {
    Command* pipeline[MAX_LENGTH_PIPELINE]; // Tableau pour stocker temporairement les commandes de la pipeline.
    unsigned index = 0;
    char* firstCommand = first_command(input);
    bool error = false;
    do { // Pour chaque commande de la pipeline.
        pipeline[index] = malloc(sizeof(Command));
        pipeline[index] -> strComm = malloc(MAX_NB_ARGS * 10);
        strcpy(pipeline[index] -> strComm, firstCommand);
        free(firstCommand);
        if (parse_redirections(pipeline[index]) == -1 || parse_command(pipeline[index]) == -1) {
            error = true;
            break;
        }
        index++;
        firstCommand = first_command(input);
    } while (firstCommand != NULL);
    // Liaison des commandes de la pipeline entre elles via leur champ input.
    for (int i = 1; i < index; ++i) {
        pipeline[i] -> input = pipeline[i-1];
    }
    if (error) {
        free_command(pipeline[index]);
        return NULL;
    }
    return pipeline[index-1];
}

// Renvoie la première commande de la ligne de commande (éventuellement une pipeline) passée en argument.
char* first_command(char* input) {
    unsigned len_input = strlen(input);
    if (len_input == 0) return NULL;
    unsigned len_command = len_input; // Par défaut, on considère que l'input est constituée d'une seule commande.
    char* strComm = malloc(MAX_NB_ARGS * 10);
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
        strncpy(strComm, input, len_command); // copie de la première commande dans strComm. 
        strComm[len_command] = '\0';
        memmove(input, input+len_command+1,strlen(first_bar_adress)); // troncage de l'input
        // au niveau de la fin de la première commande.
    } else {
      strcpy(strComm, input);
      memset(input,0,len_input);
    }
    return strComm;
}

/* Découpe les éventuels symboles de redirection et les fichiers sur lequels rediriger, et l'éventuel
symbole '&', et remplit en conséquence les champs correspondant dans la structure Command passée en
argument. */
int parse_redirections(Command* command) {
    char* cpy = malloc(MAX_NB_ARGS * 10); /* On opère le parsing sur une copie de la string
    de commande originelle */
    strcpy(cpy, command -> strComm);
    char* new_strComm = malloc(MAX_NB_ARGS * 10);
    char* tmp = malloc(50 * sizeof(char)); // Stocke temporairement les tokens.
    bool waiting_for_file_in = false, waiting_for_file_out = false, waiting_for_file_err = false;
    bool waiting_for_end = false;
    tmp = strtok(cpy, " ");
    strcat(new_strComm, tmp);
    strcat(new_strComm, " ");
    int returnValue = 0;
    while(1) {
        tmp = strtok(NULL, " ");
        if (tmp == NULL) break;
        else if (waiting_for_end) {
            fprintf(stderr,"bash : symbole '&' mal placé.\n");
            returnValue = -1;
            break;
        } else if (waiting_for_file_in) {
            command -> in_redir[1] = malloc(50);
            strcpy(command -> in_redir[1], tmp);
            waiting_for_file_in = false;
        } else if (waiting_for_file_out) {
            command -> out_redir[1] = malloc(50);
            strcpy(command -> out_redir[1], tmp);
            waiting_for_file_out = false;
        } else if (waiting_for_file_err) {
            command -> err_redir[1] = malloc(50);
            strcpy(command -> err_redir[1], tmp);
            waiting_for_file_err = false;
        } else if (!strcmp(tmp,"<")) {
            if (command -> in_redir != NULL) {
                fprintf(stderr,"bash : trop de redirection de l'entrée.\n");
                returnValue = -1;
                break;
            }
            command -> in_redir = malloc(2 * sizeof(char*));
            command -> in_redir[0] = malloc(10);
            strcpy(command -> in_redir[0], tmp);
            waiting_for_file_in = true;
        } else if (!strcmp(tmp, ">") || !strcmp(tmp, ">|") || !strcmp(tmp, ">>")) {
            if (command -> out_redir != NULL) {
                fprintf(stderr,"bash : trop de redirection de la sortie.\n");
                returnValue = -1;
                break;
            }
            command -> out_redir = malloc(2 * sizeof(char*));
            command -> out_redir[0] = malloc(10);
            strcpy(command -> out_redir[0], tmp);
            waiting_for_file_out = true;
        } else if (!strcmp(tmp, "2>") || !strcmp(tmp, "2>|") || !strcmp(tmp, "2>>")) {
            if (command -> err_redir != NULL) {
                fprintf(stderr,"bash : trop de redirection de la sortie erreur.\n");
                returnValue = -1;
                break;
            }
            command -> err_redir = malloc(2 * sizeof(char*));
            command -> err_redir[0] = malloc(10);
            strcpy(command -> err_redir[0], tmp);
            waiting_for_file_err = true;
        } else if (!strcmp(tmp, "&")) {
            command -> background = true;
            waiting_for_end = true;
        } else if (!strcmp(tmp, "<(")) {
            // Si on entre dans une substitution, on saute tous les tokens jusqu'à la fin de celle-ci.
            strcat(new_strComm, tmp);
            strcat(new_strComm, " ");
            unsigned nb_parentheses_ouvrantes = 1, nb_parentheses_fermantes = 0;
            while (1) {
                tmp = strtok(NULL, " ");
                if (tmp == NULL) {
                    fprintf(stderr,"Bash: Parenthèses mal formées.\n");
                    returnValue = -1;
                    break;
                }
                if (strcmp(tmp, "<(") == 0) nb_parentheses_ouvrantes++;
                if (strcmp(tmp, ")") == 0) nb_parentheses_fermantes++;
                strcat(new_strComm, tmp);
                strcat(new_strComm, " ");
                if (nb_parentheses_ouvrantes == nb_parentheses_fermantes) {
                    break;
                }
            }
        } else {
            strcat(new_strComm, tmp);
            strcat(new_strComm, " ");
        }
        if (returnValue == -1) break;
    }
    if (waiting_for_file_in || waiting_for_file_out || waiting_for_file_err) {
        fprintf(stderr,"bash : %s: fichier de redirection manquant.\n", command -> argsComm[0]);
        returnValue = -1;
    }
    strcpy(command -> strComm, new_strComm);
    if (returnValue == -1) tmp = NULL;
    free(tmp);
    free(cpy);
    free(new_strComm);
    return returnValue;
}

/* Prend en argument une structure Command initialisée avec seulement une string de commande, et parse
ses arguments (strings) et ses substitutions (Command), en les mettant respectivement dans les champs
argsComm et substitutions de la structure Command. */
int parse_command(Command* command) {
    int returnValue = 0;
    char* substitutions_tmp[MAX_NB_SUBSTITUTIONS]; // Stocke temporairement les strings des substitutions.
    char* cpy = malloc(MAX_NB_ARGS * 10); /* On opère le parsing sur une copie de la string
    de commande originelle */
    strcpy(cpy, command -> strComm);
    char* tmp = malloc(50 * sizeof(char)); // Stocke temporairement les tokens.
    char* inside_parentheses = malloc(MAX_NB_ARGS * 10); // Stocke la commande qui constitue une substitution.
    command -> argsComm = malloc(MAX_NB_ARGS);
    unsigned index = 0; // Nombre de tokens.
    command -> argsComm[index] = malloc(50);
    strcpy(command -> argsComm[0], strtok(cpy, " "));
    index++;
    while (1) { // Boucle sur les mots de la commande.
        if (index == (MAX_NB_ARGS)-1) { // Erreur si la commande contient trop de mots.
            fprintf(stderr,"bash : %s: too many arguments\n", command -> argsComm[0]);
            returnValue = -1;
            break;
        }
        tmp = strtok(NULL, " ");
        if (tmp == NULL) break;
        command -> argsComm[index] = malloc(50);
        if (strcmp(tmp, "<(") == 0) {
            unsigned nb_parentheses_ouvrantes = 1;
            unsigned nb_parentheses_fermantes = 0;
            while(1) {
                tmp = strtok(NULL, " ");
                if (tmp == NULL) {
                    fprintf(stderr,"Commande %s: Parenthèses mal formées.\n", command -> argsComm[0]);
                    returnValue = -1;
                    break;
                }
                if (strcmp(tmp, "<(") == 0) nb_parentheses_ouvrantes++;
                if (strcmp(tmp, ")") == 0) nb_parentheses_fermantes++;
                if (nb_parentheses_ouvrantes != nb_parentheses_fermantes) {
                    strcat(inside_parentheses, tmp);
                    strcat(inside_parentheses, " ");
                } else {
                    break;
                }
            }
            strcpy(command -> argsComm[index], "fifo");
            substitutions_tmp[command -> nbSubstitutions] = malloc(MAX_NB_ARGS * 10);
            strcpy(substitutions_tmp[command -> nbSubstitutions], inside_parentheses);
            command -> nbSubstitutions++;
        } else strcpy(command -> argsComm[index],tmp);
        ++index;
    }
    command -> nbArgs = index;
    if (returnValue == -1) tmp = NULL;
    free(tmp);
    free(cpy);
    free(inside_parentheses);
    // On crée des commandes à partir des substitutions récupérées. On le fait maintenant, à la fin de la
    // fonction, et pas lorsque la string de commande est découpée, pour ne pas interrompre strtok.
    for (int i = 0; i < command -> nbSubstitutions; ++i) {
        command -> substitutions[i] = getCommand(substitutions_tmp[i]);
        free(substitutions_tmp[i]);
        if (command -> substitutions[i] == NULL) {returnValue = -1;break;}
    }
    return returnValue;
}

/* Affiche une commande, son input (commande qui la précède dans une pipeline), et les substitutions qu'elle
utilise */
void print_command(Command* command) {
    // Affichage string de commande.
    printf("strComm: %s\n", command -> strComm);
    // Affichage arguments de la commande.
    printf("nbArgs:%i\n", command -> nbArgs);
    printf("argsComm: ");
    for (int i = 0; i < command -> nbArgs-1; ++i) {
        printf("%s,", command -> argsComm[i]);
    }
    printf("%s\n",command -> argsComm[command -> nbArgs-1]);
    // Affichage entrée, sortie et sortie erreur standard de la commande.
    if (command -> in_redir != NULL) {
        printf("Entrée: %s %s\n", command -> in_redir[0], command -> in_redir[1]);
    }
    if (command -> out_redir != NULL) {
        printf("Sortie: %s %s\n", command -> out_redir[0], command -> out_redir[1]);
    }
    if (command -> err_redir != NULL) {
        printf("Sortie erreur: %s %s\n", command -> err_redir[0], command -> err_redir[1]);
    }
    // Affichage input de la commande.
    if (command -> input != NULL) {
        printf("INPUT:\n");
        print_command(command -> input);
    } else printf("Input: aucune\n");
     // Affichage substitutions qu'utilise la commande.
    if (command -> nbSubstitutions > 0) {
        printf("SUBSTITUTIONS:\n");
        printf("nbSubstitutions: %i\n", command -> nbSubstitutions);
    }
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
    if (command -> in_redir != NULL) {
        if (command -> in_redir[0] != NULL) free(command -> in_redir[0]);
        if (command -> in_redir[1] != NULL) free(command -> in_redir[1]);
        free(command -> in_redir);
    }
    if (command -> out_redir != NULL) {
        if (command -> out_redir[0] != NULL) free(command -> out_redir[0]);
        if (command -> out_redir[1] != NULL) free(command -> out_redir[1]);
        free(command -> out_redir);
    }
    if (command -> err_redir != NULL) {
        if (command -> err_redir[0] != NULL) free(command -> err_redir[0]);
        if (command -> err_redir[1] != NULL) free(command -> err_redir[1]);
        free(command -> err_redir);
    }
    free(command);
}