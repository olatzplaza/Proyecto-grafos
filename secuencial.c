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

#define N 30 //numero de nodos 

void crear_grafo(int matriz[N][N]){

    // inicializar generador de números aleatorios
    srand(time(NULL));
    
    // llenar matriz con 0s y 1s aleatorios
    for(int i=0; i<N; i++) {
        for(int j=0; j<N; j++) {
            matriz[i][j] = rand() % 2; // 0 o 1
        }
    } 
}

int main (int argc, char *argv[]){
    int grafo[N][N];
    crear_grafo(grafo);

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
        printf("%c ", letras[j]);
    }
    printf("\n");

    //numeros
    for(int i=0; i<N; i++) {
        printf("%c ", letras[i]);
        for(int j=0; j<N; j++) {
            printf("%d ", grafo[i][j]);
        }
        printf("\n");
    }

    return 0;
}



