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

// Limites del sistema
#define MAX_MANGOS 50
#define MAX_ROBOTS 20
#define SHM_NAME "/mango_system_shm"
#define SEM_MUTEX_NAME "/mango_mutex"
#define SEM_ROBOT_NAME "/mango_robot_"

// Info de cada mango
typedef struct {
    float x;
    float y;
    int etiquetado;       // 0 o 1
    int robot_asignado;   // cual robot lo agarro
    float tiempo_etiquetado;
} Mango;

// Estado del sistema compartido
typedef struct {
    int num_mangos;
    Mango mangos[MAX_MANGOS];
    float posicion_caja;
    int robots_activos;
    int robots_disponibles[MAX_ROBOTS];
    int robots_fallados[MAX_ROBOTS];
    int caja_completada;
    int simulacion_activa;
    float posiciones_robot[MAX_ROBOTS];
    
    // Parametros
    float velocidad_banda;
    float tamano_caja;
    float longitud_banda;
    int num_robots_totales;
} EstadoSistema;

// Configuracion para simular
typedef struct {
    float velocidad_banda;
    float tamano_caja;
    float longitud_banda;
    int num_robots;
    int num_mangos;
    float prob_fallo;          // 0 a 1
    int usar_redundancia;      // 0 o 1
} ConfiguracionSistema;

// Funciones
void inicializar_sistema(EstadoSistema *estado, ConfiguracionSistema *config);
void generar_mangos(EstadoSistema *estado, int num_mangos, float tamano_caja);
void proceso_robot(int robot_id, EstadoSistema *estado, sem_t *mutex, 
                   ConfiguracionSistema *config);
int simular_etiquetado(ConfiguracionSistema *config, int *mangos_etiquetados);
void calcular_posiciones_robots(EstadoSistema *estado, float longitud_banda, 
                                int num_robots);
float calcular_tiempo_etiquetado(Mango *mango, float tamano_caja);

// Otras funciones
void imprimir_estado(EstadoSistema *estado);
void cleanup_recursos();
void signal_handler(int signo);

#endif // MANGO_SYSTEM_H
