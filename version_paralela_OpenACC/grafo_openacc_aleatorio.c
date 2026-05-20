#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
   PROYECTO DE LA ASIGNATURA ARQUITECTURA DE ALTAS PRESTACIONES
   PRACTICA 4 - VERSION OpenACC
   PROBLEMA: APAREAMIENTO EN GRAFOS

   Compilar en el equipo del aula con soporte OpenACC:
       gcc -fopenacc -foffload=nvptx-none -fcf-protection=none -no-pie grafo_openacc_aleatorio.c -o grafo_openacc_aleatorio

   Ejecutar:
       ./grafo_openacc_aleatorio

   Opcional para perfilar:
       nvprof ./grafo_openacc_aleatorio
*/

#define N 5
#define MAX_EDGES (N * (N - 1) / 2)
#define MAX_SUBCONJUNTOS (1 << MAX_EDGES)

/*
   Estructura para representar una arista del grafo.
*/
typedef struct {
    int u;
    int v;
} Arista;


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
   Crea un grafo aleatorio no dirigido mediante una matriz de adyacencia.
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
   Imprime la matriz de adyacencia del grafo.
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
   Obtiene las aristas unicas del grafo recorriendo solo la parte superior
   de la matriz, para evitar duplicados en un grafo no dirigido.
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
   Imprime las aristas del grafo.
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
   Imprime un apareamiento a partir de una mascara de bits.
   Si el bit i esta activo, la arista i forma parte del apareamiento.
*/
void imprimir_apareamiento_mascara(Arista aristas[], int total_aristas, int mascara) {
    char a[10], b[10];
    int primero = 1;

    printf("{ ");

    for (int i = 0; i < total_aristas; i++) {
        if (mascara & (1 << i)) {
            etiqueta(aristas[i].u, a);
            etiqueta(aristas[i].v, b);

            if (!primero) {
                printf(", ");
            }

            printf("(%s,%s)", a, b);
            primero = 0;
        }
    }

    printf(" }");
}

/*
   Comprueba de forma secuencial si una mascara representa un apareamiento.
   Se usa solo para imprimir los apareamientos maximos al final.
*/
int es_apareamiento_valido_cpu(Arista aristas[], int total_aristas, int mascara, int *tam) {
    int vertices_usados = 0;
    *tam = 0;

    for (int i = 0; i < total_aristas; i++) {
        if (mascara & (1 << i)) {
            int bit_u = 1 << aristas[i].u;
            int bit_v = 1 << aristas[i].v;

            if ((vertices_usados & bit_u) || (vertices_usados & bit_v)) {
                return 0;
            }

            vertices_usados |= bit_u;
            vertices_usados |= bit_v;
            (*tam)++;
        }
    }

    return (*tam > 0);
}

/*
   Version OpenACC para contar todos los apareamientos.

   En las versiones secuencial, OpenMP y MPI se uso backtracking.
   En OpenACC se usa una estrategia mas adecuada para GPU:
   enumerar en paralelo todos los subconjuntos posibles de aristas.

   Cada subconjunto se representa como una mascara de bits y cada iteracion
   del bucle comprueba si ese subconjunto es un apareamiento valido.
*/
void buscar_apareamientos_openacc(
    int u[],
    int v[],
    int total_aristas,
    int *total_apareamientos,
    int *tam_maximo,
    int *total_maximos
) {
    int total_subconjuntos = 1 << total_aristas;

    int total = 0;
    int maximo = 0;

    /*
       Primera pasada en GPU:
       - cuenta todos los apareamientos no vacios
       - calcula el tamaño maximo encontrado

       copyin copia los arreglos u y v hacia la GPU.
       reduction evita condiciones de carrera al acumular los resultados.
    */
    #pragma acc data copyin(u[0:total_aristas], v[0:total_aristas])
    {
        #pragma acc parallel loop reduction(+:total) reduction(max:maximo)
        for (int mascara = 1; mascara < total_subconjuntos; mascara++) {
            int vertices_usados = 0;
            int valido = 1;
            int tam = 0;

            for (int i = 0; i < total_aristas; i++) {
                if (mascara & (1 << i)) {
                    int bit_u = 1 << u[i];
                    int bit_v = 1 << v[i];

                    if ((vertices_usados & bit_u) || (vertices_usados & bit_v)) {
                        valido = 0;
                    }
                    else {
                        vertices_usados |= bit_u;
                        vertices_usados |= bit_v;
                        tam++;
                    }
                }
            }

            if (valido && tam > 0) {
                total++;

                if (tam > maximo) {
                    maximo = tam;
                }
            }
        }

        int maximos = 0;

        /*
           Segunda pasada en GPU:
           una vez conocido el tamaño maximo, se cuentan solamente
           los apareamientos cuyo tamaño coincide con ese maximo.
        */
        #pragma acc parallel loop reduction(+:maximos)
        for (int mascara = 1; mascara < total_subconjuntos; mascara++) {
            int vertices_usados = 0;
            int valido = 1;
            int tam = 0;

            for (int i = 0; i < total_aristas; i++) {
                if (mascara & (1 << i)) {
                    int bit_u = 1 << u[i];
                    int bit_v = 1 << v[i];

                    if ((vertices_usados & bit_u) || (vertices_usados & bit_v)) {
                        valido = 0;
                    }
                    else {
                        vertices_usados |= bit_u;
                        vertices_usados |= bit_v;
                        tam++;
                    }
                }
            }

            if (valido && tam == maximo) {
                maximos++;
            }
        }

        *total_maximos = maximos;
    }

    *total_apareamientos = total;
    *tam_maximo = maximo;
}

/*
   Muestra los apareamientos maximos en CPU.
   La impresion se mantiene secuencial para evitar overhead y para que la
   salida sea ordenada, igual que en las versiones OpenMP y MPI.
*/
void mostrar_apareamientos_maximos(Arista aristas[], int total_aristas, int tam_objetivo) {
    int total_subconjuntos = 1 << total_aristas;
    int contador = 0;

    for (int mascara = 1; mascara < total_subconjuntos; mascara++) {
        int tam = 0;

        if (es_apareamiento_valido_cpu(aristas, total_aristas, mascara, &tam) && tam == tam_objetivo) {
            contador++;
            printf("Maximo %d = ", contador);
            imprimir_apareamiento_mascara(aristas, total_aristas, mascara);
            printf("\n");
        }
    }
}

int main(void) {
    int grafo[N][N];
    Arista aristas[MAX_EDGES];

    int u[MAX_EDGES];
    int v[MAX_EDGES];

    int total_aristas;
    int total_apareamientos = 0;
    int tam_maximo = 0;
    int total_maximos = 0;

    /*
       Semilla fija para facilitar la comparacion con las otras versiones.
       Si se quiere un grafo diferente en cada ejecucion, se puede usar:
       srand(time(NULL));
    */
    srand(1);

    crear_grafo(grafo);
    imprimir_grafo(grafo);

    total_aristas = obtener_aristas_unicas(grafo, aristas);
    imprimir_aristas(aristas, total_aristas);

    /*
       Se separan los extremos de las aristas en dos arreglos simples.
       Esto facilita la transferencia de datos a la GPU con OpenACC.
    */
    for (int i = 0; i < total_aristas; i++) {
        u[i] = aristas[i].u;
        v[i] = aristas[i].v;
    }

    buscar_apareamientos_openacc(
        u,
        v,
        total_aristas,
        &total_apareamientos,
        &tam_maximo,
        &total_maximos
    );

    printf("\nRESUMEN\n");
    printf("Numero total de apareamientos no vacios: %d\n", total_apareamientos);
    printf("Tamaño del apareamiento maximo: %d\n", tam_maximo);
    printf("Cantidad de apareamientos maximos: %d\n", total_maximos);

    printf("\nAPAREAMIENTOS MAXIMOS\n");
    mostrar_apareamientos_maximos(aristas, total_aristas, tam_maximo);

    return 0;
}
