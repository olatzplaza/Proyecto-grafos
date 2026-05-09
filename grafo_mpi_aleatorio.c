#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <time.h>

/*
   PROYECTO DE LA ASIGNATURA ARQUITECTURA DE ALTAS PRESTACIONES
   PRACTICA 3 - VERSION MPI
   PROBLEMA: APAREAMIENTO EN GRAFOS

   Compilar en GCC con MPI:
       mpicc grafo_mpi_aleatorio.c -o grafo_mpi_aleatorio
   
   Ejecutar en GCC con MPI:
       mpirun -np 4 ./grafo_mpi_aleatorio
*/

#define N 5 // Numero de nodos en el grafo
#define MAX_EDGES (N * (N - 1) / 2) // Numero maximo que puede tener un grafo no dirigido de N nodos, 
// con N=5 es 10, osea que como maximo podra haber 10 aristas distintas.

// Estructura para representar una arista del grafo, con los indices de los nodos que conecta:
typedef struct {
    int u;
    int v;
} Arista; 


// Funcion para convertir un numero a su representacion alfabetica (0->A, 1->B, ..., 25->Z, 26->AA, etc.)
void etiqueta(int n, char *resultado) {
    int i = 0;

    while (n >= 0) {
        resultado[i++] = 'A' + (n % 26);
        n = n / 26 - 1;
    }

    resultado[i] = '\0';

    for (int j = 0; j < i / 2; j++) {
        char tmp = resultado[j];
        resultado[j] = resultado[i - 1 - j];
        resultado[i - 1 - j] = tmp;
    }
}

// Funcion para crear un grafo aleatorio no dirigido usando una matriz de adyacencia:
void crear_grafo(int matriz[N][N]) {
    for (int i = 0; i < N; i++) {
        matriz[i][i] = 0;  // No se permiten lazos

        for (int j = i + 1; j < N; j++) {
            int valor = rand() % 2;   // 0 o 1 aleatorio
            matriz[i][j] = valor;
            matriz[j][i] = valor;     // Garantiza simetria
        }
    }
}

// Funcion para imprimir la matriz de adyacencia del grafo:
void imprimir_grafo(int matriz[N][N]) {
    char nombre[10];

    printf("GRAFO DE %d NODOS\n", N);
    printf("    ");
    for (int j = 0; j < N; j++) {
        etiqueta(j, nombre);
        printf("%s ", nombre);
    }
    printf("\n");

    for (int i = 0; i < N; i++) {
        etiqueta(i, nombre);
        printf("%s : ", nombre);
        for (int j = 0; j < N; j++) {
            printf("%d ", matriz[i][j]);
        }
        printf("\n");
    }
}

// Funcion para obtener las aristas unicas del grafo a partir de la matriz de adyacencia:
int obtener_aristas_unicas(int matriz[N][N], Arista aristas[]) {
    int total = 0;

    for (int i = 0; i < N; i++) {
        for (int j = i + 1; j < N; j++) {
            if (matriz[i][j] == 1) {
                aristas[total].u = i;
                aristas[total].v = j;
                total++;
            }
        }
    }

    return total;
}

// Funcion para imprimir las aristas del grafo:
void imprimir_aristas(Arista aristas[], int total) {
    char a[10], b[10];

    printf("\nARISTAS DEL GRAFO\n");
    for (int i = 0; i < total; i++) {
        etiqueta(aristas[i].u, a);
        etiqueta(aristas[i].v, b);
        printf("(%s, %s)\n", a, b);
    }
    printf("Total de aristas unicas: %d\n", total);
}

// Funcion para imprimir un apareamiento dado un array de indices de aristas seleccionadas:
void imprimir_apareamiento(Arista aristas[], int seleccion[], int tam) {
    char a[10], b[10];

    printf("{ ");
    for (int i = 0; i < tam; i++) {
        etiqueta(aristas[seleccion[i]].u, a);
        etiqueta(aristas[seleccion[i]].v, b);
        printf("(%s,%s)", a, b);
        if (i < tam - 1) {
            printf(", ");
        }
    }
    printf(" }");
}

// Funcion recursiva para buscar todos los apareamientos posibles en el grafo usando backtracking:
void buscar_apareamientos(
    Arista aristas[],
    int total_aristas,
    int indice,
    int usados[],
    int seleccion[],
    int tam_seleccion,
    int *total_apareamientos,
    int *tam_maximo,
    int *total_maximos,
    int rank
) {

    if (indice == total_aristas) {
        if (tam_seleccion > 0) {
            (*total_apareamientos)++;
            printf("Apareamiento %d= ", *total_apareamientos);
            imprimir_apareamiento(aristas, seleccion, tam_seleccion);
            printf("\n");

            if (tam_seleccion > *tam_maximo) {
                *tam_maximo = tam_seleccion;
                *total_maximos = 1;
            } else if (tam_seleccion == *tam_maximo) {
                (*total_maximos)++;
            }
        }
        return;
    }

    /* Caso 1: no incluir la arista actual */
    buscar_apareamientos(
        aristas,
        total_aristas,
        indice + 1,
        usados,
        seleccion,
        tam_seleccion,
        total_apareamientos,
        tam_maximo,
        total_maximos,
        rank
    );

    /* Caso 2: incluir la arista actual si sus vertices no han sido usados */
    if (!usados[aristas[indice].u] && !usados[aristas[indice].v]) {
        usados[aristas[indice].u] = 1;
        usados[aristas[indice].v] = 1;
        seleccion[tam_seleccion] = indice;

        buscar_apareamientos(
            aristas,
            total_aristas,
            indice + 1,
            usados,
            seleccion,
            tam_seleccion + 1,
            total_apareamientos,
            tam_maximo,
            total_maximos,
            rank
        );

        usados[aristas[indice].u] = 0;
        usados[aristas[indice].v] = 0;
    }
}

// Funcion para mostrar solo los apareamientos de tamaño maximo encontrados:
void mostrar_apareamientos_maximos(Arista aristas[], int total_aristas, int tam_objetivo) {
    int usados[N] = {0};
    int seleccion[MAX_EDGES];
    int stack_indice[MAX_EDGES + 1];
    int stack_tam[MAX_EDGES + 1];
    int top = 0;
    int indice = 0;
    int tam = 0;
    char a[10], b[10];
    int contador = 0;

    /*
       Pequeño backtracking iterativo para volver a recorrer y mostrar solo
       los apareamientos de tamaño maximo.
    */
    while (1) {
        while (indice < total_aristas) {
            if (!usados[aristas[indice].u] && !usados[aristas[indice].v] && tam < tam_objetivo) {
                usados[aristas[indice].u] = 1;
                usados[aristas[indice].v] = 1;
                seleccion[tam] = indice;
                stack_indice[top] = indice;
                stack_tam[top] = tam;
                top++;
                tam++;
            }
            indice++;
        }

        if (tam == tam_objetivo) {
            contador++;
            printf("Maximo %d = { ", contador);
            for (int i = 0; i < tam; i++) {
                etiqueta(aristas[seleccion[i]].u, a);
                etiqueta(aristas[seleccion[i]].v, b);
                printf("(%s,%s)", a, b);
                if (i < tam - 1) {
                    printf(", ");
                }
            }
            printf(" }\n");
        }

        if (top == 0) {
            break;
        }

        top--;
        indice = stack_indice[top];
        tam = stack_tam[top];
        usados[aristas[indice].u] = 0;
        usados[aristas[indice].v] = 0;
        indice++;
    }
}

int main(int argc, char *argv[]) {
    //Inicializar
    int rank, size; 

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    double inicio = MPI_Wtime(); // Debe estar después de MPI_Init para medir correctamente el tiempo de ejecución

    srand(time(NULL)+rank);
    
    int grafo[N][N];

    Arista aristas[MAX_EDGES];
    int total_aristas;

    int total_apareamientos = 0;
    int tam_maximo = 0;
    int total_maximos = 0;

    int total_apareamientos_local = 0;
    int tam_maximo_local = 0;
    int total_maximos_local = 0;

    //Crear grafo
    if(rank ==0){
        crear_grafo(grafo);
        imprimir_grafo(grafo);
    }
    //Compartir grafo con todos los procesos
    MPI_Bcast(grafo, N*N, MPI_INT, 0, MPI_COMM_WORLD);

    //Obtener aristas unicas del grafo
    total_aristas = obtener_aristas_unicas(grafo, aristas);
  
    if(rank == 0){        
        imprimir_aristas(aristas, total_aristas);
        //Repartir aristas
        //printf("\nAPAREAMIENTOS ENCONTRADOS\n");
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    for(int i = rank; i < total_aristas; i += size){
        //Cada proceso busca apareamientos a partir de su arista asignada
        int usados_local[N];
        int seleccion_local[MAX_EDGES];

        memset(usados_local, 0, sizeof(usados_local));

        usados_local[aristas[i].u] = 1;
        usados_local[aristas[i].v] = 1;

        seleccion_local[0] = i;

        buscar_apareamientos(
            aristas,
            total_aristas,
            i,
            usados_local,
            seleccion_local,
            0,
            &total_apareamientos_local,
            &tam_maximo_local,
            &total_maximos_local,
            rank
        );

    }

    // Reducir resultados a nivel global
    MPI_Reduce(&total_apareamientos_local, &total_apareamientos, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&tam_maximo_local, &tam_maximo, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&total_maximos_local, &total_maximos, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("\nRESUMEN\n");
        printf("Numero total de apareamientos no vacios: %d\n", total_apareamientos);
        printf("Tamaño del apareamiento maximo: %d\n", tam_maximo);
        printf("Cantidad de apareamientos maximos: %d\n", total_maximos);

        printf("\nAPAREAMIENTOS MAXIMOS\n");
        mostrar_apareamientos_maximos(aristas, total_aristas, tam_maximo);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    double fin = MPI_Wtime();
    double tiempo = fin - inicio;
    printf("Proceso %d: Tiempo de ejecucion = %f segundos\n", rank, tiempo);

    MPI_Finalize();

    return 0;
}
