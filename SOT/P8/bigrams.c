/*
    Autor: Sergio Carro Albarrán
    Grado en Ingeniería en Tecnologías de la Telecomunicación
    Sistemas Operativos
    bigrams.c
*/

/*
    gcc -g -Wall -Wshadow -pthread bigrams.c && gcc -o bigrams bigrams.o -pthread
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <err.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct mapa{
    void *direccion;
    char *archivo;
    pthread_mutex_t mutex;
}MAPA;

pthread_mutex_t mutex;

int
calculate_position(char char1, char char2){
    int a = toascii(char1);
    int b = toascii(char2);
    int posicion = ((a*128) + b);
    return posicion;
}

void
agregarAMapa(MAPA *mapa, int posicion)
{
    unsigned char *direccion = (unsigned char *)mapa->direccion + posicion;

    if(*direccion <255){
        (*direccion)++;
    }
}

void*
procesar(void *args)
{
    MAPA *mapa = (MAPA *)args;
    FILE *file;
    char leido, caracter;
    int primera = 0, posicion;

    if(!(file = fopen(mapa->archivo, "r"))){
        warnx("Error opening %s\n", mapa->archivo);
        pthread_exit((void *)1);
    }
    while((leido=fgetc(file)) != EOF){
        if(!primera){
            caracter = leido;
            primera++;
        } else {
            posicion = calculate_position(caracter, leido);
            pthread_mutex_lock(&mapa->mutex);
            agregarAMapa(mapa, posicion);
            pthread_mutex_unlock(&mapa->mutex);
            caracter = leido;
        }
    }
    fclose(file);
    return NULL;
}

void *
crearMapa(char *fichero){
    int fd;
    void *direccion;
    int length = 128*128;

    fd = creat(fichero, 0644);
    if(fd < 0)
        err(1, "Creat Error: cannot create map");
    
    lseek(fd,(length)-1, 0);
    if(write(fd,"\0",1) != 1) 
        errx(1, "Write Error: impossible to write on map");

    close(fd);

    if((fd = open(fichero, O_RDWR))<0)
        errx(1, "Open Error: cannot open map");

    direccion = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, fd, 0);
    if(direccion == MAP_FAILED){
        errx(1, "Map Error: cannot create mmap");
    }
    close(fd);
    return direccion;
}

void
imprimir(MAPA mapa)
{
    int i, j;
    unsigned char* mapa_aux;
    void* direccion = mapa.direccion;
    mapa_aux = (unsigned char *)direccion;
    
    for(i = 0; i < 128; i++){
        for(j = 0; j < 128; j++){
            mapa_aux = (mapa_aux+(i*128 + j));
            printf("(%d, %d): %d\n", i, j, *mapa_aux);
            mapa_aux = direccion;
        }
    }
}

int
main(int argc, char *argv[])
{
    MAPA mapa;
    int option, i=0;
    int length = 128*128;

    argc--;
    argv++;
    if(argc < 2){
        errx(1, "Usage: [option][mmap name][files]");
    }
    if(!(strcmp(argv[0], "-p"))){
        option = 1;
        argv++;
        argc--;
    }

    mapa.direccion = crearMapa(argv[0]);
    argv++;
    argc--;

    pthread_t thread_id[argc];
    
    pthread_mutex_init(&mapa.mutex, NULL);

    while(i<argc){
        mapa.archivo = argv[i];
        pthread_create(&thread_id[i], NULL, &procesar, &mapa);
        i++;
    }

    i=0;
    while(i<argc){
        pthread_join(thread_id[i], NULL);
        i++;
    }

    if(option){
        imprimir(mapa);
    }

    if(munmap(mapa.direccion, length)<0){
        errx(1, "Munmap Error: cannot close map");
    }

    pthread_mutex_destroy(&mapa.mutex);
    exit(EXIT_SUCCESS);
}