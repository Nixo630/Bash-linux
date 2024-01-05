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
        // Initialisation membres de la structure command.
        pipeline[index] = malloc(sizeof(Command));
        pipeline[index] -> strComm = NULL;
        pipeline[index] -> argsComm = NULL;
        pipeline[index] -> nbArgs = 0;
        pipeline[index] -> in_redir = NULL;
        pipeline[index] -> out_redir = NULL;
        pipeline[index] -> err_redir = NULL;
        pipeline[index] -> substitutions = NULL;
        pipeline[index] -> nbSubstitutions = 0;
        pipeline[index] -> input = NULL;
        pipeline[index] -> background = false;
        // Remplissage du champ strComm de la structure commande.
        pipeline[index] -> strComm = malloc(MAX_NB_ARGS * 10);
        strcpy(pipeline[index] -> strComm, firstCommand);
        free(firstCommand);
        // Découpage des arguments et des redirections de la commande.
        if (parse_command(pipeline[index]) == -1 || parse_redirections(pipeline[index]) == -1) {
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

/* Prend en argument une structure Command initialisée avec seulement une string de commande, et parse
ses arguments (strings) et ses substitutions (Command), en les mettant respectivement dans les champs
argsComm et substitutions de la structure Command. */
int parse_command(Command* command) {
    int returnValue = 0;
    char* substitutions_tmp[MAX_NB_SUBSTITUTIONS]; // Stocke temporairement les strings des substitutions.
    char* cpy = malloc(MAX_NB_ARGS * 10); /* On opère le parsing sur une copie de la string
    de commande originelle */
    strcpy(cpy, command -> strComm);
    // char* tmp = malloc(50 * sizeof(char)); // Stocke temporairement les tokens.
    char* inside_parentheses = malloc(MAX_NB_ARGS * 10); // Stocke la commande qui constitue une substitution.
    // Initialisation tableau argsComm.
    command -> argsComm = malloc(MAX_NB_ARGS * sizeof(char*));
    for (int i = 0; i < MAX_NB_ARGS; ++i) {
        command -> argsComm[i] = NULL;
    }
    unsigned index = 0; // Nombre de tokens.
    command -> argsComm[index] = malloc(MAX_SIZE_ARG);
    char* tmp = strtok(cpy, " ");
    strcpy(command -> argsComm[0], tmp);
    index++;
    while (1) { // Boucle sur les mots de la commande.
        if (index == (MAX_NB_ARGS)-1) { // Erreur si la commande contient trop de mots.
            fprintf(stderr,"bash : %s: too many arguments\n", command -> argsComm[0]);
            returnValue = -1;
            break;
        }
        tmp = strtok(NULL, " ");
        if (tmp == NULL) break;
        command -> argsComm[index] = malloc(MAX_SIZE_ARG);
        if (strcmp(tmp, "<(") == 0) { // Si on est au début d'une substitution.
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
    // Libération buffers.
    //free(tmp);
    free(cpy);
    free(inside_parentheses);
    // On crée des commandes à partir des substitutions récupérées. On le fait maintenant, à la fin de la
    // fonction, et pas lorsque la string de commande est découpée, pour ne pas interrompre strtok.
    if (command -> nbSubstitutions > 0) {
        command -> substitutions = malloc(MAX_NB_SUBSTITUTIONS * sizeof(Command));
        for (int i = 0; i < command -> nbSubstitutions; ++i) {
            command -> substitutions[i] = getCommand(substitutions_tmp[i]);
            free(substitutions_tmp[i]);
            if (command -> substitutions[i] == NULL) {returnValue = -1;break;}
        }
    }
    return returnValue;
}

/* Découpe les éventuels symboles de redirection et les fichiers sur lequels rediriger, et l'éventuel
symbole '&', et remplit en conséquence les champs correspondant dans la structure Command passée en
argument. */
int parse_redirections(Command* command) {
    unsigned returnValue = 0;
    if (command -> argsComm == NULL) return -1;
    unsigned args_removed = 0;
    for (unsigned i = 0; i < command -> nbArgs; ++i) {
        if (command -> argsComm[i] == NULL) break;
        if (!strcmp(command -> argsComm[i], "&")) {
            if (i != command -> nbArgs - args_removed - 1) { // Si '&' n'est pas le dernier mot de la commande.
                fprintf(stderr,"Commande %s: Symbole '&' mal placé.\n", command -> argsComm[0]);
                returnValue = -1;
                break;
            } else {
                command -> background = true;
                command -> argsComm[i] = NULL;
                free(command -> argsComm[i]);
                args_removed++;
                break;
            }
        }
        int redirection_value = is_redirection_symbol(command -> argsComm[i]);
        if (redirection_value != -1) {
            if (command -> argsComm[i+1] == NULL || !strcmp(command -> argsComm[i+1], "fifo") ||
                is_redirection_symbol(command -> argsComm[i+1]) != -1) {
                fprintf(stderr,"Commande %s: Symbole de redirection non suivi d'un nom de fichier.\n", command -> argsComm[0]);
                returnValue = -1;
                break;
            }
            switch (redirection_value) {
                case 0: // Cas d'un symbole de redirection d'entrée.
                    if (command -> in_redir != NULL) {
                        fprintf(stderr,"Commande %s: Trop de redirections de l'entrée.\n", command -> argsComm[0]);
                        return -1;
                    }
                    command -> in_redir = malloc(2 * sizeof(char*));
                    command -> in_redir[0] = malloc(5);
                    strcpy(command -> in_redir[0], command -> argsComm[i]);
                    command -> in_redir[1] = malloc(MAX_SIZE_ARG);
                    strcpy(command -> in_redir[1], command -> argsComm[i+1]);
                    break;
                case 1: // Cas d'un symbole de redirection de sortie.
                    if (command -> out_redir != NULL) {
                        fprintf(stderr,"Commande %s: Trop de redirections de la sortie.\n", command -> argsComm[0]);
                        returnValue = -1;
                        break;
                    }
                    command -> out_redir = malloc(2 * sizeof(char*));
                    command -> out_redir[0] = malloc(5);
                    strcpy(command -> out_redir[0], command -> argsComm[i]);
                    command -> out_redir[1] = malloc(MAX_SIZE_ARG);
                    strcpy(command -> out_redir[1], command -> argsComm[i+1]);
                    break;
                case 2: // Cas d'un symbole de redirection de sortie erreur.
                    if (command -> err_redir != NULL) {
                        fprintf(stderr,"Commande %s: Trop de redirections de la sortie erreur.\n", command -> argsComm[0]);
                        returnValue = -1;
                        break;
                    }
                    command -> err_redir = malloc(2 * sizeof(char*));
                    command -> err_redir[0] = malloc(5);
                    strcpy(command -> err_redir[0], command -> argsComm[i]);
                    command -> err_redir[1] = malloc(MAX_SIZE_ARG);
                    strcpy(command -> err_redir[1], command -> argsComm[i+1]);
                    break;
            }
            // Décalage du reste des arguments de deux cases vers la gauche.
            for (unsigned j = i+2; j < command -> nbArgs - args_removed; ++j) {
                strcpy(command -> argsComm[j-2], command -> argsComm[j]);
            }
            // Mise à NULL et libération des deux dernières cases d'arguments non-nulles.
            for (unsigned j = 0; j < 2; ++j) {
                command -> argsComm[command -> nbArgs - args_removed-1] = NULL;
                free(command -> argsComm[command -> nbArgs - args_removed-1]);
                args_removed++;
            }
            i--;
        }
    }
    command -> nbArgs -= args_removed;
    return returnValue;
}

/* Suivant si la string passée en argument est un symbole de redirection d'entrée, de sortie, de sortie
erreur ou n'est pas un symbole de redirection, la fonction renvoie respectivement la valeur 0,1,2 ou -1.*/
int is_redirection_symbol(char* string) {
    if (!strcmp(string, "<")) return 0;
    else if (!strcmp(string, ">") || !strcmp(string, ">|") || !strcmp(string, ">>")) return 1;
    else if (!strcmp(string, "2>") || !strcmp(string, "2>|") || !strcmp(string, "2>>")) return 2;
    else return -1;
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
    // Affichage du fait que la commande doit être exécutée en arrière-plan ou non.
    printf("Background:%s\n", command -> background ? "yes" : "no");
    // Affichage entrée, sortie et sortie erreur standard de la commande.
    if (command -> in_redir != NULL) printf("Entrée: %s %s\n", command -> in_redir[0], command -> in_redir[1]);
    if (command -> out_redir != NULL) printf("Sortie: %s %s\n", command -> out_redir[0], command -> out_redir[1]);
    if (command -> err_redir != NULL) printf("Sortie erreur: %s %s\n", command -> err_redir[0], command -> err_redir[1]);
    // Affichage input de la commande.
    printf("Input: %s\n", command -> input == NULL ? "aucune" : command -> argsComm[0]);
     // Affichage substitutions qu'utilise la commande.
    if (command -> nbSubstitutions > 0) {
        printf("nbSubstitutions: %i\n", command -> nbSubstitutions);
        for (int i = 0; i < command -> nbSubstitutions-1; ++i) {
            printf("%s, ", command -> substitutions[i] -> argsComm[0]);
        }
        printf("%s\n", command -> substitutions[command -> nbSubstitutions-1] -> argsComm[0]);
    }
}

// Libère toute la mémoire allouée pour une structure Command.
void free_command(Command* command) {
    // Libération string de commande.
    free(command -> strComm);
    // Libération arguments de la commande.
    for (int i = 0; i < command -> nbArgs; ++i) {
        free(command -> argsComm[i]);
    }
    free(command -> argsComm);
    // Libération redirection entrée de la commande.
    if (command -> in_redir != NULL) {
        if (command -> in_redir[0] != NULL) free(command -> in_redir[0]);
        if (command -> in_redir[1] != NULL) free(command -> in_redir[1]);
        free(command -> in_redir);
    }
    // Libération redirection sortie de la commande.
    if (command -> out_redir != NULL) {
        if (command -> out_redir[0] != NULL) free(command -> out_redir[0]);
        if (command -> out_redir[1] != NULL) free(command -> out_redir[1]);
        free(command -> out_redir);
    }
    // Libération redirection sortie erreur de la commande.
    if (command -> err_redir != NULL) {
        if (command -> err_redir[0] != NULL) free(command -> err_redir[0]);
        if (command -> err_redir[1] != NULL) free(command -> err_redir[1]);
        free(command -> err_redir);
    }
    // Libération éventuelle input.
    if (command -> input != NULL) {
        free_command(command -> input);
    }
    // Libération éventuelles substitutions.
    if (command -> substitutions != NULL) {
        for (unsigned i = 0; i < command -> nbSubstitutions; ++i) {
            free_command(command -> substitutions[i]);
        }
        free(command -> substitutions);
    }
    free(command);
}