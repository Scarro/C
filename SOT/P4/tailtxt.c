/*
    Autor: Sergio Carro Albarrán
    Grado en Ingeniería en Tecnologías de la Telecomunicación
    Sistemas Operativos
    tailtxt.c
*/

/*
    gcc -c -Wall -Wshadow -g tailtxt.c && gcc -o tailtxt tailtxt.o
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUFFSIZE 1024
#define TXT ".txt"
#define LTXT 4


void
reiniciarBuffer(char buffer[], int num)
{
    int i = 0;
    while(i<num){
        buffer[i] = 0;
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
    Reader
    return: number of bytes readed
            -1 error
*/
int
leerArchivo(int fd)
{
    char buffer[BUFFSIZE];
    int leidos = 0, total = 0;

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
            write(1, buffer, leidos);
        }
        if(leidos == 0){
            break;  
        }
        reiniciarBuffer(buffer, leidos);
    }
    return total;
}


/*
    return  >0 readed bytes
            -1 error opening or reading
*/
int
leerTxt(char *nombre, int longitud, int nbytes)
{
    int fdescriptor=0, leidos=0, offset=0;
    int tam = longitudTxt(nombre);

    if((fdescriptor = open(nombre, O_RDONLY))>=0){
        if(nbytes && (nbytes < tam)){
            offset = lseek(fdescriptor, -(nbytes), SEEK_END);
        }
        if(offset >= 0){
            leidos = leerArchivo(fdescriptor);
            close(fdescriptor);
            if(leidos < 0){
                return -1;
            }
        }
    } else {
        warnx("Error opening %s.", nombre);
        return -1;
    }
    return leidos;
}


int
procesarTxts(char *ruta, int nbytes)
{
    DIR *directorio;
    struct stat bufferdatos;
    struct dirent *carpeta;
    char *nombre;
    int longitud=0, leidos=0, errores=0, narchivos=0;

    if(stat(ruta, &bufferdatos)>=0){
        if(S_ISDIR(bufferdatos.st_mode)){
            directorio = opendir(ruta);
            if(directorio == NULL){
                err(1, "Error opening currnent dir");
            }
            while((carpeta = readdir(directorio))){
                nombre = carpeta->d_name;
                if(esTxt(nombre)){
                    leidos = leerTxt(nombre, longitud, nbytes);
                    if(leidos == -1){
                        errores++;
                    } else{
                        narchivos++;
                    }
                }
            }
            closedir(directorio);
        }
    }
    if(errores)
        return -1;
    return narchivos;
}


int
main(int argc, char *argv[])
{
    int nbytes = 0, narchivos = 0;
    argc--;
    argv++;

    if(argc == 1){
        nbytes = esArgumentoValido(argv[0]);
        if(nbytes < 1){
            err(1, " not valid arg");
        }
    }else if(argc > 1){
        fprintf(stderr, "Usage: [option]");
        exit(EXIT_FAILURE);
    }
    narchivos = procesarTxts(".", nbytes);
    if(narchivos<0)
        exit(EXIT_FAILURE);
    exit(EXIT_SUCCESS);
}