/*
    Autor: Sergio Carro Albarrán
    Grado en Ingeniería en Tecnologías de la Telecomunicación
    Sistemas Operativos
    proctailtxt.c
*/

/*
    gcc -c -Wall -Wshadow -g -O0 proctailtxt.c && gcc -o proctailtxt proctailtxt.o
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

#define NUMELEM 20
#define DIRACTUAL "."
#define TXT ".txt"
#define LTXT 4
#define SALIDA ".out"
#define BUFFSIZE 1024


void
inicializarArray(char *ficheros[])
{
    int i = 0;
    for(;i<NUMELEM;i++){
        ficheros[i] = NULL;
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


void
imprimirFicheros(char *ficheros[]){
    int i = 0;
    while(ficheros[i] && i<NUMELEM){
        printf("%i: [%s]\n", i+1, ficheros[i]);
        i++;
    }
}

void
liberarFicheros(char *ficheros[]){
    int i = 0;
    while(ficheros[i] && i < NUMELEM){
        free(ficheros[i]);
        ficheros[i] = NULL;
        i++;
    }
}

/* Cambia el tipo a integer y comprueba que es un argumento correcto
    return: 1 OK
            0 not valid argument
*/
int
esArgumentoValido(char *argumento)
{
    int numero = atoi(argumento);
    int numaux = numero;
    int longitud = strlen(argumento);
    int cifras = 1;

    while(numaux/10>0){
        numaux = numaux/10;
        cifras++;
    }

    if(numero > 0 && cifras == longitud)
        return numero;
    return 0;
}

/*
    Devuelve el tamaño del contenido del archivo txt
    return: -1 error with stat
*/
int
longitudTxt(char *nombre_archivo)
{
    struct stat bufferdatos;
    if(stat(nombre_archivo, &bufferdatos) >= 0){
        return bufferdatos.st_size;
    }
    return -1;
}

/*
    Comprueba si se trata de un archivo txt
    return  1 OK
            0 no es la ruta de un txt
*/
int
esTxt (char *nombre)
{
    struct stat bufferdatos;
    int longitud = 0;
    char *tipo=NULL;

    if(stat(nombre, &bufferdatos) >=0){
        if(S_ISREG(bufferdatos.st_mode)){
            tipo = strrchr(nombre, '.');
            if(tipo){
                longitud = strlen(tipo);
                if(longitud == LTXT && !strcmp(tipo, TXT))
                    return 1;
            }
        }
    }
    return 0;
}


int
dameTxts(char *ruta, char *ficheros[])
{
    DIR *directorio;
    struct stat bufferdatos;
    struct dirent *carpeta;
    char *nombre;
    int num_txts = 0;

    if(stat(ruta, &bufferdatos)>=0){
        if(S_ISDIR(bufferdatos.st_mode)){
            directorio = opendir(ruta);
            if(!directorio){
                err(1, "%s: cant open directory", ruta);
            }
            while((carpeta = readdir(directorio))){
                nombre = carpeta->d_name;
                if(esTxt(nombre)){
                    if(num_txts == NUMELEM){
                        liberarFicheros(ficheros);
                        closedir(directorio);
                        return -1;
                    }
                    ficheros[num_txts] = strdup(nombre);
                    num_txts++;
                }
            }
            closedir(directorio);
        }
    }
    return num_txts;
}

int
crearOut(char *fichero)
{
    int longitud = strlen(fichero), fd;
    char nuevo[longitud + 5];

    strcpy(nuevo, fichero);
    strcat(nuevo,SALIDA);
    
    if((fd = open(nuevo, O_CREAT | O_TRUNC | O_WRONLY, 0664)))
        return fd;
    else
        return -1;
}

/*
    Reader Y Writer
    return: number of bytes readed
            -1 error
*/
int
leoYEscribo(char *fichero, int fd)
{
    char buffer[BUFFSIZE];
    int leidos = 0, total = 0, escritos = 0;
    int fdout = crearOut(fichero);

    if(fdout>0){
        reiniciarBuffer(buffer, BUFFSIZE);
        for(;;){
            total += leidos;
            leidos = read(fd, buffer, BUFFSIZE-1);
            if(leidos < 0){
                warn("ERROR reading");
                total = -1;
                break;
            }
            if(leidos){
                escritos = write(fdout, buffer, leidos);
                if(escritos < leidos){
                    warnx("Impossible to write all readed chars");
                    close(fdout);
                    return -1;
                } else if (escritos < 0){
                    warnx("Write Error");
                    close(fdout);
                    return -1;
                }
            }
            if(leidos == 0){
                break;  
            }
            reiniciarBuffer(buffer, leidos);
        }
        close(fdout);
    } else {
        return -1;
    }
    return total;
}


int
procesarTxt(char *fichero, int nbytes)
{
    int fd=0, leidos=0, offset=0;
    int tam = longitudTxt(fichero);

    if((fd = open(fichero, O_RDONLY))>=0){
        if(nbytes && (nbytes < tam)){
            offset = lseek(fd, -(nbytes), SEEK_END);
        }
        if(offset >= 0){
            leidos = leoYEscribo(fichero, fd);
            close(fd);
            if(leidos < 0){
                return -1;
            }
        }
    } else {
        warnx("Error opening %s.", fichero);
        return -1;
    }
    return leidos;
}

int
procesarTxts(char *ficheros[], int num_txts, int nbytes)
{
    int status, i, resultado = 0;

    for(i=0; i<num_txts;i++){
        switch(fork()){
            case -1:
                errx(1, "FORK ERROR");
            case 0:
                resultado = 0;
                resultado = procesarTxt(ficheros[i], nbytes);
                if(resultado < 0){
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_SUCCESS);
            default:
                ;
        }
    }
    while(wait(&status)>0){
        if((WEXITSTATUS(status))>0){
            resultado = -1;
        }
    }
    return resultado;
}

int
main(int argc, char *argv[])
{
    char *ficheros[NUMELEM];
    int num_txts, nbytes=0, resultado = 0;

    argc--;
    argv++;

    if(argc==1){
        nbytes = esArgumentoValido(argv[0]);
        if(!nbytes){
            errx(1,"Not valid argument.");
        }
    } else if(argc > 1){
        errx(1, " Usage: [option]");
    }

    inicializarArray(ficheros);
    num_txts = dameTxts(DIRACTUAL, ficheros);
    if(num_txts>0){
        resultado = procesarTxts(ficheros, num_txts, nbytes);
        liberarFicheros(ficheros);
    }
    if(resultado<0){
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}