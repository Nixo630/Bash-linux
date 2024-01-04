#include <stddef.h>
#define MAX_NB_ARGS 32 // Nombre d'arguments max d'une commande (une substitution compte pour 1).
#define MAX_NB_SUBSTITUTIONS 32 // Nombre de substitutions max pour une commande.
#define MAX_LENGTH_PIPELINE 32 // Nombre de commandes max dans une pipeline.

// Structure
struct Command {
    char* strComm; // La string de commande.
    char** argsComm; /* Les différents arguments de la commande (avec la string "fifo" aux
    emplacements des futures éventuels tubes nommés). */
    unsigned args_to_fill[MAX_NB_SUBSTITUTIONS]; // Les indices des arguments à remplacer par un tube nommé.
    unsigned nbArgs; // Le nombre d'arguments de la commande.
    //int in_out_err[3]; /* Les descripteurs des fichiers sur lesquels rediriger l'entrée, la sortie,
    //et la sortie erreur standards. */
    char** in_redir; // Symbole de redirection de l'entrée et nom du fichier sur lequel rediriger.
    char** out_redir; // Symbole de redirection de la sortie et nom du fichier sur lequel rediriger.
    char** err_redir; // Symbole de redirection de la sortie erreur et nom du fichier sur lequel rediriger.
    struct Command* substitutions[MAX_NB_SUBSTITUTIONS]; // Les éventuelles substitutions.
    unsigned nbSubstitutions; // Le nombre de substitutions qu'utilise la commande.
    struct Command* input; // La commande précédente (dans le contexte d'une pipeline).
    bool background; // booléen indiquant si la commande doit être lancée en arrière-plan ou non.
};
typedef struct Command Command;

// Fonctions
Command* getCommand(char* input);
char* first_command(char* input);
int parse_redirections(Command* command);
int parse_command(Command* command);
void print_command(Command* command);
void free_command(Command* command);