#include <stddef.h>
#define MAX_NB_ARGS 32 // Nombre d'arguments max d'une commande (une substitution compte pour 1).
#define MAX_NB_SUBSTITUTIONS 32 // Nombre de substitutions max pour une commande.
#define MAX_LENGTH_PIPELINE 32 // Nombre max de commandes dans une pipeline.

// Structures

// struct Input;

struct Command {
    char* strComm; // La string de commande.
    char** argsComm; /* Les différents arguments de la commande (avec la string "fifo" aux
    emplacements des futures éventuels tubes nommés). */
    unsigned args_to_fill[MAX_NB_SUBSTITUTIONS]; // Les indices des arguments à remplacer par un tube nommé.
    unsigned nbArgs; // Le nombre d'arguments de la commande.
    int in_out_err[3]; /* Les descripteurs des fichiers sur lesquels rediriger l'entrée, la sortie,
    et la sortie erreur standards. */
    struct Command* substitutions[MAX_NB_SUBSTITUTIONS]; // Les éventuelles substitutions.
    unsigned nbSubstitutions;
    struct Command* input; // La commande précédente (dans le cadre d'une pipeline).
    // bool background; // booléen indiquant si la commande doit être lancée en arrière-plan ou non.
};
typedef struct Command Command;

// struct Input {
//     Command command;
//     char* pathname;
// }
// typedef struct Input Input;

// Fonctions
Command* getCommand(char* input);
char* first_command(char* input);
void parse_command(Command* command);
void print_command(Command* command);
// void reset(char** args, size_t* argsCapacity);