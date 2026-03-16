// Compilar: gcc secuencial.c -o secuencial
// Ejecutar: ./secuencial

//
//    PROYECTO DE LA ASIGNATURA ARQUITECTURA DE ALTAS PRESTACIONES
//
//               APAREAMIENTO DE GRAFOS
//
//    El objetivo es encontrar los apareamientos en un grafo. 
//    De entrada vamos a tener un grafo, que consiste en una matriz bidimensional. 
//    Se indicaran con 1 las aristas entre nodos y con 0 la ausencia de aristas. 

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define N 5 //numero de nodos 

typedef struct aristas{
    char nodov[10];
    char nodoh[10];
    struct aristas* siguiente;
    }aristas;

void crear_grafo(int matriz[N][N]){

    // inicializar generador de números aleatorios
    srand(time(NULL));
    
    // llenar matriz con 0s y 1s aleatorios
    for(int i=0; i<N; i++) {
        for(int j=0; j<N; j++) {
            if(i==j){
                matriz[i][j] = 0; //no puede pasar
            }
            else{
                matriz[i][j] = rand() % 2; // 0 o 1
            }
            
        }
    } 
}

void etiqueta(int n, char *resultado){

    int i = 0;

    while(n >= 0){
        resultado[i++] = 'A' + (n % 26);
        n = n / 26 - 1;
    }

    resultado[i] = '\0';

    // invertir string
    for(int j=0;j<i/2;j++){
        char tmp = resultado[j];
        resultado[j] = resultado[i-1-j];
        resultado[i-1-j] = tmp;
    }
}

aristas* lista_posibles_aristas(int matriz[N][N]){
    aristas* head = NULL;
    char nombrei[10], nombrej[10];

    for(int i=0; i<N; i++){
        for(int j=0; j<N; j++) {
            if (matriz[i][j] == 1 && i!=j){   
                //Guardar nodos en la lista 

                aristas* nuevo = malloc(sizeof(aristas));
                etiqueta(i, nombrei);
                etiqueta(j, nombrej);

                strcpy(nuevo->nodoh, nombrei);
                strcpy(nuevo->nodov, nombrej);
                nuevo->siguiente = NULL;

                if(head == NULL){
                    head = nuevo;
                }
                else{
                    aristas* temp = head;
                    while(temp->siguiente != NULL){
                        temp = temp->siguiente;
                    }
                    temp->siguiente = nuevo;
                }
            }
        }
    }
    return head;
}

void imprimir_lista(aristas* lista){

    aristas* actual = lista;

    while(actual != NULL){

        printf("(%s , %s)\n", actual->nodoh, actual->nodov);

        actual = actual->siguiente;
    }
}

int contar_aristas(aristas* lista){

    int contador = 0;

    aristas* actual = lista;

    while(actual != NULL){
        contador++;
        actual = actual->siguiente;
    }

    return contador;
}

aristas* filtrar_lista(aristas* lista){

    aristas* actual = lista;

    while(actual != NULL){

        aristas* prev = actual;
        aristas* comp = actual->siguiente;

        while(comp != NULL){

            if(strcmp(actual->nodoh, comp->nodov) == 0 &&
               strcmp(actual->nodov, comp->nodoh) == 0){

                printf("Borrando arista duplicada: (%s,%s)\n",
                       comp->nodoh, comp->nodov);

                prev->siguiente = comp->siguiente;
                free(comp);
                comp = prev->siguiente;
            }
            else{
                prev = comp;
                comp = comp->siguiente;
            }
        }

        actual = actual->siguiente;
    }

    return lista;
}

int main (int argc, char *argv[]){
    int grafo[N][N];
    crear_grafo(grafo);

    char nombre[10];

    // imprimir matriz con índices de letras
    char letras[N];

       // inicializar letras
    for(int i=0; i<N; i++) {
        letras[i] = 'A' + i; // 'A','B','C',...
    }

    printf("GRAFO DE %d NODOS\n", N);
    printf("   ");
    // encabezados
    for(int j=0; j<N; j++) {
        etiqueta(j,nombre);
        printf("%s ", nombre);
    }
    printf("\n");

    //numeros
    for(int i=0; i<N; i++) {
        etiqueta(i,nombre);
        printf("%s ", nombre);
        for(int j=0; j<N; j++) {
            printf("%d ", grafo[i][j]);
        }
        printf("\n");
    }

    //Buscar todas las aristas posibles, las que tengan uno

    aristas* lista;
    lista = lista_posibles_aristas(grafo);

    //Inprimir posibles aristas 
    printf("Aristas posibles\n");
    imprimir_lista(lista);
    printf("En total hay %d posibles aristas\n", contar_aristas(lista));


    //Filtrar, para que no se repitan
    aristas* filtrado;
    filtrado = filtrar_lista(lista);
    printf("Aristas únicas\n");
    //Mostrar de nuevo
    imprimir_lista(filtrado);
    printf("En total hay %d aristas únicas\n", contar_aristas(filtrado));

    
    return 0;
}



