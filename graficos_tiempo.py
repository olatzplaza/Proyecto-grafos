import os
import pandas as pd
import matplotlib.pyplot as plt

def generar_graficos_rendimiento(archivo_csv):
    # 1. Leer el archivo CSV usando una ruta relativa
    if not os.path.exists(archivo_csv):
        print(f"Error: No se encontró el archivo '{archivo_csv}' en el directorio actual.")
        return
    
    df = pd.read_csv(archivo_csv)
    
    # 2. Obtener los valores únicos de N para iterar y crear un gráfico por cada uno
    valores_n = sorted(df['N'].unique())
    
    # Definir paleta de colores y marcadores estables para cada versión
    estilos = {
        'secuencial': {'color': '#7f8c8d', 'marker': 'o', 'linestyle': '--', 'label': 'Secuencial'},
        'openmp':     {'color': '#2980b9', 'marker': 's', 'linestyle': '-',  'label': 'OpenMP'},
        'mpi':        {'color': '#e74c3c', 'marker': '^', 'linestyle': '-',  'label': 'MPI'},
        'openacc':    {'color': '#27ae60', 'marker': 'd', 'linestyle': '-',  'label': 'OpenACC'}
    }
    
    # 3. Generar un gráfico independiente por cada N
    for n in valores_n:
        df_n = df[df['N'] == n]
        
        plt.figure(figsize=(8, 5), dpi=150)
        
        # Agrupar por versión y graficar
        for version, datos_version in df_n.groupby('version'):
            # Ordenamos por número de núcleos para asegurar el flujo correcto de la línea
            datos_version = datos_version.sort_values('nucleos')
            
            # Obtener el estilo correspondiente (usar uno por defecto si no existe)
            estilo = estilos.get(version, {'color': '#34495e', 'marker': 'x', 'linestyle': '-.', 'label': version})
            
            plt.plot(
                datos_version['nucleos'], 
                datos_version['tiempo_segundos'], 
                marker=estilo['marker'], 
                linestyle=estilo['linestyle'], 
                color=estilo['color'], 
                linewidth=1.8,
                markersize=6,
                label=estilo['label']
            )
        
        # Configuración estética del gráfico
        plt.title(f'Evolución del Tiempo de Ejecución (N = {n})', fontsize=13, fontweight='bold', pad=12)
        plt.xlabel('Número de Núcleos / Procesos', fontsize=11)
        plt.ylabel('Tiempo (segundos)', fontsize=11)
        
        # Asegurar que el eje X muestre exactamente los valores de núcleos utilizados
        plt.xticks(sorted(df_n['nucleos'].unique()))
        
        # Añadir cuadrícula sutil de fondo
        plt.grid(True, linestyle=':', alpha=0.6)
        
        # Leyenda con sombra ligera
        plt.legend(frameon=True, facecolor='white', edgecolor='none', shadow=True, loc='best')
        
        plt.tight_layout()
        
        # Guardar la gráfica automáticamente con el nombre de N correspondiente
        nombre_grafica = f'resultados/experimento_2026-05-24_17-51-51/grafica_tiempo_N{n}.png'
        plt.savefig(nombre_grafica, bbox_inches='tight')
        plt.close()
        print(f"Grafica guardada exitosamente: {nombre_grafica}")

# Ejecución del script buscando el archivo en la misma carpeta
if __name__ == "__main__":
    # Ruta relativa al archivo de datos
    ruta_archivo = 'resultados/experimento_2026-05-24_17-51-51/rendimiento_grafos_detalle.csv'
    generar_graficos_rendimiento(ruta_archivo)