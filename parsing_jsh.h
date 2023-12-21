#include <stddef.h>
#define STARTING_ARGS_CAPACITY 32;

// Fonctions
char* first_command(char* input);
int parse_command(char* strComm, char** argsCommand, size_t* argsCapacity);
void reset(char** args, size_t* argsCapacity);