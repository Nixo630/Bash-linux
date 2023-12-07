#define STARTING_ARGS_CAPACITY 32;

// Fonctions
int parse_command(char* strCommand, char** argsCommand, size_t* argsCapacity);
void reset(char** args, size_t* argsCapacity);