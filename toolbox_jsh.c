#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "toolbox_jsh.h"

// Vérifie que le pointeur passé en argument est différent de NULL.
void checkAlloc(void* ptr) {
    if (ptr == NULL) {
        fprintf(stderr,"ERROR IN MALLOC : NOT ENOUGH SPACE !");
        exit(-1);
    }
}

// Prend une string en argument et renvoie l'entier représenté par cette string.
int convert_str_to_int (char* string) {
    char* string2 = malloc(sizeof(char)*strlen(string)+1);
    // Enlever l'éventuel '-' ou '%' devant le nombre.
    if (string[0] == '-' || string[0] == '%') {
        for (int i = 0; i < strlen(string)-1; i++) {
            *(string2+i) = *(string+i+1);
        }
        string2[strlen(string)-1] = '\0';
    } else strcpy(string2,string);
    char** tmp = malloc(sizeof(char*));
    int int_args = strtol(string2,tmp,10);//base 10 and we store invalids arguments in tmp
    // if ((strcmp(tmp[0],"") != 0 && strlen(tmp[0]) > 0) || int_args == LONG_MIN || int_args == LONG_MAX) {//we check the second argument doesn't contain some chars
    //     free(string2);
    //     free(tmp);
    //     return INT_MIN;
    // }
    free(tmp);
    free(string2);
    if (string[0] == '%') {
        return getpgid(int_args);
    }
    return int_args;
}

// Retourne le nombre de chiffres utilisés dans l'écriture d'un entier en base 10.
int length_base10(int n) {
    int i = 1;
    int x = n;
    while (x>= 10){
        i++;
        x = x/10;
    }
    return i;
}

// Vérifie si une string n'est pas constituée que d'espaces.
int is_only_spaces(char* string) {
    for (unsigned i=0; i < strlen(string); ++i) {
        if (string[i] != ' ') return 0;
    }
    return 1;
}