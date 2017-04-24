/*
    Autor: Sergio Carro Albarrán
    Grado en Ingeniería en Tecnologías de la Telecomunicación
    Sistemas Operativos
    myecho.c
*/

/*
    gcc -c -Wall -Wshadow -g myecho.c && gcc -o myecho myecho.o
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/*
    Comprueba si existe opcion
    return: 0 Hay opcion
            1 No
*/
int
hayOpcion(char *opcion)
{
    return !(strcmp(opcion, "-n"));
}

/*
    Simple echo con sustitución
    return: 0 si no ha ejecutado nada
            >0 segun el numero de impresiones que haya realizado
*/
int
myecho(char *argumentos[], int num_argumentos, int opcion, char *usuario, char* casa)
{
    char dir_actual[1024];
    int pid = getpid();
    int i = 0, resultado = 0;

    getcwd(dir_actual, 1024);

    if(opcion)
        i++;
    for(;i<num_argumentos;i++){
        if(*(argumentos[i]) == '*'){
            printf("%i", pid);
        }else if(!strcmp(argumentos[i], "USUARIO")){
            printf("%s", usuario);
        } else if(!strcmp(argumentos[i], "CASA")){
            printf("%s", casa);
        } else if(!strcmp(argumentos[i], "DIRECTORIO")){
            printf("%s", dir_actual);
        } else {
            printf("%s", argumentos[i]);
        }
        if(i<num_argumentos-1)
            printf(" ");
    }
    if(!opcion)
        printf("\n");

    return resultado;
}

int
main(int argc, char *argv[])
{
    char *user=getenv("USER"), *casa=getenv("HOME");
    int opcion = 0;

    argc--;
    argv++;

    if(argc > 0){
        opcion = hayOpcion(argv[0]);
        myecho(argv, argc, opcion, user, casa);

    }

    exit(EXIT_SUCCESS);
}