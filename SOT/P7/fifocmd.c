/*
    Autor: Sergio Carro Albarrán
    Grado en Ingeniería en Tecnologías de la Telecomunicación
    Sistemas Operativos
    fifocmd.c
*/

/*
    gcc -c -Wall -Wshadow -g -O0 fifocmd.c && gcc -o fifocmd fifocmd.o
*/

/*
    Redirijo a /dev/null los errores de las ejecuciones unicamente
    En caso de querer redirigir todos los errores, habria que abrir /dev/null
    en el main y hacer un dup2(fd, 2), con lo que quedarían así redirigidos
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <err.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define BUFFSIZE 1024


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
    Separa los caracteres dependiendo del separador introducido
    return: Array con todos los elementos separados por separador
            NULL, si no hay
*/
char **
tokenizar(char *leido, int *ncomandos, char *separador)
{
    char **comandos = NULL;
    char *token = NULL;
    int contador = 0;

    for(token=strtok(leido, separador); token!=NULL; token=strtok(NULL, separador)){
        comandos = (char **)realloc(comandos, (contador+1)*sizeof(char *));
        comandos[contador] = token;
        contador++;
    }

    // Ultimo elemento a NULL
    comandos = (char **)realloc(comandos, (contador+1)*sizeof(char *));
    comandos[contador] = NULL;

    *ncomandos = contador;
    return comandos;
}


/*
    Elimina ultimo elemento y comprueba que lo restante es valido
    return  0 no valida
            1 valida
*/
int
esRutaValida(char *ruta)
{
    struct stat bufferdatos;
    char *path = NULL;
    char *path_aux = NULL;
    int valida = 0;

    if(ruta && ruta[0] != '/'){
        path = dameRutaCompleta(ruta, ".");
    } else {
        path = strdup(ruta);
    }
    path_aux = strrchr(path, '/');
    if(path_aux){
        path_aux[0] = '\0';
    }
    if((stat(path, &bufferdatos))>=0){
        if(S_ISDIR(bufferdatos.st_mode)){
            valida = 1;
        }
    }
    free(path);
    path = NULL;
    return valida;
}

char *
dameComando(char *comando)
{
    char *ruta = NULL;
    int i = 0, nopciones = 0;
    char *path_env = getenv("PATH");

    char **opciones = tokenizar(path_env, &nopciones, ":");
    while(i<nopciones){
        ruta = dameRutaCompleta(comando, opciones[i]);
        if(access(ruta, X_OK) == 0){
            free(opciones);
            return ruta;
        }
        free(ruta);
        ruta = NULL;
        i++;
    }
    free(opciones);
    return NULL;
}

char *
lector(char *leido, char buffer[], int total)
{
    char *aux = NULL;
    if(!leido){
        leido = strdup(buffer);
    } else {
        aux = calloc(total, sizeof(char *));
        strcpy(aux, leido);
        free(leido);
        leido = NULL;
        strcat(aux, buffer);
        leido = aux;
        aux = NULL;
    }
    return leido;
}

void
imprimirArray(char **comandos){
    int i=0;
    while(comandos[i]){
        printf("[%s]\n", comandos[i]);
        i++;
    }
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

void
exec1(int pipe1[], int fderr, char **cmd_array)
{
    char *cmd = NULL;

    cmd = dameComando(cmd_array[0]);
    dup2(pipe1[1], 1);
    if(fderr>-1)
        dup2(fderr, 2);
    close(pipe1[0]);
    close(pipe1[1]);
    if(cmd){
        execv(cmd, cmd_array);
    } else {
        exit(EXIT_FAILURE);
    }
}

void
exec2(int pipe1[], int fderr, char *argv[])
{
    int fd = 0;

    if((fd = open("fifocmd.out", O_CREAT | O_APPEND | O_WRONLY, 0664))){
        dup2(pipe1[0], 0);
        dup2(fd, 1);
        if(fderr>-1)
            dup2(fderr, 2);
        close(pipe1[0]);
        close(pipe1[1]);
        execvp(argv[0], argv);
    }
    exit(EXIT_FAILURE);
}

int
pipeline(char *comando, char *argv[])
{
    char **cmd_array = NULL;
    int num_pipes = 1, num_comandos = 0;
    int pipes[num_pipes][2];
    int pid, status, i, fderr;

    if(crearPipes(pipes, 1)){
        warnx("Cannot create pipe");
        return -1;
    }
    cmd_array = tokenizar(comando, &num_comandos, " ");

    if((fderr = open("/dev/null", O_CREAT | O_APPEND | O_WRONLY, 0664))<0){
        warn("Los errores no han podido ser redirigidos a dev/null\n");
    }
    for(i=0;i<2;i++){
        pid = fork();
        switch(pid){
            case -1:
                warnx("Fork Error");
                free(cmd_array);
                return -1;
            case 0:
                if(i==0){
                    exec1(pipes[0], fderr, cmd_array);
                }else{
                    exec2(pipes[0], fderr, argv);
                    fprintf(stderr, "%s: doesnt exists", argv[0]);
                }
                exit(EXIT_FAILURE);
            default:
                ;
        }
    }

    cerrarPipes(pipes, 1);
    waitpid(pid, &status, 0);
    free(cmd_array);
    return 0;
}


void
procesar(char *leido, char *argv[])
{
    char **comandos;
    int num_comandos = 0, i=0;

    comandos = tokenizar(leido, &num_comandos, "\n");
//    printf("num_comandos = [%d]\n", num_comandos);
//    imprimirArray(comandos);
    while(i<num_comandos){
        pipeline(comandos[i], argv);
        i++;
    }
    free(comandos);
    comandos = NULL;
}

int
main(int argc, char *argv[])
{
    char buf[BUFFSIZE];
    int fd, total = 0, nr = 0;
    char *leido = NULL, *ruta = NULL;

    argc--;
    argv++;

    if(argc < 2){
        errx(1, "Usage: [fifo path] [command] [args]");
    } else {
        if(!esRutaValida(argv[0])){
            errx(1,"Invalid path to create fifo");
        }
    }

    ruta = argv[0];
    argc--;
    argv++;
    unlink(ruta);
    unlink("fifocmd.out");
    if(mkfifo(ruta, 0664) < 0)
        err(1, "Error: couldnt make fifo");
    for(;;){
        fd = open(ruta, O_RDONLY);
        if(fd < 0) {
            errx(1, "Error: couldnt open fifo");
        }
        for(;;){
            nr = read(fd, buf, sizeof buf -1);
            if(nr < 0){
                errx(1, "Error: fifo reading");
            }
            if(nr == 0){
                procesar(leido, argv);
                break;
            }
            if(strcmp(buf, "bye\n") == 0){
                close(fd);
                exit(EXIT_SUCCESS);
            }
            total = total + nr;
            buf[nr] = 0;
            leido = lector(leido, buf, total);
        }
        free(leido);
        leido = NULL;
        total = 0;
        close(fd);
    }
    exit(EXIT_SUCCESS);
}