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
        if (tmp[0][0] == '%') {
            char* string2 = malloc(sizeof(char)*strlen(string)-1);
            for (int i = 0; i < strlen(string)-1; i++) {
                *(string2+i) = *(string+i+1);
            }
            free(tmp);
            char** tmp2 = malloc(sizeof(char)*50);
            string2[strlen(string)-1] = '\0';
            int_args = strtol(string2,tmp2,10);
            if ((strcmp(tmp2[0],"") != 0 && strlen(tmp2[0]) > 0) || int_args == LONG_MIN || int_args == LONG_MAX) {
                free(tmp2);
                free(string2);
                return INT_MIN;
            }
            free(tmp2);
            free(string2);
            int_args = int_args*(-1);
            printf("%d\n",int_args);
            return int_args;
        }
        free(tmp);
        return INT_MIN;
    }
    free(tmp);
    printf("%d\n",int_args);
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