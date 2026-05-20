#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import re
import time
import shutil
import argparse
import subprocess
from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt


# ==========================================================
# CONFIGURACION DE VERSIONES
# ==========================================================

VERSIONES = {
    "secuencial": {
        "src": "version_secuencial/grafo_secuencial_aleatorio_comparar.c",
        "compiler": "gcc",
        "compile_flags": [],
        "run_mode": "normal"
    },

    "openmp": {
        "src": "version_paralela_OpenMP/grafo_openmp_aleatorio.c",
        "compiler": "gcc",
        "compile_flags": ["-fopenmp"],
        "run_mode": "openmp"
    },

    "mpi": {
        "src": "version_paralela_MPI/grafo_mpi_aleatorio.c",
        "compiler": "mpicc",
        "compile_flags": [],
        "run_mode": "mpi"
    },

    "openacc": {
        "src": "version_paralela_OpenACC/grafo_openacc_aleatorio.c",
        "compiler": "gcc",
        "compile_flags": [
            "-fopenacc",
            "-foffload=nvptx-none",
            "-fcf-protection=none",
            "-no-pie"
        ],
        "run_mode": "openacc"
    }
}


CARPETA_BUILD = Path("build_benchmark")
CARPETA_RESULTADOS = Path("resultados")


# ==========================================================
# ARGUMENTOS
# ==========================================================

parser = argparse.ArgumentParser(
    description="Benchmark de versiones secuencial, OpenMP, MPI y OpenACC"
)

parser.add_argument(
    "--versiones",
    nargs="+",
    choices=["secuencial", "openmp", "mpi", "openacc"],
    required=True,
    help="Versiones a ejecutar"
)

parser.add_argument(
    "--nodos",
    nargs="+",
    type=int,
    required=True,
    help="Valores de N a probar"
)

parser.add_argument(
    "--nucleos",
    nargs="+",
    type=int,
    required=True,
    help="Numero de hilos/procesos a probar"
)

parser.add_argument(
    "--repeticiones",
    type=int,
    default=3,
    help="Numero de repeticiones por experimento"
)

args = parser.parse_args()

VERSIONES_A_EJECUTAR = args.versiones
VALORES_N = args.nodos
NUCLEOS = args.nucleos
REPETICIONES = args.repeticiones


# ==========================================================
# FUNCIONES AUXILIARES
# ==========================================================

def preparar_carpetas():
    CARPETA_BUILD.mkdir(exist_ok=True)
    CARPETA_RESULTADOS.mkdir(exist_ok=True)


def crear_fuente_temporal(src_original: Path, nombre_version: str, n: int) -> Path:

    texto = src_original.read_text(encoding="utf-8", errors="ignore")

    texto_modificado = re.sub(
        r"^\s*#define\s+N\s+\d+",
        f"#define N {n}",
        texto,
        flags=re.MULTILINE
    )

    src_temp = CARPETA_BUILD / f"{nombre_version}_N{n}.c"

    src_temp.write_text(texto_modificado, encoding="utf-8")

    return src_temp


def compilar(nombre_version: str, config: dict, n: int) -> Path:

    src_original = Path(config["src"])

    src_temp = crear_fuente_temporal(
        src_original,
        nombre_version,
        n
    )

    ejecutable = CARPETA_BUILD / f"{nombre_version}_N{n}"

    cmd = [
        config["compiler"],
        *config["compile_flags"],
        str(src_temp),
        "-o",
        str(ejecutable)
    ]

    print("\nCompilando:")
    print(" ".join(cmd))

    subprocess.run(
        cmd,
        check=True
    )

    return ejecutable


def ejecutar(ejecutable: Path, run_mode: str, nucleos: int):

    env = os.environ.copy()

    if run_mode == "openmp":

        env["OMP_NUM_THREADS"] = str(nucleos)

        cmd = [str(ejecutable)]

    elif run_mode == "mpi":

        cmd = [
            "mpirun",
            "-np",
            str(nucleos),
            str(ejecutable)
        ]

    elif run_mode == "openacc":

        env["OMP_NUM_THREADS"] = str(nucleos)

        cmd = [str(ejecutable)]

    else:

        cmd = [str(ejecutable)]

    inicio = time.perf_counter()

    proceso = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        env=env
    )

    fin = time.perf_counter()

    tiempo = fin - inicio

    return proceso.returncode, tiempo, proceso.stdout, proceso.stderr, " ".join(cmd)


def extraer_resultados(stdout: str):

    total_aristas = None
    total_apareamientos = None
    tam_maximo = None
    total_maximos = None

    m = re.search(r"Total de aristas unicas:\s*(\d+)", stdout)
    if m:
        total_aristas = int(m.group(1))

    m = re.search(r"Numero total de apareamientos no vacios:\s*(\d+)", stdout)
    if m:
        total_apareamientos = int(m.group(1))

    m = re.search(r"Tamaño del apareamiento maximo:\s*(\d+)", stdout)
    if m:
        tam_maximo = int(m.group(1))

    m = re.search(r"Cantidad de apareamientos maximos:\s*(\d+)", stdout)
    if m:
        total_maximos = int(m.group(1))

    return (
        total_aristas,
        total_apareamientos,
        tam_maximo,
        total_maximos
    )


def calcular_metricas(df):

    resumen = (
        df.groupby(
            ["version", "N", "nucleos"],
            as_index=False
        )
        .agg(
            tiempo_promedio_segundos=("tiempo_segundos", "mean"),
            tiempo_minimo_segundos=("tiempo_segundos", "min"),
            tiempo_maximo_segundos=("tiempo_segundos", "max")
        )
    )

    base = (
        resumen[resumen["version"] == "secuencial"]
        .groupby("N")["tiempo_promedio_segundos"]
        .min()
        .to_dict()
    )

    resumen["tiempo_base_secuencial"] = resumen["N"].map(base)

    resumen["speedup"] = (
        resumen["tiempo_base_secuencial"] /
        resumen["tiempo_promedio_segundos"]
    )

    resumen["eficiencia"] = (
        resumen["speedup"] /
        resumen["nucleos"]
    )

    return resumen


def generar_graficas(resumen):

    """
    Genera una grafica de speedup para cada valor de N.

    Esto evita mezclar diferentes tamaños de entrada
    en una misma linea, lo cual producia graficas
    confusas e incorrectas.
    """

    # Recorre cada valor distinto de N.
    for n in sorted(resumen["N"].unique()):

        plt.figure()

        # Filtra solo las filas correspondientes a ese N
        # y excluye la version secuencial.
        datos_n = resumen[
            (resumen["N"] == n) &
            (resumen["version"] != "secuencial")
        ]

        # Recorre cada version paralela.
        for version in datos_n["version"].unique():

            datos_version = datos_n[
                datos_n["version"] == version
            ]

            # Ordena por nucleos para que la linea quede correcta.
            datos_version = datos_version.sort_values("nucleos")

            plt.plot(
                datos_version["nucleos"],
                datos_version["speedup"],
                marker="o",
                label=version
            )

        plt.xlabel("Nucleos / procesos")
        plt.ylabel("Speedup")

        plt.title(f"Speedup para N={n}")

        plt.legend()

        plt.grid(True)

        plt.tight_layout()

        # Guarda una grafica distinta por cada N.
        plt.savefig(
            CARPETA_RESULTADOS / f"grafica_speedup_N{n}.png",
            dpi=200
        )

        plt.close()


def guardar_resultados(df, resumen):

    csv_detalle = (
        CARPETA_RESULTADOS /
        "rendimiento_grafos_detalle.csv"
    )

    csv_resumen = (
        CARPETA_RESULTADOS /
        "rendimiento_grafos_resumen.csv"
    )

    xlsx = (
        CARPETA_RESULTADOS /
        "rendimiento_grafos.xlsx"
    )

    df.to_csv(
        csv_detalle,
        index=False,
        encoding="utf-8-sig"
    )

    resumen.to_csv(
        csv_resumen,
        index=False,
        encoding="utf-8-sig"
    )

    with pd.ExcelWriter(
        xlsx,
        engine="openpyxl"
    ) as writer:

        df.to_excel(
            writer,
            index=False,
            sheet_name="detalle"
        )

        resumen.to_excel(
            writer,
            index=False,
            sheet_name="resumen"
        )

    print("\nArchivos guardados correctamente.")


# ==========================================================
# MAIN
# ==========================================================

def main():

    preparar_carpetas()

    filas = []

    for n in VALORES_N:

        print("\n================================")
        print(f"PRUEBAS PARA N={n}")
        print("================================")

        ejecutables = {}

        for nombre_version in VERSIONES_A_EJECUTAR:

            config = VERSIONES[nombre_version]

            try:

                ejecutables[nombre_version] = compilar(
                    nombre_version,
                    config,
                    n
                )

            except Exception as e:

                print(
                    f"ERROR compilando {nombre_version}: {e}"
                )

        for nombre_version in VERSIONES_A_EJECUTAR:

            if nombre_version not in ejecutables:
                continue

            config = VERSIONES[nombre_version]

            ejecutable = ejecutables[nombre_version]

            run_mode = config["run_mode"]

            nucleos_a_probar = NUCLEOS

            if run_mode == "normal":
                nucleos_a_probar = [1]

            for nucleos in nucleos_a_probar:

                for rep in range(1, REPETICIONES + 1):

                    print(
                        f"\nEjecutando:"
                        f" version={nombre_version}"
                        f" N={n}"
                        f" nucleos={nucleos}"
                        f" repeticion={rep}"
                    )

                    (
                        returncode,
                        tiempo,
                        stdout,
                        stderr,
                        comando
                    ) = ejecutar(
                        ejecutable,
                        run_mode,
                        nucleos
                    )

                    (
                        total_aristas,
                        total_apareamientos,
                        tam_maximo,
                        total_maximos
                    ) = extraer_resultados(stdout)

                    filas.append({
                        "version": nombre_version,
                        "N": n,
                        "nucleos": nucleos,
                        "repeticion": rep,
                        "tiempo_segundos": tiempo,
                        "tiempo_minutos": tiempo / 60,
                        "ok": returncode == 0,
                        "total_aristas": total_aristas,
                        "total_apareamientos": total_apareamientos,
                        "tam_maximo": tam_maximo,
                        "total_maximos": total_maximos,
                        "comando": comando
                    })

    df = pd.DataFrame(filas)

    resumen = calcular_metricas(df)

    guardar_resultados(df, resumen)

    generar_graficas(resumen)

    print("\nBenchmark terminado.")


if __name__ == "__main__":
    main()
