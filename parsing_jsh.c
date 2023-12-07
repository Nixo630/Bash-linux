#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Prend une commande (string), un tableau de string et la taille de ce tableau en arguments.
Stocke les différents arguments de la commande dans le tableau, en agrandissant sa taille
si nécessaire. Retourne le nombre de mots stockés dans le tableau. */
int parse_command(char* strCommand, char** argsCommand, size_t* argsCapacity) {
    argsCommand[0] = strtok(strCommand, " ");
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