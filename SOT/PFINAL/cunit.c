/*
    Autor: Sergio Carro Albarrán
    Grado en Ingeniería en Tecnologías de la Telecomunicación
    Sistemas Operativos
    cunit.c
*/

/*
    gcc -c -Wall -Wshadow -g -O0 cunit.c && gcc -o cunit cunit.o
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>

#define BUFFSIZE 1024

typedef struct nodo{
    char *comando;
    char **argumentos; // comando incluido en el primer elemento (ultimo elemento NULL)
    struct nodo *siguiente;
}NODO;

typedef struct lista{
    NODO *primero;
    NODO *ultimo;
    int tam;
}LISTA;


int
esOpcionValida(char *argumento)
{
    if(!strcmp(argumento, "-c"))
        return 2;
    return 0;
}

int
longitudElementos(char **elementos)
{
    int len=0;
    if(elementos != NULL){
        while(elementos[len]){
            len++;
        }
    }
    return len;
}

void
imprimir(char **elementos){
    int i = 0;
    if(longitudElementos(elementos)){
        while(elementos[i]){
            printf("[%d]: [%s]\n", i+1, elementos[i]);
            i++;
        }
    }
}

void
liberar(char **elementos)
{
    int i = 0, tam=0;

    if(elementos != NULL){
        tam = longitudElementos(elementos);
        while(i<=tam){
            free(elementos[i]);
            elementos[i] = NULL;
            i++;
        }
        free(elementos);
    }
    elementos = NULL;
}

void
reiniciarBuffer(char buffer[])
{
    int i = 0;
    while(i<BUFFSIZE){
        buffer[i] = '\0';
        i++;
    }
}

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
            warnx("Error finding path");
            return NULL;
        }
        longitud2 = strlen(ruta);
    } else {
        longitud2 = strlen(directorio);
    }
    if(!(ruta_completa = (char *)malloc(longitud1 + longitud2 + 2))){
        warnx("Malloc error: %s\n", fichero);
        return NULL;
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
    return  0 no valida
            1 valida
*/
int
esRutaValida(char *ruta)
{
    struct stat bufferdatos;
    char *path = NULL;
    int valida = 0;

    if(ruta && ruta[0] != '/'){
        path = dameRutaCompleta(ruta, ".");
    } else {
        path = strdup(ruta);
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

/*
    Mi builtin
*/
int
mycd(char **argumentos)
{
    int len = 1;
    char *home = NULL;
    int valida = 0;

    while(argumentos[len]){
        len++;
    }
    if(len > 2 && len < 1){
        warnx("cd: wrong number of arguments.\n");
        return -1;
    }else if(len == 2){
        valida = esRutaValida(argumentos[1]);
        if(valida)
            chdir(argumentos[1]);
        else
            return -1;
    } else if(len == 1){
        home = getenv("HOME");
        chdir(home);
    }
    return len;
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
    char *leido_aux = strdup(leido);

    for(token=strtok(leido_aux, separador); token!=NULL; token=strtok(NULL, separador)){

        comandos = (char **)realloc(comandos, (contador+1)*sizeof(char *));
        if(token[0] == '$'){
            token++;
            token = getenv(token);
            if(token){
                comandos[contador] = strdup(token);
                contador++;
            }
        } else {
            comandos[contador] = strdup(token);
            contador++;
        }
    }

    // Ultimo elemento a NULL
    comandos = (char **)realloc(comandos, (contador+1)*sizeof(char *));
    comandos[contador] = NULL;

    *ncomandos = contador;
    free(leido_aux);
    return comandos;
}

char *
dameNuevoTipo(char *fichero, char *tipo)
{
    char *fich_aux = strdup(fichero);
    char *ptr_aux = NULL;
    int longitud1, longitud2;
    char *final;

    ptr_aux = strrchr(fich_aux, '.');
    if(ptr_aux){
        ptr_aux[0] = '\0';
    }
    longitud1 = strlen(fich_aux);
    longitud2 = strlen(tipo);
    if(!(final = (char *)malloc(longitud1 + longitud2 + 1))){
        warnx("Error malloc dameNuevoTipo");
        free(fich_aux);
        return NULL;
    }
    sprintf(final, "%s%s", fich_aux, tipo);

    free(fich_aux);
    return final;
}


char *
dameComando(char *comando)
{
    struct stat bufferdatos;
    char *ruta = NULL;
    char buffer[BUFFSIZE];
    char *working_directory, *opcion_aux, *opcion;
    char **opciones = NULL;
    int i = 0;
    char *path_env = getenv("PATH");
    int n_elementos;

    if(!strcmp(comando, "cd")){
        return "cd";
    }

    working_directory = getcwd(buffer, BUFFSIZE);
    opcion = dameRutaCompleta(comando, working_directory);
    if(stat(opcion, &bufferdatos)>=0){
        if(S_ISREG(bufferdatos.st_mode)){
            if(access(opcion, X_OK) == 0){
                return opcion;
            }
        }
    }
    free(opcion);
    opcion = NULL;

    //Busco sh tambien como ejecutables
    opcion_aux = dameNuevoTipo(comando, ".sh");
    opcion = dameRutaCompleta(opcion_aux, working_directory);
    free(opcion_aux);
    if(stat(opcion, &bufferdatos)>=0){
        if(S_ISREG(bufferdatos.st_mode)){
            if(access(opcion, X_OK) == 0){
                return opcion;
            }
        }
    }
    free(opcion);

    opciones = tokenizar(path_env, &n_elementos,":");

    while(opciones[i]){
        ruta = dameRutaCompleta(comando, opciones[i]);
        if(stat(ruta, &bufferdatos)>=0){
            if(S_ISREG(bufferdatos.st_mode)){
                if(access(ruta, X_OK) == 0){
                    liberar(opciones);
                    return ruta;
                }
            }
        }
        free(ruta);
        ruta = NULL;
        i++;
    }
    liberar(opciones);
    return NULL;
}

/*
    Operaciones de la lista
*/
void
inicializarLista(LISTA *lista)
{
    lista->primero = NULL;
    lista->ultimo = NULL;
    lista->tam = 0;
}

void
agregarElemento(char **argumentos, LISTA *lista)
{
    NODO *nuevo = NULL;
    NODO *nodoaux = NULL;

    if(!(nuevo = malloc(sizeof(NODO))))
        err(1, "No se pudo agregar elemento");
    nuevo->comando = argumentos[0];
    nuevo->argumentos = argumentos;
    nuevo->siguiente = NULL;
    if(!lista->primero){
        lista->primero = nuevo;
        lista->ultimo = nuevo;
    } else {
        nodoaux = lista->ultimo;
        nodoaux->siguiente = nuevo;
        lista->ultimo = nuevo;
    }
    lista->tam++;
}

void
imprimirContenido(LISTA lista)
{
    NODO *nodoaux = lista.primero;
    int i = 0;
    while(nodoaux){
        printf("comando %d: [%s]\n", i+1, nodoaux->comando);
        imprimir(nodoaux->argumentos);
        nodoaux = nodoaux->siguiente;
        i++;
    }
    printf("\n");
}

void
liberarContenido(LISTA *lista)
{
    NODO *nodoaux1 = lista->primero;
    NODO *nodoaux2 = lista->primero;

    while(nodoaux1){
        nodoaux2 = nodoaux1->siguiente;
        liberar(nodoaux1->argumentos);
        free(nodoaux1);
        nodoaux1 = nodoaux2;
    }
}
/*
*/

int
existeFOK(char *fichero)
{
    char *fich_ok = NULL;
    char *ruta_completa = NULL;
    int resultado = 0;
    
    ruta_completa = dameNuevoTipo(fichero, ".ok");
    if(!access(ruta_completa, F_OK)){
        resultado = 1;
    }
    free(fich_ok);
    free(ruta_completa);
    return resultado;
}

/*
    return: 1 OK
            0 no es del tipo especificado
            -1 error
*/
int
esTipo(char *fichero, char *tipo, char *directorio)
{
    char *ruta_completa = NULL;
    int longitud1, longitud2, es_tipo = 0;
    char *tipo_fichero = NULL;
    struct stat bufferdatos;

    ruta_completa = dameRutaCompleta(fichero, directorio);
    if(stat(ruta_completa, &bufferdatos)>=0){
        if(S_ISREG(bufferdatos.st_mode)){
            tipo_fichero = strrchr(fichero, '.');
            if(tipo_fichero){
                longitud1 = strlen(tipo_fichero);
                longitud2 = strlen(tipo);
                if(longitud1 == longitud2 && !strcmp(tipo, tipo_fichero))
                    es_tipo++;
            }
        }
    }else{
        warnx("Error: could stat %s. No type matches.", ruta_completa);
        free(ruta_completa);
        return -1;
    }
    free(ruta_completa);
    return es_tipo;
}

/*
    Rellena el array de punteros con el tipo de archivo encontrados
    en la ruta especificada
    return: num de archivos encontrados
*/
char **
dameArchivos(char *tipo, char *ruta)
{
    DIR *directorio;
    struct stat bufferdatos;
    struct dirent *carpeta;
    char *nombre = NULL;
    char **elementos = NULL;
    int num=0;

    if(stat(ruta, &bufferdatos) >= 0){
        if(S_ISDIR(bufferdatos.st_mode)){
            directorio = opendir(ruta);
            if(!directorio){
                errx(1,"Error: cannot open %s.", ruta);
            }
            while((carpeta = readdir(directorio))){
                if(esTipo(carpeta->d_name, tipo, ruta)){
                    nombre = dameRutaCompleta(carpeta->d_name, ruta);
                    if(nombre){
                        if(!(elementos = (char **)realloc(elementos, (num+1)*sizeof(char *)))){
                            warnx("Error %s: realloc: heap memory error", nombre);
                        } else {
                            elementos[num] = nombre;
                            num++;
                        }
                        nombre = NULL;
                    }
                }
            }
            if(num){
                if(!(elementos = (char **)realloc(elementos, (num+1)*sizeof(char *)))){
                    warnx("Error : realloc: heap memory error");
                } else {
                    elementos[num] = NULL;
                }
            }
            closedir(directorio);
        }
    } else {
        warnx("%s not a valid directory.", ruta);
        return NULL;
    }
    
    return elementos;
}

/*
    Rellena una lista con el contenido del fichero
    return: num de lineas con elementos leidos
*/
int
dameContenidoTest(FILE *file, LISTA *lista)
{
    char bufferlinea[BUFFSIZE];
    char **argumentos = NULL;
    int n_elementos=0;

    reiniciarBuffer(bufferlinea);
    while((fgets(bufferlinea, BUFFSIZE, file))){
        if(strlen(bufferlinea) != 0){
            bufferlinea[strlen(bufferlinea)-1] = '\0';
            argumentos = tokenizar(bufferlinea, &n_elementos, " ");
            if(n_elementos){
                //imprimir(argumentos);
                agregarElemento(argumentos, lista);
            } else {
                free(argumentos);
            }
            argumentos = NULL;
            reiniciarBuffer(bufferlinea);
        }
    }
    return n_elementos;
}

/*
    Crea el tipo de salida especificado
    return  fd: de la salida
            -1: error
*/
int
crearFSalida(char *fichero, char *salida)
{
    int fdsalida = 0;
    char *fich_aux;

    fich_aux = dameNuevoTipo(fichero, salida);

    if((fdsalida = open(fich_aux, O_CREAT | O_TRUNC | O_WRONLY, 0664))<0){
        free(fich_aux);
        warnx("Open Error: cannot open [%s]", fich_aux);
        return -1;
    }
    free(fich_aux);
    return fdsalida;
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
    return num_pipes;
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

int
ejecutarComando(NODO *comando, int num_hijo, int pipes[][2], int num_pipes, int fdout)
{
    int i = num_hijo, fd;
    char *cmd = NULL;

    if(num_hijo==0){ //Primer hijo
        if((fd = open("/dev/null", O_CREAT | O_APPEND | O_WRONLY, 0664))<0){
            warn("La entrada no han podido ser redirigidos a dev/null\n");
        } else {
            dup2(fd, 0);
        }
        if(num_pipes>0 && num_hijo < num_pipes){
            dup2(pipes[i][1], 1);
        } else if(num_hijo == num_pipes){
            dup2(fdout, 2);
            dup2(fdout, 1);
        }
    } else {
        dup2(pipes[i-1][0], 0);
        if(i < num_pipes){
            dup2(pipes[i][1],1);
        } else if(i == num_pipes){
            dup2(fdout, 2);
            dup2(fdout, 1);
        }
    }
    cerrarPipes(pipes, num_pipes);

    cmd = dameComando(comando->comando);
    if(cmd){
        comando->comando = cmd;
        comando->argumentos[0] = cmd;
        return execv(comando->comando, comando->argumentos);
    }
    return -1;
}

//Devuelve el status con el que ha terminado el ultimo comando
int
pipeline(LISTA *contenido, char *fichero, int fdout){
    int i = 0, status = 0, pid;
    int num_pipes = (contenido->tam)-1;
    int num_forks = contenido->tam;
    int pipes[num_pipes][2];
    char *cmd = NULL;
    NODO *nodoaux = contenido->primero;

    cmd=dameComando(nodoaux->comando);
    if(cmd && !strcmp(cmd, "cd")){
        mycd(nodoaux->argumentos);
        num_forks--;
        num_pipes--;
        nodoaux = nodoaux->siguiente;
    } else {
        free(cmd);
    }

    if(num_pipes>0){
        if(crearPipes(pipes, num_pipes)<0){
            warnx("PIPE ERROR");
            return -1;
        }
    }

    for(; i<num_forks; i++){
        pid = fork();
        switch(pid){
            case -1:
                err(1, "FORK ERROR");
            case 0:
                ejecutarComando(nodoaux, i, pipes, num_pipes, fdout);
                fprintf(stderr, "%s: this command does not exist\n", nodoaux->comando);
                liberarContenido(contenido);
                free(fichero);
                exit(EXIT_FAILURE);
            default:
                ;
        }
        nodoaux = nodoaux->siguiente;
    }
    if(num_pipes>0)
        cerrarPipes(pipes, num_pipes);
    if(num_forks>0)
        waitpid(pid, &status, 0);

    return status;
}

void
copiarArchivo(char *fichero1, char *fichero2)
{
    int fd_read, fd_write, nr;
    char bufferlectura[BUFFSIZE];

    if((fd_read = open(fichero1, O_RDONLY, 0664))<0){
        err(1, "ERROR %s open", fichero1);
    }
    if((fd_write = creat(fichero2, 0664))<0){
        err(1, "ERROR %s open", fichero2);
    }

    reiniciarBuffer(bufferlectura);
    for(;;){
        nr = read(fd_read, bufferlectura, BUFFSIZE-1 );
        if(nr < 0){
            err(1, "ERROR reading %s", fichero1);
        }
        if(nr > 0){
            write(fd_write, bufferlectura, nr);
        }
        if(nr == 0){
            break;  
        }
        reiniciarBuffer(bufferlectura);
    }

    close(fd_read);
    close(fd_write);
}

//1 si los 2 ficheros son iguales, 0 si son distintos
int
comparar(char *fichero1, char *fichero2)
{
    FILE *fp1, *fp2;
    unsigned int char1 = ' ', char2=' ';
    int resultado = 0;

    if(!(fp1 = fopen(fichero1, "r"))){
        warnx("Error al abrir %s para comparar", fichero1);
        return -1;
    }
    if(!(fp2 = fopen(fichero2, "r"))){
        warnx("Error al abrir %s para comparar", fichero1);
        return -1;
    }

    while(char1 != EOF && char2 != EOF && char1 == char2){
        char1 = getc(fp1);
        char2 = getc(fp2);
    }

    if(char1 == char2)
        resultado = 1;

    fclose(fp1);
    fclose(fp2);
    return resultado;
}
/*
    Proceso cada test
    return:  0: OK
            -1: error
*/
int
procesar(char *test)
{
    LISTA contenido;
    FILE *file;
    int fdout, status, resultado = 0;
    char *fichero_ok, *fichero_out;

    inicializarLista(&contenido);

    if(!(file = fopen(test, "r"))){
        warnx("Couldnt open %s", test);
        return -1;
    }

    dameContenidoTest(file, &contenido);
    fclose(file);

    if(contenido.tam > 0){
        if((fdout = crearFSalida(test,".out")) < 0){
            liberarContenido(&contenido);
            resultado = -1;
        }else {
            status = pipeline(&contenido, test, fdout);
            fichero_ok = dameNuevoTipo(test, ".ok");
            fichero_out = dameNuevoTipo(test, ".out");
            if(!existeFOK(test)){
                copiarArchivo(fichero_out, fichero_ok);
            }
            if(!status){
                resultado = comparar(fichero_ok, fichero_out);
                resultado--;
            } else {
                resultado = -1;
            }
        
            free(fichero_ok);
            free(fichero_out);
            close(fdout);
        }
    }
    liberarContenido(&contenido);
    return resultado;
}


void
borrarSalidas(void){
    char **out;
    char **ok;
    char buffer[BUFFSIZE];
    char *working_directory;
    int i = 0;

    working_directory = getcwd(buffer, BUFFSIZE);

    out = dameArchivos(".out", working_directory);
    ok = dameArchivos(".ok", working_directory);

    if(longitudElementos(out)){
        while(out[i]){
            if(unlink(out[i]))
                warnx("cannot delete [%s]", out[i]);
            i++;
        }
        liberar(out);
    }
    i = 0;
    if(longitudElementos(ok)){
        while(ok[i]){
            if(unlink(ok[i]))
                warnx("cannot delete [%s]", ok[i]);
            i++;
        }
        liberar(ok);
    }

}

int
main(int argc, char *argv[])
{
    int pid, status, i=0, resultado = 0, errores = 0;
    int opcion = 0;
    char **tests;
    char *test;
    char buffer[BUFFSIZE];

    char *tipo = ".tst";
    char *ruta = getcwd(buffer, BUFFSIZE);

    argc--;
    argv++;
    //Por ahora descartamos solo tenemos la opcion -c
    if(argc == 1){
        opcion = esOpcionValida(argv[0]);
        if(!opcion){
            errx(1, "Usage: [option] [arguments(-t)]");
        }
        switch(opcion){
            case 2:
                borrarSalidas();
                break;
            default:
                ;
        }
        exit(EXIT_SUCCESS);
    }else if(argc > 1){
        errx(1, "Usage: [option] [arguments(-t)]");
    }

    tests = dameArchivos(tipo, ruta);

    if(tests){
        while(tests[i]){
            pid = fork();
            switch(pid){
                case -1:
                    warnx("Fork Error: Test [%s] failed.", tests[i]);
                    break;
                case 0:
                    test = strdup(tests[i]);
                    liberar(tests);
                    resultado = procesar(test);
                    if(resultado>=0){
                        printf("Test [%s]: [OK]\n", test);
                        free(test);
                        exit(EXIT_SUCCESS);
                    } else {
                        printf("Test [%s]: [BAD]\n", test);
                        free(test);
                        exit(EXIT_FAILURE);
                    }
                default:
                    ;
            }
            i++;
        }
        while(wait(&status)>0){
            if(WEXITSTATUS(status)!=0)
                errores++;
        }
    } else {
        printf("No tests found.\n");
    }

    liberar(tests);
    if(errores)
        exit(EXIT_FAILURE);
    exit(EXIT_SUCCESS);
}