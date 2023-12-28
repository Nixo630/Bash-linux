#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parsing_jsh.h"

char* first_command(char* input) {
    int len_input = strlen(input);
    if (len_input == 0) return NULL;
    int len_command = len_input; // Par défaut, on considère que l'input est constituée d'une seule commande.
    char* strComm = malloc(sizeof(input));
    char* tmp = NULL;
    tmp = strstr(input, "|");
    if (tmp != NULL) {
        len_command = len_input - strlen(tmp);
    } else {
        tmp = strstr(input, "&");
        if (tmp != NULL) {
            len_command = strlen(input) - strlen(tmp);
        }
    }
    strncpy(strComm, input, len_command); // copie de la première commande dans strComm.
    memmove(input, input+len_command+1, (len_input - len_command)+1); // troncage de l'input.
    // au niveau de la fin de la première commande.
    return strComm;
}

// parse_redirections () {

// }

/* Prend une commande (string), un tableau de string et la taille de ce tableau en arguments.
Stocke les différents arguments de la commande dans le tableau, en agrandissant sa taille
si nécessaire. Retourne le nombre de mots stockés dans le tableau. */
int parse_command(char* strComm, char** argsCommand, size_t* argsCapacity) {
    argsCommand[0] = strtok(strComm, " ");
    unsigned index = 1;
    while (1) { // Boucle sur les mots de la commande.
        if (index == (*argsCapacity)-1) { /* Si la commande contient au moins argsCapacity mots, (le dernier
        pointeur doit être laissé à NULL pour un éventuel appel à execvp): allocation supplémentaire. */
            (*argsCapacity) *= 2;
            argsCommand = realloc(argsCommand, (*argsCapacity) * sizeof(char*));
        }
        argsCommand[index] = strtok(NULL, " ");
        if (argsCommand[index] == NULL) break;
        ++index;
    }
    return index;
}

// Nettoie le tableau stockant les arguments d'une commande.
void reset(char** args, size_t* len) {
    for (int i = 0; i < *len; i++) {
        args[i] = NULL;
    }
}