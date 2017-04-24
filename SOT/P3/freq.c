/*
    Autor: Sergio Carro Albarrán
    Grado en Ingeniería en Tecnologías de la Telecomunicación
    Sistemas Operativos
    freq.c
*/

/*
    gcc -c -Wall -Wshadow -g freq.c && gcc -o freq freq.o
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <ctype.h>

#define ASCII 256
#define BUFFSIZE 10

typedef struct letra {
    int freq;
    int primera;
    int ultima;
}letra;

void
inicializarDiccionario(letra diccionario[])
{
    int i;
    for(i=0; i<ASCII;i++){
        diccionario[i].freq=0;
        diccionario[i].primera=0;
        diccionario[i].ultima=0;
    }
}

void
reiniciarBuffer(char buffer[], int num)
{
    int i = 0;
    while(i<num){
        buffer[i] = 0;
        i++;
    }
}

/*
    return  1 si hay opcion
            0 si no la hay
*/
int
hayOpcion(char *opcion){
    return !(strcmp(opcion, "-i"));
}

/*
    return numero total de caracteres con frecuencia de aparición encontrados en el diccionario
*/
int
totalChars(letra diccionario[])
{
    int i, total = 0;
    for(i=0;i<ASCII;i++){
        if(diccionario[i].freq){
            total += diccionario[i].freq;
        }
    }
    return total;
}

/*
    Calcula la frecuencia de las letras
    return Ultimo caracter leido
*/
char
frecuencia(char buffer[], letra diccionario[], int opcion, char ultimo)
{
    int i=0;
    char c = buffer[i], ult = ultimo;

    do{
        if(isalnum(c)){
            if(!opcion){
                c = tolower(c);
            }
            diccionario[(int)c].freq++;
            //COMPROBAMOS SI ES LA PRIMERA
            if(i>0 && i<=strlen(buffer) && !isalnum(buffer[i-1])){
                diccionario[(int)c].primera++;
            }else if(i==0 && !isalnum(ult)){
                diccionario[(int)c].primera++;
            }
            //COMPROBAMOS SI ES LA ULTIMA
            if(i>=0 && i<strlen(buffer) && !isalnum(buffer[i+1])){
                diccionario[(int)c].ultima++;
            }
        } else {
            if(isalnum(ultimo)){
                diccionario[(int)ultimo].ultima++;
            }
        }
        ult = buffer[i];
        i++;
        c = buffer[i];
    } while(c != 0);
    return ult;
}

int
leerFichero(int fdescriptor, int opcion, letra diccionario[])
{
    char buffer[BUFFSIZE], ultimo=0;;
    int leidos = 0, total = 0;

    reiniciarBuffer(buffer, BUFFSIZE);

    for(;;){
        total += leidos;
        leidos = read(fdescriptor, buffer, BUFFSIZE-1);
        if(leidos < 0){
            warn("ERROR reading %d", fdescriptor);
            total = 0;
            break;
        }
        if(leidos > 0){
            ultimo = frecuencia(buffer, diccionario, opcion, ultimo);
        }
        if(leidos == 0){
            break;  
        }
        reiniciarBuffer(buffer, leidos);
    }
    return total;
}


void
imprimirDiccionario(letra diccionario[])
{
    int i, total = totalChars(diccionario); 
    float porcentaje = 0.0;

    if(total){
        for(i=0;i<ASCII;i++){
            if(diccionario[i].freq){
                porcentaje = ((float)(100*diccionario[i].freq) / total);
                printf("%c %3.2f%% %d %d\n", i, porcentaje, diccionario[i].primera, diccionario[i].ultima);
            }
        }
    } else {
        printf("No characters found.\n");
    }
}


int
main(int argc, char *argv[])
{
    letra diccionario[ASCII];
    int opcion = 0, fd=0, i=0;

    inicializarDiccionario(diccionario);
    argc--;
    argv++;

    if(argc > 0){
        opcion = hayOpcion(argv[0]);
        if(opcion && argc == 1) // leemos de la stdin
            leerFichero(fd, opcion, diccionario);
        else {
            if(opcion)
                i=1;
            for(;i<argc;i++){
                fd = open(argv[i], O_RDONLY);
                if(fd > -1){
                    leerFichero(fd, opcion, diccionario);
                    close(fd);
                } else {
                    warn("Error: impossible to open %s\n", argv[i]);
                }
            }
        }
    } else {
        // tenemos fd a 0 así que leemos el fichero de entrada de la stdin
        leerFichero(fd, opcion, diccionario);
    }

    imprimirDiccionario(diccionario);

    exit(EXIT_SUCCESS);
}