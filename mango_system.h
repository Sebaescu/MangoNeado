#ifndef MANGO_SYSTEM_H
#define MANGO_SYSTEM_H

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>

// Constantes del sistema
#define MAX_MANGOS 50
#define MAX_ROBOTS 20
#define SHM_NAME "/mango_system_shm"
#define SEM_MUTEX_NAME "/mango_mutex"
#define SEM_ROBOT_NAME "/mango_robot_"

// Estructura para representar un mango
typedef struct {
    float x;              // Coordenada X relativa al centroide (cm)
    float y;              // Coordenada Y relativa al centroide (cm)
    int etiquetado;       // 0 = no etiquetado, 1 = etiquetado
    int robot_asignado;   // ID del robot asignado (-1 si ninguno)
    float tiempo_etiquetado; // Tiempo en que fue etiquetado
} Mango;

// Estado compartido del sistema
typedef struct {
    int num_mangos;                    // Número de mangos en la caja actual
    Mango mangos[MAX_MANGOS];          // Array de mangos
    float posicion_caja;               // Posición actual de la caja en la banda (cm)
    int robots_activos;                // Número de robots actualmente activos
    int robots_disponibles[MAX_ROBOTS]; // 1 = disponible, 0 = no disponible
    int robots_fallados[MAX_ROBOTS];   // 1 = fallado, 0 = operativo
    int caja_completada;               // 1 = todos los mangos etiquetados
    int simulacion_activa;             // 1 = simulación en curso
    float posiciones_robot[MAX_ROBOTS]; // Posición de cada robot en la banda
    
    // Parámetros de operación
    float velocidad_banda;  // X cm/s
    float tamano_caja;      // Z cm (lado del cuadrado)
    float longitud_banda;   // W cm
    int num_robots_totales; // Número total de robots instalados
} EstadoSistema;

// Parámetros de configuración
typedef struct {
    float velocidad_banda;     // X cm/s
    float tamano_caja;         // Z cm
    float longitud_banda;      // W cm
    int num_robots;            // Número de robots
    int num_mangos;            // Número de mangos en la caja
    float prob_fallo;          // Probabilidad de fallo (0-1)
    int usar_redundancia;      // 1 = activar redundancia
} ConfiguracionSistema;

// Funciones principales
void inicializar_sistema(EstadoSistema *estado, ConfiguracionSistema *config);
void generar_mangos(EstadoSistema *estado, int num_mangos, float tamano_caja);
void proceso_robot(int robot_id, EstadoSistema *estado, sem_t *mutex, 
                   ConfiguracionSistema *config);
int simular_etiquetado(ConfiguracionSistema *config, int *mangos_etiquetados);
void calcular_posiciones_robots(EstadoSistema *estado, float longitud_banda, 
                                int num_robots);
float calcular_tiempo_etiquetado(Mango *mango, float tamano_caja);

// Funciones de utilidad
void imprimir_estado(EstadoSistema *estado);
void cleanup_recursos();
void signal_handler(int signo);

#endif // MANGO_SYSTEM_H
