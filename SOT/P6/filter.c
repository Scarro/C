/*
    Autor: Sergio Carro Albarrán
    Grado en Ingeniería en Tecnologías de la Telecomunicación
    Sistemas Operativos
    filter.c
*/

/*
    gcc -c -Wall -Wshadow -g -O0 filter.c && gcc -o filter filter.o
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#define BUFFSIZE 1024
#define DIRECTORIOACTUAL "."

/*
    Si existe devuelve la ruta absoluta del fichero
    return: absolute path
            NULL: Malloc error, current working directory error
*/
char *
dameRutaCompleta(char *fichero, char *directorio)
{
    int longitud1 = 0, longitud2 = 0;
    char *ruta_completa = NULL;
    char buffer[BUFFSIZE];
    char *ruta = NULL;

    longitud1 = strlen(fichero);
    if(!strcmp(directorio, ".")){
        ruta = getcwd(buffer, BUFFSIZE);
        if(!ruta){
            errx(1, "Error finding path");
        }
        longitud2 = strlen(ruta);
    } else {
        longitud2 = strlen(directorio);
    }
    if(!(ruta_completa = (char *)malloc(longitud1 + longitud2 + 2))){
        errx(1, "Malloc error: %s\n", fichero);
    }
    if(strcmp(directorio, ".")){
        if(directorio[strlen(directorio)-1] != '/')
            sprintf(ruta_completa, "%s/%s", directorio, fichero);
        else
            sprintf(ruta_completa, "%s%s", directorio, fichero);
    } else {
        sprintf(ruta_completa, "%s/%s", ruta, fichero);
    }
    return ruta_completa;
}

/*
    Devuelve el path completo del comando buscando entre los posibles directorios
    donde se encuentre su ejecutable
*/
char *
dameComando(char *comando)
{
    struct stat bufferdatos;
    char *ruta = NULL;
    int i = 0;
    char *opciones[] = {"/usr/bin", "/bin"};

    for(;i<2;i++){
        ruta = dameRutaCompleta(comando, opciones[i]);
        if(stat(ruta, &bufferdatos)>=0){
            if(access(ruta, X_OK) == 0){
                return ruta;
            }
        }
    }
    free(ruta);
    ruta = NULL;
    return ruta;
}

/*
    return: 1 -> es fichero
            0 -> NO es fichero
*/
int
esFichero(char *fichero){
    struct stat bufferdatos;

    if(stat(fichero, &bufferdatos)>=0){
        if(S_ISREG(bufferdatos.st_mode)){
            return 1;
        }
    }
    return 0;
}


int
crearPipes(int pipes[][2], int num_pipes)
{
    int i;
    for(i=0; i < num_pipes; i++){
        if(pipe(pipes[i])<0){
            return -1;
        }
    }
    return 0;
}


void
cerrarPipes(int pipes[][2], int num_pipes)
{
    int i = 0;
    for(i=0;i<num_pipes;i++){
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}

/*
    Ejecuta el comando que le toque dependiendo del hijo que sea
    return: -1 si no se ha conseguido ejecutar el comando
*/
int
ejecutarComando(int num_hijo, int pipes[][2], int fd, char *reg_exp, char *comando, char *argumentos[])
{
    if(num_hijo==0){
        //Derivo los errores a /dev/null para facilitar la vista de la solucion
        close(pipes[0][0]);
        dup2(fd, 0);
        dup2(pipes[0][1], 1);
        close(pipes[0][1]);
        execv(comando, argumentos);
        return -1;
    }else{
        close(pipes[0][1]);
        dup2(pipes[0][0],0);
        close(pipes[0][0]);
        execlp("grep", "grep", reg_exp, NULL);
        return -1;
    }
}

/*
    procesa el fichero ordenado la ejecucion del comando y el grep sobre el
    return: -1 en caso de que ocurra algun error o que no encuentre
            la expresion regular indicada
            0 exito
*/
int
procesarFichero(char *fichero, char *reg_exp, char *argumentos[])
{
    int num_pipes = 1, fd, i, status, resultado = 0;
    int pipes[num_pipes][2];
    char *comando = NULL;

    if(crearPipes(pipes, 1)){
        errx(1, "PIPE ERROR");
    }
    if(!(comando = dameComando(argumentos[0]))){
        errx(1, "Error: %s invalid.", comando);
    }
    if((fd = open(fichero,O_RDONLY))>=0){
        for(i=0;i<2;i++){
            switch(fork()){
                case -1:
                    errx(1, "FORK ERROR\n");
                case 0:
                    if(!(ejecutarComando(i, pipes, fd, reg_exp, comando, argumentos))){
                        exit(EXIT_FAILURE);
                    }
                    exit(EXIT_SUCCESS);
                default:
                    ;
            }
        }
        cerrarPipes(pipes, 1);
        while(wait(&status)>0){
            if((WEXITSTATUS(status))>0){
                resultado = -1;
            }
        }
        close(fd);
        free(comando);
        return resultado;
    } else {
        warnx("Error opening %s", fichero);
        return -1;
    }
}

/*
    Procesa todos los ficheros secuencialmente
    return: numero de errores ocurridos
*/
int
procesarFicheros(char *reg_exp, char *argumentos[], char *working_directory)
{
    DIR *directorio;
    struct stat bufferdatos;
    struct dirent *carpeta;
    char *nombre = NULL;
    int errores = 0;

    if(stat(working_directory, &bufferdatos)>=0){
        if(S_ISDIR(bufferdatos.st_mode)){
            directorio = opendir(working_directory);
            if(!directorio){
                errx(1, "Error: cant open directory");
            }
            while((carpeta = readdir(directorio))){
                nombre = dameRutaCompleta(carpeta->d_name, working_directory);
                if(esFichero(nombre)){
                    if(procesarFichero(nombre, reg_exp, argumentos)<0){
                        errores++;
                    }
                }
                free(nombre);
            }
            closedir(directorio);
        }
    }
    return errores;
}

int
main(int argc, char *argv[])
{
    char *reg_exp = NULL;
    char *working_directory=NULL;
    char buffer[BUFFSIZE];
    int resultado = 0;
    argc--;
    argv++;

    chdir("Archivos");
    working_directory = getcwd(buffer, BUFFSIZE);
    if(argc > 1){
        reg_exp = argv[0];
        argc--;
        argv++;
    } else {
        err(1, "Usage: [regexp] [command] [args]");
    }

    resultado = procesarFicheros(reg_exp, argv, working_directory);

    if(resultado){
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}