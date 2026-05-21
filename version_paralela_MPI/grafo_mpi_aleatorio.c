#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

/*
   PROYECTO DE LA ASIGNATURA ARQUITECTURA DE ALTAS PRESTACIONES
   PRACTICA 3 - VERSION MPI
   PROBLEMA: APAREAMIENTO EN GRAFOS

   Compilar:
       mpicc grafo_mpi_aleatorio.c -o grafo_mpi_aleatorio

   Ejecutar:
       mpirun -np 4 ./grafo_mpi_aleatorio
*/

#define N 5
#define MAX_EDGES (N * (N - 1) / 2)

/*
   Estructura para representar una arista.
*/
typedef struct {
    int u;
    int v;
} Arista;

/*
   Convierte un numero a formato alfabetico.
*/
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

/*
   Crea un grafo aleatorio no dirigido.
*/
void crear_grafo(int matriz[N][N]) {

    for (int i = 0; i < N; i++) {

        matriz[i][i] = 0;

        for (int j = i + 1; j < N; j++) {

            int valor = rand() % 2;

            matriz[i][j] = valor;
            matriz[j][i] = valor;
        }
    }
}

/*
   Imprime la matriz de adyacencia.
*/
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

/*
   Obtiene las aristas unicas.
*/
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

/*
   Imprime las aristas.
*/
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

/*
   Imprime un apareamiento.
*/
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

/*
   Backtracking secuencial.

   IMPORTANTE:
   No se imprimen todos los apareamientos para evitar
   overhead de salida y permitir comparaciones mas justas
   entre la version secuencial, OpenMP y MPI.
*/
void buscar_apareamientos(
    Arista aristas[],
    int total_aristas,
    int indice,
    int usados[],
    int seleccion[],
    int tam_seleccion,
    int *total_apareamientos,
    int *tam_maximo,
    int *total_maximos
) {

    if (indice == total_aristas) {

        if (tam_seleccion > 0) {

            (*total_apareamientos)++;

            if (tam_seleccion > *tam_maximo) {

                *tam_maximo = tam_seleccion;
                *total_maximos = 1;
            }
            else if (tam_seleccion == *tam_maximo) {

                (*total_maximos)++;
            }
        }

        return;
    }

    /*
       Caso 1:
       No incluir la arista actual.
    */
    buscar_apareamientos(
        aristas,
        total_aristas,
        indice + 1,
        usados,
        seleccion,
        tam_seleccion,
        total_apareamientos,
        tam_maximo,
        total_maximos
    );

    /*
       Caso 2:
       Incluir la arista actual si sus vertices
       aun no han sido usados.
    */
    if (!usados[aristas[indice].u] &&
        !usados[aristas[indice].v]) {

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
            total_maximos
        );

        usados[aristas[indice].u] = 0;
        usados[aristas[indice].v] = 0;
    }
}

/*
   Muestra los apareamientos maximos.
*/
void mostrar_apareamientos_maximos(
    Arista aristas[],
    int total_aristas,
    int tam_objetivo
) {

    int usados[N] = {0};

    int seleccion[MAX_EDGES];

    int stack_indice[MAX_EDGES + 1];
    int stack_tam[MAX_EDGES + 1];

    int top = 0;
    int indice = 0;
    int tam = 0;

    int contador = 0;

    while (1) {

        while (indice < total_aristas) {

            if (!usados[aristas[indice].u] &&
                !usados[aristas[indice].v] &&
                tam < tam_objetivo) {

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

            printf("Maximo %d = ", contador);

            imprimir_apareamiento(
                aristas,
                seleccion,
                tam
            );

            printf("\n");
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

    int rank;
    int size;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int grafo[N][N];

    Arista aristas[MAX_EDGES];

    int total_aristas;

    int total_apareamientos_local = 0;
    int tam_maximo_local = 0;
    int total_maximos_local = 0;

    int total_apareamientos = 0;
    int tam_maximo = 0;
    int total_maximos = 0;

    /*
       Solo el proceso 0 crea e imprime el grafo.
    */
    if (rank == 0) {

        srand(1);

        crear_grafo(grafo);

        imprimir_grafo(grafo);
    }

    /*
       El proceso 0 comparte el grafo con todos.
    */
    MPI_Bcast(
        grafo,
        N * N,
        MPI_INT,
        0,
        MPI_COMM_WORLD
    );

    total_aristas = obtener_aristas_unicas(
        grafo,
        aristas
    );

    if (rank == 0) {
        imprimir_aristas(aristas, total_aristas);
    }

    /*
       Reparto del trabajo.

       Cada proceso comienza desde una arista distinta.
    */
    for (int i = rank; i < total_aristas; i += size) {

        int usados_local[N] = {0};

        int seleccion_local[MAX_EDGES];

        /*
           La arista inicial se fuerza a formar
           parte del apareamiento.
        */
        usados_local[aristas[i].u] = 1;
        usados_local[aristas[i].v] = 1;

        seleccion_local[0] = i;

        buscar_apareamientos(
            aristas,
            total_aristas,
            i + 1,
            usados_local,
            seleccion_local,
            1,
            &total_apareamientos_local,
            &tam_maximo_local,
            &total_maximos_local
        );
    }

    /*
       Suma el total de apareamientos encontrados
       por todos los procesos.
    */
    MPI_Reduce(
        &total_apareamientos_local,
        &total_apareamientos,
        1,
        MPI_INT,
        MPI_SUM,
        0,
        MPI_COMM_WORLD
    );

    /*
       Obtiene el tamaño maximo global.

       Se usa MPI_Allreduce porque todos los
       procesos necesitan conocer el maximo global.
    */
    MPI_Allreduce(
        &tam_maximo_local,
        &tam_maximo,
        1,
        MPI_INT,
        MPI_MAX,
        MPI_COMM_WORLD
    );

    /*
       Solo se cuentan los maximos locales que
       realmente coinciden con el maximo global.
    */
    int total_maximos_validos = 0;

    if (tam_maximo_local == tam_maximo) {
        total_maximos_validos = total_maximos_local;
    }

    /*
       Suma la cantidad real de apareamientos maximos.
    */
    MPI_Reduce(
        &total_maximos_validos,
        &total_maximos,
        1,
        MPI_INT,
        MPI_SUM,
        0,
        MPI_COMM_WORLD
    );

    /*
       Solo el proceso 0 imprime el resultado final.
    */
    if (rank == 0) {

        printf("\nRESUMEN\n");

        printf(
            "Numero total de apareamientos no vacios: %d\n",
            total_apareamientos
        );

        printf(
            "Tamaño del apareamiento maximo: %d\n",
            tam_maximo
        );

        printf(
            "Cantidad de apareamientos maximos: %d\n",
            total_maximos
        );

        printf("\nAPAREAMIENTOS MAXIMOS\n");

        mostrar_apareamientos_maximos(
            aristas,
            total_aristas,
            tam_maximo
        );
    }

    MPI_Finalize();

    return 0;
}