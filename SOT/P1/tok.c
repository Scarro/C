/*
    Autor: Sergio Carro Albarrán
    Grado en Ingeniería en Tecnologías de la Telecomunicación
    Sistemas Operativos
    tok.c
*/

/*
    gcc -c -Wall -Wshadow -g tok.c && gcc -o tok tok.o
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
mytokenize(char *str, char **args, int maxargs)
{
    char *ptr = str;
    int indice = 0;
    int longitud = strlen(str);
    int posicion = 0;

    while(*ptr != '\0'){
        if(*ptr == '\t' || *ptr=='\r' || *ptr == ' ' || *ptr == '\n'){
            *ptr = '\0';
            ptr++;
            while(*ptr=='\t' || *ptr=='\r' || *ptr==' ' || *ptr=='\n'){
                ptr++;
            }
        } else {
            ptr++;
        }
    }
    ptr = str;
    while(indice < maxargs && posicion < longitud){
        while(*ptr=='\t' || *ptr=='\r' || *ptr==' ' || *ptr=='\n'){
            ptr++;
            posicion++;
        }
        if(*ptr != '\0'){
            args[indice] = ptr;
            indice++;
            if(posicion < longitud){
                while(*ptr != '\0'){
                    ptr++;
                    posicion++;
                }
            }
        } else {
            ptr++;
            posicion++;
        }
    }
    return indice;
}

int
main(int argc, char *argv[])
{
    char frase[] = "              Cadenai     de texto      \nhola ";
    int maxargs = 5;
    char *args[maxargs];
    int resultado = 0;
    int i = 0;

    resultado = mytokenize(frase, args, maxargs);
    while(i<resultado){
        printf("args[%d]: [%s]\n", i, args[i]);
        i++;
    }
    exit(EXIT_SUCCESS);
}