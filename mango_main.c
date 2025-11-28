#include "mango_system.h"

// Variables globales para cleanup
static int shm_fd = -1;
static sem_t *sem_mutex = NULL;
static EstadoSistema *estado_compartido = NULL;

// Manejador de señales para limpieza
void signal_handler(int signo) {
    (void)signo; // Suprimir warning de parámetro no usado
    if (estado_compartido != NULL) {
        estado_compartido->simulacion_activa = 0;
    }
    cleanup_recursos();
    exit(0);
}

// Limpieza de recursos
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

// Inicializar el sistema
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
    
    // Inicializar todos los robots como disponibles y operativos
    for (int i = 0; i < config->num_robots; i++) {
        estado->robots_disponibles[i] = 1;
        estado->robots_fallados[i] = 0;
    }
    
    // Calcular posiciones de los robots en la banda
    calcular_posiciones_robots(estado, config->longitud_banda, config->num_robots);
}

// Calcular posiciones equidistantes de robots
void calcular_posiciones_robots(EstadoSistema *estado, float longitud_banda, 
                                int num_robots) {
    if (num_robots <= 0) return;
    
    float separacion = longitud_banda / num_robots;
    for (int i = 0; i < num_robots; i++) {
        estado->posiciones_robot[i] = separacion * (i + 0.5);
    }
}

// Generar mangos aleatorios en la caja
void generar_mangos(EstadoSistema *estado, int num_mangos, float tamano_caja) {
    estado->num_mangos = num_mangos;
    
    // Los mangos se distribuyen aleatoriamente dentro de la caja
    // Coordenadas relativas al centroide (0,0)
    float limite = tamano_caja / 2.0 - 2.0; // Margen de 2cm
    
    for (int i = 0; i < num_mangos; i++) {
        estado->mangos[i].x = ((float)rand() / RAND_MAX) * 2.0 * limite - limite;
        estado->mangos[i].y = ((float)rand() / RAND_MAX) * 2.0 * limite - limite;
        estado->mangos[i].etiquetado = 0;
        estado->mangos[i].robot_asignado = -1;
        estado->mangos[i].tiempo_etiquetado = -1.0;
    }
}

// Calcular tiempo necesario para etiquetar un mango
float calcular_tiempo_etiquetado(Mango *mango, float tamano_caja) {
    // Velocidad del robot: Z/10 cm/s en promedio
    // Distancia desde el centro: sqrt(x^2 + y^2)
    float distancia = sqrt(mango->x * mango->x + mango->y * mango->y);
    float velocidad_robot = tamano_caja / 10.0;
    
    // Tiempo = distancia / velocidad (ida y vuelta)
    return (2.0 * distancia) / velocidad_robot;
}

// Proceso de robot individual
void proceso_robot(int robot_id, EstadoSistema *estado, sem_t *mutex, 
                   ConfiguracionSistema *config) {
    float pos_robot = estado->posiciones_robot[robot_id];
    float inicio_zona = pos_robot - config->tamano_caja / 2.0;
    float fin_zona = pos_robot + config->tamano_caja / 2.0;
    
    printf("[Robot %d] Iniciado en posición %.2f cm (zona: %.2f - %.2f)\n", 
           robot_id, pos_robot, inicio_zona, fin_zona);
    
    while (estado->simulacion_activa) {
        // Verificar si el robot ha fallado
        if (estado->robots_fallados[robot_id]) {
            usleep(100000); // Esperar 100ms
            continue;
        }
        
        // Verificar si la caja está en la zona del robot
        float pos_caja = estado->posicion_caja;
        
        if (pos_caja >= inicio_zona && pos_caja <= fin_zona) {
            // La caja está en nuestra zona, buscar mangos para etiquetar
            sem_wait(mutex);
            
            int mango_encontrado = 0;
            float tiempo_actual = pos_caja / config->velocidad_banda;
            
            for (int i = 0; i < estado->num_mangos; i++) {
                if (!estado->mangos[i].etiquetado && 
                    estado->mangos[i].robot_asignado == -1) {
                    
                    // Calcular tiempo necesario para etiquetar
                    float tiempo_etiquetado = calcular_tiempo_etiquetado(
                        &estado->mangos[i], config->tamano_caja);
                    
                    // Verificar si tenemos tiempo suficiente
                    float tiempo_disponible = (fin_zona - pos_caja) / 
                                             config->velocidad_banda;
                    
                    if (tiempo_etiquetado <= tiempo_disponible) {
                        // Asignar este mango al robot
                        estado->mangos[i].robot_asignado = robot_id;
                        mango_encontrado = 1;
                        
                        sem_post(mutex);
                        
                        // Simular el tiempo de etiquetado
                        usleep((int)(tiempo_etiquetado * 1000000));
                        
                        sem_wait(mutex);
                        
                        // Marcar como etiquetado
                        estado->mangos[i].etiquetado = 1;
                        estado->mangos[i].tiempo_etiquetado = tiempo_actual + 
                                                              tiempo_etiquetado;
                        
                        printf("[Robot %d] Etiquetó mango %d en (%.2f, %.2f) "
                               "- Tiempo: %.3fs\n", 
                               robot_id, i, estado->mangos[i].x, 
                               estado->mangos[i].y, tiempo_etiquetado);
                        
                        sem_post(mutex);
                        break;
                    }
                }
            }
            
            if (!mango_encontrado) {
                sem_post(mutex);
            }
        }
        
        // Verificar si la caja ya pasó completamente
        if (pos_caja > fin_zona + config->tamano_caja) {
            // Verificar si todos los mangos están etiquetados
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
            }
            sem_post(mutex);
            break;
        }
        
        usleep(10000); // Esperar 10ms entre verificaciones
    }
    
    printf("[Robot %d] Finalizando operación\n", robot_id);
}

// Simular una corrida completa
int simular_etiquetado(ConfiguracionSistema *config, int *mangos_etiquetados) {
    pid_t pids[MAX_ROBOTS];
    int num_procesos = 0;
    
    // Crear memoria compartida
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
    
    // Crear semáforo mutex
    sem_mutex = sem_open(SEM_MUTEX_NAME, O_CREAT, 0666, 1);
    if (sem_mutex == SEM_FAILED) {
        perror("sem_open");
        return -1;
    }
    
    // Inicializar sistema
    inicializar_sistema(estado_compartido, config);
    generar_mangos(estado_compartido, config->num_mangos, config->tamano_caja);
    
    printf("\n=== INICIANDO SIMULACIÓN ===\n");
    printf("Mangos: %d | Robots: %d | Velocidad: %.2f cm/s | Caja: %.2f cm\n",
           config->num_mangos, config->num_robots, config->velocidad_banda, 
           config->tamano_caja);
    
    // Crear procesos robot
    for (int i = 0; i < config->num_robots; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Proceso hijo (robot)
            proceso_robot(i, estado_compartido, sem_mutex, config);
            exit(0);
        } else if (pid > 0) {
            pids[num_procesos++] = pid;
        } else {
            perror("fork");
            return -1;
        }
    }
    
    // Proceso principal: mover la banda
    float tiempo_total = (config->longitud_banda + config->tamano_caja) / 
                         config->velocidad_banda;
    float dt = 0.05; // Actualizar cada 50ms
    int pasos = (int)(tiempo_total / dt);
    
    for (int paso = 0; paso <= pasos && estado_compartido->simulacion_activa; 
         paso++) {
        sem_wait(sem_mutex);
        estado_compartido->posicion_caja += config->velocidad_banda * dt;
        
        // Simular fallos de robots si hay redundancia
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
    
    // Señalar finalización
    estado_compartido->simulacion_activa = 0;
    
    // Esperar a que terminen los robots
    for (int i = 0; i < num_procesos; i++) {
        waitpid(pids[i], NULL, 0);
    }
    
    // Contar mangos etiquetados
    *mangos_etiquetados = 0;
    for (int i = 0; i < estado_compartido->num_mangos; i++) {
        if (estado_compartido->mangos[i].etiquetado) {
            (*mangos_etiquetados)++;
        }
    }
    
    printf("\n=== SIMULACIÓN COMPLETADA ===\n");
    printf("Mangos etiquetados: %d / %d\n", *mangos_etiquetados, 
           config->num_mangos);
    
    int exito = (*mangos_etiquetados == config->num_mangos) ? 1 : 0;
    
    cleanup_recursos();
    return exito;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    srand(time(NULL));
    
    ConfiguracionSistema config;
    
    // Configuración por defecto o desde argumentos
    if (argc >= 5) {
        config.velocidad_banda = atof(argv[1]);
        config.tamano_caja = atof(argv[2]);
        config.longitud_banda = atof(argv[3]);
        config.num_robots = atoi(argv[4]);
        config.num_mangos = (argc >= 6) ? atoi(argv[5]) : 20;
        config.prob_fallo = (argc >= 7) ? atof(argv[6]) : 0.0;
        config.usar_redundancia = (argc >= 8) ? atoi(argv[7]) : 0;
    } else {
        // Valores por defecto para prueba
        config.velocidad_banda = 10.0;  // 10 cm/s
        config.tamano_caja = 50.0;      // 50 cm
        config.longitud_banda = 200.0;  // 200 cm
        config.num_robots = 5;
        config.num_mangos = 20;
        config.prob_fallo = 0.0;
        config.usar_redundancia = 0;
        
        printf("Uso: %s <velocidad_banda> <tamano_caja> <longitud_banda> "
               "<num_robots> [num_mangos] [prob_fallo] [usar_redundancia]\n", 
               argv[0]);
        printf("Usando configuración por defecto...\n\n");
    }
    
    int mangos_etiquetados;
    int resultado = simular_etiquetado(&config, &mangos_etiquetados);
    
    if (resultado == 1) {
        printf("\n✓ ÉXITO: Todos los mangos fueron etiquetados\n");
        return 0;
    } else if (resultado == 0) {
        printf("\n✗ FALLO: No todos los mangos fueron etiquetados\n");
        return 1;
    } else {
        printf("\n✗ ERROR: Error en la simulación\n");
        return 2;
    }
}