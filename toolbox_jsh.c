#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "toolbox_jsh.h"

// Vérifie que le pointeur passé en argument est différent de NULL.
void checkAlloc(void* ptr) {
    if (ptr == NULL) {
        fprintf(stderr,"ERROR IN MALLOC : NOT ENOUGH SPACE !");
        exit(-1);
    }
}

int convert_str_to_int (char* string) {
    char** tmp = malloc(sizeof(char)*50);
    int int_args = strtol(string,tmp,10);//base 10 and we store invalids arguments in tmp
    if ((strcmp(tmp[0],"") != 0 && strlen(tmp[0]) > 0) || int_args == LONG_MIN || int_args == LONG_MAX) {//we check the second argument doesn't contain some chars
        fprintf(stderr,"Exit takes a normal integer as argument\n");
        return INT_MIN;
    }
    free(tmp);
    return int_args;
}

int length_base10(int n) {
    int i = 1;
    int x = n;
    while (x>= 10){
        i++;
        x = x/10;
    }
    return i;
}