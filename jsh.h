#include <stdbool.h>
// Fonctions de commande
char* pwd();
void cd(char* pathname);
void exit_jsh(int val);
int question_mark();
int external_command(Command* command, char* fifo_out_name);

// Fonctions auxiliaires
int main(int argc, char** argv);
void main_loop();
void executeCommand(Command* command, char* fifo_out_name);
void apply_redirections(Command* command, char* fifo_in_name, char* fifo_out_name);
bool is_internal(char* command_name);
void callRightCommand(Command* command);
bool correct_nbArgs(char**, unsigned, unsigned);
char* getPrompt();

// Variables globales
bool running;
int lastReturn;
char* current_folder;
char* previous_folder;