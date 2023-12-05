#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_command(char* strCommand, char** argsCommand, size_t* argsCapacity) {
    argsCommand[0] = strtok(strCommand, " ");
    int index = 1;
    while (1) { // Boucle sur les mots d'une commande.
        if (index == *argsCapacity-1) { // Si une commande contient au moins wordsCapacity mots, (le
        // dernier pointeur doit être laissé à NULL pour external_command): allocation supplémentaire.
            *argsCapacity = (*argsCapacity)*2;
            argsCommand = realloc(argsCommand, (*argsCapacity) * sizeof(char*));
        }
        argsCommand[index] = strtok(NULL, " ");
        if (argsCommand[index] == NULL) break;
        ++index;
    }
    return index;
}

// Nettoie les buffers de mots d'une commande.
void reset(char** args, size_t* len) {
    for (int i = 0; i < *len; i++) {
        args[i] = NULL;
    }
}