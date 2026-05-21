#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <omp.h>

/*
   PROYECTO DE LA ASIGNATURA ARQUITECTURA DE ALTAS PRESTACIONES
   PRACTICA 2 - VERSION OPENMP
   PROBLEMA: APAREAMIENTO EN GRAFOS

   Compilar en GCC:
       gcc -fopenmp grafo_openmp_aleatorio.c -o grafo_openmp_aleatorio
       
   Opcional: export OMP_NUM_THREADS=4
   
   Ejecutar en GCC:
       ./grafo_openmp_aleatorio
*/

#define N 5
#define MAX_EDGES (N * (N - 1) / 2)
#define PROFUNDIDAD_CORTE 2 // hasta qué nivel del backtracking se crean tareas OpenMP

// Estructura para representar una arista del grafo, con los indices de los nodos que conecta:
typedef struct {
    int u;
    int v;
} Arista;

typedef struct {
    int total_apareamientos;
    int tam_maximo;
    int total_maximos;
} Resultado;

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
        matriz[i][i] = 0;

        for (int j = i + 1; j < N; j++) {
            int valor = rand() % 2;
            matriz[i][j] = valor;
            matriz[j][i] = valor;
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

// Funcion para combinar los resultados locales de cada hilo en el resultado global, usando una seccion critica para evitar condiciones de carrera:
void combinar_resultados(Resultado *global, Resultado local) {
    #pragma omp critical // solo un hilo a la vez puede entrar a esta sección
    {
        global->total_apareamientos += local.total_apareamientos; // sumamos el total de apareamientos encontrados por este hilo al global

        if (local.tam_maximo > global->tam_maximo) {
            global->tam_maximo = local.tam_maximo;
            global->total_maximos = local.total_maximos;
        } else if (local.tam_maximo == global->tam_maximo) {
            global->total_maximos += local.total_maximos;
        }
    }
}

// Funcion recursiva para buscar apareamientos de forma secuencial, usada por cada hilo para explorar su parte del espacio de soluciones:
void buscar_apareamientos_secuencial(
    Arista aristas[],
    int total_aristas,
    int indice,
    int usados[],
    int seleccion[],
    int tam_seleccion,
    Resultado *res
) {
    if (indice == total_aristas) {
        if (tam_seleccion > 0) {
            res->total_apareamientos++;

            if (tam_seleccion > res->tam_maximo) {
                res->tam_maximo = tam_seleccion;
                res->total_maximos = 1;
            } else if (tam_seleccion == res->tam_maximo) {
                res->total_maximos++;
            }
        }
        return;
    }

    buscar_apareamientos_secuencial(
        aristas,
        total_aristas,
        indice + 1,
        usados,
        seleccion,
        tam_seleccion,
        res
    );

    if (!usados[aristas[indice].u] && !usados[aristas[indice].v]) {
        usados[aristas[indice].u] = 1;
        usados[aristas[indice].v] = 1;
        seleccion[tam_seleccion] = indice;

        buscar_apareamientos_secuencial(
            aristas,
            total_aristas,
            indice + 1,
            usados,
            seleccion,
            tam_seleccion + 1,
            res
        );

        usados[aristas[indice].u] = 0;
        usados[aristas[indice].v] = 0;
    }
}

// Parte principal de la versión OpenMP:
// Funcion recursiva para buscar apareamientos usando OpenMP, creando tareas para cada rama del backtracking hasta una cierta profundidad para evitar overhead excesivo:
void buscar_apareamientos_openmp(
    Arista aristas[],
    int total_aristas,
    int indice,
    int usados[],
    int seleccion[],
    int tam_seleccion,
    int profundidad,
    Resultado *global
) {
    if (indice == total_aristas || profundidad >= PROFUNDIDAD_CORTE) {
        Resultado local = {0, 0, 0};
        int usados_local[N];
        int seleccion_local[MAX_EDGES];

        memcpy(usados_local, usados, sizeof(int) * N);
        memcpy(seleccion_local, seleccion, sizeof(int) * MAX_EDGES);

        buscar_apareamientos_secuencial(
            aristas,
            total_aristas,
            indice,
            usados_local,
            seleccion_local,
            tam_seleccion,
            &local
        );

        combinar_resultados(global, local);
        return;
    }
    // Creamos una tarea para la rama del backtracking que no incluye la arista actual
    // Usamos el firstprivate para pasar las variables necesarias a la tarea, y shared para el resultado global
    #pragma omp task default(none) firstprivate(total_aristas, indice, tam_seleccion, profundidad) shared(aristas, usados, seleccion, global)
    {
        int usados_no[N];
        int seleccion_no[MAX_EDGES];
        
        // 
        memcpy(usados_no, usados, sizeof(int) * N);
        memcpy(seleccion_no, seleccion, sizeof(int) * MAX_EDGES);

        buscar_apareamientos_openmp(
            aristas,
            total_aristas,
            indice + 1,
            usados_no,
            seleccion_no,
            tam_seleccion,
            profundidad + 1,
            global
        );
    }

    if (!usados[aristas[indice].u] && !usados[aristas[indice].v]) {
        // Esperamos a que la tarea anterior termine antes de modificar el estado compartido para evitar condiciones de carrera, ya que ambos caminos del backtracking modifican el mismo array de usados y seleccion
        #pragma omp task default(none) firstprivate(total_aristas, indice, tam_seleccion, profundidad) shared(aristas, usados, seleccion, global)
        {
            int usados_si[N];
            int seleccion_si[MAX_EDGES];

            memcpy(usados_si, usados, sizeof(int) * N);
            memcpy(seleccion_si, seleccion, sizeof(int) * MAX_EDGES);

            usados_si[aristas[indice].u] = 1;
            usados_si[aristas[indice].v] = 1;
            seleccion_si[tam_seleccion] = indice;

            buscar_apareamientos_openmp(
                aristas,
                total_aristas,
                indice + 1,
                usados_si,
                seleccion_si,
                tam_seleccion + 1,
                profundidad + 1,
                global
            );
        }
    }

    #pragma omp taskwait
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

int main(void) {
    srand(time(NULL));

    int grafo[N][N];
    Arista aristas[MAX_EDGES];
    int total_aristas;
    int usados[N] = {0};
    int seleccion[MAX_EDGES] = {0};
    Resultado resultado = {0, 0, 0};

    crear_grafo(grafo);
    imprimir_grafo(grafo);

    total_aristas = obtener_aristas_unicas(grafo, aristas);
    imprimir_aristas(aristas, total_aristas);

    #pragma omp parallel
    {
        #pragma omp single
        {
            buscar_apareamientos_openmp(
                aristas,
                total_aristas,
                0,
                usados,
                seleccion,
                0,
                0,
                &resultado
            );
        }
    }

    printf("\nRESUMEN\n");
    printf("Numero total de apareamientos no vacios: %d\n", resultado.total_apareamientos);
    printf("Tamaño del apareamiento maximo: %d\n", resultado.tam_maximo);
    printf("Cantidad de apareamientos maximos: %d\n", resultado.total_maximos);

    printf("\nAPAREAMIENTOS MAXIMOS\n");
    mostrar_apareamientos_maximos(aristas, total_aristas, resultado.tam_maximo);

    return 0;
}
