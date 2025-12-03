#include "mango_system.h"

// Variables para limpiar al final
static int shm_fd = -1;
static sem_t *sem_mutex = NULL;
static EstadoSistema *estado_compartido = NULL;

// Para manejar Ctrl+C
void signal_handler(int signo) {
    (void)signo;
    if (estado_compartido != NULL) {
        estado_compartido->simulacion_activa = 0;
    }
    cleanup_recursos();
    exit(0);
}

// Limpia la memoria compartida y semaforos
void cleanup_recursos() {
    if (estado_compartido != NULL) {
        munmap(estado_compartido, sizeof(EstadoSistema));
        estado_compartido = NULL;
    }
    if (shm_fd != -1) {
        close(shm_fd);
        shm_unlink(SHM_NAME);
        shm_fd = -1;
    }
    if (sem_mutex != NULL) {
        sem_close(sem_mutex);
        sem_unlink(SEM_MUTEX_NAME);
        sem_mutex = NULL;
    }
}

// Pone todo en 0 al inicio
void inicializar_sistema(EstadoSistema *estado, ConfiguracionSistema *config) {
    memset(estado, 0, sizeof(EstadoSistema));
    
    estado->velocidad_banda = config->velocidad_banda;
    estado->tamano_caja = config->tamano_caja;
    estado->longitud_banda = config->longitud_banda;
    estado->num_robots_totales = config->num_robots;
    estado->robots_activos = config->num_robots;
    estado->simulacion_activa = 1;
    estado->posicion_caja = 0.0;
    estado->caja_completada = 0;
    
    for (int i = 0; i < config->num_robots; i++) {
        estado->robots_disponibles[i] = 1;
        estado->robots_fallados[i] = 0;
    }
    
    calcular_posiciones_robots(estado, config->longitud_banda, config->num_robots);
}

// Calcula donde va cada robot en la banda
void calcular_posiciones_robots(EstadoSistema *estado, float longitud_banda, 
                                int num_robots) {
    if (num_robots <= 0) return;
    
    float separacion = longitud_banda / num_robots;
    for (int i = 0; i < num_robots; i++) {
        estado->posiciones_robot[i] = separacion * (i + 0.5);
    }
}

// Genera mangos random en la caja
void generar_mangos(EstadoSistema *estado, int num_mangos, float tamano_caja) {
    estado->num_mangos = num_mangos;
    
    // No poner los mangos muy lejos para que se puedan alcanzar
    (void)tamano_caja;
    float limite = 7.0;  // 7cm max para que funcione bien
    
    for (int i = 0; i < num_mangos; i++) {
        estado->mangos[i].x = ((float)rand() / RAND_MAX) * 2.0 * limite - limite;
        estado->mangos[i].y = ((float)rand() / RAND_MAX) * 2.0 * limite - limite;
        estado->mangos[i].etiquetado = 0;
        estado->mangos[i].robot_asignado = -1;
        estado->mangos[i].tiempo_etiquetado = -1.0;
    }
}

// Calcula cuanto tarda en etiquetar un mango
float calcular_tiempo_etiquetado(Mango *mango, float tamano_caja) {
    float distancia = sqrt(mango->x * mango->x + mango->y * mango->y);
    float velocidad_robot = tamano_caja / 10.0;
    return (2.0 * distancia) / velocidad_robot;
}

// Lo que hace cada robot
void proceso_robot(int robot_id, EstadoSistema *estado, sem_t *mutex, 
                   ConfiguracionSistema *config) {
    float pos_robot = estado->posiciones_robot[robot_id];
    float inicio_zona = pos_robot - config->tamano_caja / 2.0;
    float fin_zona = pos_robot + config->tamano_caja / 2.0;
    
    printf("[Robot %d] Iniciado en posición %.2f cm (zona: %.2f - %.2f)\n", 
           robot_id, pos_robot, inicio_zona, fin_zona);
    
    while (estado->simulacion_activa) {
        if (estado->robots_fallados[robot_id]) {
            usleep(100000);
            continue;
        }
        
        float pos_caja = estado->posicion_caja;
        
        // Si la caja esta en mi zona, buscar mangos
        if (pos_caja >= inicio_zona && pos_caja <= fin_zona) {
            sem_wait(mutex);
            
            int mango_etiquetado_ahora = 0;
            float tiempo_actual = pos_caja / config->velocidad_banda;
            
            // Buscar un mango que no este etiquetado
            for (int i = 0; i < estado->num_mangos; i++) {
                if (!estado->mangos[i].etiquetado && 
                    estado->mangos[i].robot_asignado == -1) {
                    
                    float tiempo_etiquetado = calcular_tiempo_etiquetado(
                        &estado->mangos[i], config->tamano_caja);
                    
                    float tiempo_disponible = (fin_zona - pos_caja) / 
                                             config->velocidad_banda;
                    
                    if (tiempo_etiquetado <= tiempo_disponible) {
                        estado->mangos[i].robot_asignado = robot_id;
                        mango_etiquetado_ahora = 1;
                        
                        sem_post(mutex);
                        usleep((int)(tiempo_etiquetado * 1000000));
                        sem_wait(mutex);
                        
                        estado->mangos[i].etiquetado = 1;
                        estado->mangos[i].tiempo_etiquetado = tiempo_actual + 
                                                              tiempo_etiquetado;
                        
                        printf("[Robot %d] Etiquetó mango %d en (%.2f, %.2f) "
                               "- Tiempo: %.3fs\n", 
                               robot_id, i, estado->mangos[i].x, 
                               estado->mangos[i].y, tiempo_etiquetado);
                        
                        break;  // Ya etiquete uno, buscar otro
                    }
                }
            }
            
            // Soltar el mutex
            if (mango_etiquetado_ahora) {
                sem_post(mutex);
            } else {
                sem_post(mutex);
                // No hay mangos, esperar
                usleep(10000);
            }
        } else {
            // La caja no esta en mi zona
            usleep(10000);
        }
        
        // Ver si ya terminamos
        if (pos_caja > fin_zona) {
            sem_wait(mutex);
            int todos_etiquetados = 1;
            for (int i = 0; i < estado->num_mangos; i++) {
                if (!estado->mangos[i].etiquetado) {
                    todos_etiquetados = 0;
                    break;
                }
            }
            if (todos_etiquetados) {
                estado->caja_completada = 1;
                sem_post(mutex);
                break;
            }
            sem_post(mutex);
        }
        
        usleep(1000);  // Dormir un poco
    }
    
    printf("[Robot %d] Finalizando operación\n", robot_id);
}

// Corre toda la simulacion
int simular_etiquetado(ConfiguracionSistema *config, int *mangos_etiquetados) {
    pid_t pids[MAX_ROBOTS];
    int num_procesos = 0;
    
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return -1;
    }
    
    if (ftruncate(shm_fd, sizeof(EstadoSistema)) == -1) {
        perror("ftruncate");
        return -1;
    }
    
    estado_compartido = mmap(NULL, sizeof(EstadoSistema), 
                             PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (estado_compartido == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    
    sem_mutex = sem_open(SEM_MUTEX_NAME, O_CREAT, 0666, 1);
    if (sem_mutex == SEM_FAILED) {
        perror("sem_open");
        return -1;
    }
    
    inicializar_sistema(estado_compartido, config);
    generar_mangos(estado_compartido, config->num_mangos, config->tamano_caja);
    
    // Crear los procesos de los robots
    for (int i = 0; i < config->num_robots; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            proceso_robot(i, estado_compartido, sem_mutex, config);
            exit(0);
        } else if (pid > 0) {
            pids[num_procesos++] = pid;
        } else {
            perror("fork");
            return -1;
        }
    }
    
    // Mover la banda
    float tiempo_total = (config->longitud_banda + config->tamano_caja) / 
                         config->velocidad_banda;
    float dt = 0.05;
    int pasos = (int)(tiempo_total / dt);
    
    for (int paso = 0; paso <= pasos && estado_compartido->simulacion_activa; 
         paso++) {
        sem_wait(sem_mutex);
        estado_compartido->posicion_caja += config->velocidad_banda * dt;
        
        if (config->usar_redundancia && config->prob_fallo > 0) {
            for (int i = 0; i < config->num_robots; i++) {
                if (!estado_compartido->robots_fallados[i]) {
                    if ((float)rand() / RAND_MAX < config->prob_fallo * dt) {
                        estado_compartido->robots_fallados[i] = 1;
                        printf("[SISTEMA] Robot %d ha fallado!\n", i);
                    }
                }
            }
        }
        
        int completada = estado_compartido->caja_completada;
        sem_post(sem_mutex);
        
        if (completada) {
            break;
        }
        
        usleep((int)(dt * 1000000));
    }
    
    estado_compartido->simulacion_activa = 0;
    
    for (int i = 0; i < num_procesos; i++) {
        waitpid(pids[i], NULL, 0);
    }
    
    *mangos_etiquetados = 0;
    for (int i = 0; i < estado_compartido->num_mangos; i++) {
        if (estado_compartido->mangos[i].etiquetado) {
            (*mangos_etiquetados)++;
        }
    }
    
    printf("\n=== SIMULACIÓN COMPLETADA ===\n");
    printf("Mangos etiquetados: %d / %d\n", *mangos_etiquetados, config->num_mangos);
    
    // Calcular porcentaje de éxito (90% o más se considera éxito)
    float porcentaje_etiquetado = (float)(*mangos_etiquetados) / config->num_mangos;
    int exito = (porcentaje_etiquetado >= 0.90) ? 1 : 0;
    
    cleanup_recursos();
    return exito;
}