#include "mango_system.h"

#define NUM_SIMULACIONES 10  // Número de simulaciones por configuración
#define MAX_INTENTOS_ROBOT 15 // Máximo número de robots a probar

// Estructura para resultados de análisis
typedef struct {
    int num_robots;
    int num_mangos;
    float tasa_exito;
    float tiempo_promedio;
    int exitos;
    int fallos;
} ResultadoAnalisis;

// Función para realizar múltiples simulaciones y calcular estadísticas
ResultadoAnalisis analizar_configuracion(ConfiguracionSistema *config, 
                                         int num_simulaciones) {
    ResultadoAnalisis resultado;
    resultado.num_robots = config->num_robots;
    resultado.num_mangos = config->num_mangos;
    resultado.exitos = 0;
    resultado.fallos = 0;
    resultado.tiempo_promedio = 0.0;
    
    printf("\nAnalizando: %d robots, %d mangos", 
           config->num_robots, config->num_mangos);
    if (config->prob_fallo > 0) {
        printf(", prob_fallo=%.2f", config->prob_fallo);
    }
    printf("\n");
    
    for (int i = 0; i < num_simulaciones; i++) {
        int mangos_etiquetados;
        
        clock_t inicio = clock();
        int exito = simular_etiquetado(config, &mangos_etiquetados);
        clock_t fin = clock();
        
        double tiempo_sim = (double)(fin - inicio) / CLOCKS_PER_SEC;
        resultado.tiempo_promedio += tiempo_sim;
        
        if (exito == 1) {
            resultado.exitos++;
            printf("  [%d/%d] ✓ Éxito (%.2fs)\n", 
                   i + 1, num_simulaciones, tiempo_sim);
        } else {
            resultado.fallos++;
            printf("  [%d/%d] ✗ Fallo: %d/%d etiquetados (%.2fs)\n", 
                   i + 1, num_simulaciones, mangos_etiquetados, 
                   config->num_mangos, tiempo_sim);
        }
        
        // Pequeña pausa entre simulaciones para asegurar limpieza
        usleep(100000);
    }
    
    resultado.tiempo_promedio /= num_simulaciones;
    resultado.tasa_exito = (float)resultado.exitos / num_simulaciones;
    
    return resultado;
}

// Encontrar el número mínimo de robots para una tasa de éxito objetivo
int encontrar_num_robots_optimo(ConfiguracionSistema *config, 
                                float tasa_exito_objetivo,
                                int num_simulaciones) {
    printf("\n=== BUSCANDO NÚMERO ÓPTIMO DE ROBOTS ===\n");
    printf("Objetivo: %.0f%% tasa de éxito\n", tasa_exito_objetivo * 100);
    printf("Mangos: %d\n", config->num_mangos);
    
    // Búsqueda lineal comenzando con pocos robots
    for (int num_robots = 1; num_robots <= MAX_INTENTOS_ROBOT; num_robots++) {
        config->num_robots = num_robots;
        
        ResultadoAnalisis resultado = analizar_configuracion(config, 
                                                             num_simulaciones);
        
        printf("Resultado: %d robots → %.0f%% éxito\n", 
               num_robots, resultado.tasa_exito * 100);
        
        if (resultado.tasa_exito >= tasa_exito_objetivo) {
            printf("\n✓ Número óptimo encontrado: %d robots\n", num_robots);
            return num_robots;
        }
    }
    
    printf("\n✗ No se encontró configuración óptima en %d robots\n", 
           MAX_INTENTOS_ROBOT);
    return -1;
}

// Generar curva de robots vs mangos
void generar_curva_robots_mangos(ConfiguracionSistema *config_base, 
                                 int num_mangos_min, 
                                 int num_mangos_max,
                                 int incremento_mangos,
                                 int num_simulaciones,
                                 float tasa_exito_objetivo) {
    printf("\n=== GENERANDO CURVA ROBOTS VS MANGOS ===\n");
    printf("Rango de mangos: %d - %d (incremento: %d)\n", 
           num_mangos_min, num_mangos_max, incremento_mangos);
    printf("Tasa de éxito objetivo: %.0f%%\n\n", tasa_exito_objetivo * 100);
    
    // Abrir archivo para guardar resultados
    FILE *archivo = fopen("curva_robots_mangos.csv", "w");
    if (archivo == NULL) {
        perror("Error abriendo archivo");
        return;
    }
    
    fprintf(archivo, "NumMangos,RobotsMínimos,TasaÉxito,TiempoPromedio\n");
    
    printf("NumMangos | Robots | Tasa Éxito | Tiempo\n");
    printf("----------|--------|------------|--------\n");
    
    ConfiguracionSistema config = *config_base;
    
    for (int n = num_mangos_min; n <= num_mangos_max; n += incremento_mangos) {
        config.num_mangos = n;
        
        int robots_optimos = encontrar_num_robots_optimo(&config, 
                                                         tasa_exito_objetivo, 
                                                         num_simulaciones);
        
        if (robots_optimos > 0) {
            config.num_robots = robots_optimos;
            ResultadoAnalisis resultado_final = analizar_configuracion(&config, 
                                                                       num_simulaciones);
            
            printf("%9d | %6d | %9.1f%% | %6.2fs\n", 
                   n, robots_optimos, resultado_final.tasa_exito * 100, 
                   resultado_final.tiempo_promedio);
            
            fprintf(archivo, "%d,%d,%.3f,%.3f\n", 
                    n, robots_optimos, resultado_final.tasa_exito, 
                    resultado_final.tiempo_promedio);
        }
    }
    
    fclose(archivo);
    printf("\nResultados guardados en: curva_robots_mangos.csv\n");
}

// Análisis con redundancia (probabilidad de fallo)
void analizar_con_redundancia(ConfiguracionSistema *config_base,
                              float prob_fallo,
                              int num_simulaciones) {
    printf("\n=== ANÁLISIS CON REDUNDANCIA ===\n");
    printf("Probabilidad de fallo: %.2f%%\n", prob_fallo * 100);
    
    ConfiguracionSistema config = *config_base;
    config.prob_fallo = prob_fallo;
    config.usar_redundancia = 1;
    
    // Abrir archivo para resultados
    FILE *archivo = fopen("analisis_redundancia.csv", "w");
    if (archivo == NULL) {
        perror("Error abriendo archivo");
        return;
    }
    
    fprintf(archivo, "NumRobots,TasaÉxito,TiempoPromedio\n");
    
    printf("\nRobots | Tasa Éxito | Tiempo\n");
    printf("-------|------------|--------\n");
    
    // Probar diferentes números de robots con redundancia
    for (int r = config_base->num_robots; 
         r <= config_base->num_robots + 5; r++) {
        config.num_robots = r;
        
        ResultadoAnalisis resultado = analizar_configuracion(&config, 
                                                             num_simulaciones);
        
        printf("%6d | %9.1f%% | %6.2fs\n", 
               r, resultado.tasa_exito * 100, resultado.tiempo_promedio);
        
        fprintf(archivo, "%d,%.3f,%.3f\n", 
                r, resultado.tasa_exito, resultado.tiempo_promedio);
        
        // Si alcanzamos alta tasa de éxito, podemos parar
        if (resultado.tasa_exito >= 0.95) {
            printf("\n✓ Redundancia adecuada con %d robots "
                   "(%.0f%% éxito con fallo %.0f%%)\n", 
                   r, resultado.tasa_exito * 100, prob_fallo * 100);
            break;
        }
    }
    
    fclose(archivo);
    printf("\nResultados guardados en: analisis_redundancia.csv\n");
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    // Configuración base del sistema
    ConfiguracionSistema config_base;
    config_base.velocidad_banda = 10.0;   // 10 cm/s
    config_base.tamano_caja = 50.0;       // 50 cm
    config_base.longitud_banda = 200.0;   // 200 cm
    config_base.prob_fallo = 0.0;
    config_base.usar_redundancia = 0;
    
    if (argc < 2) {
        printf("Uso: %s <modo> [opciones]\n", argv[0]);
        printf("\nModos:\n");
        printf("  1 - Análisis simple (encontrar robots óptimos)\n");
        printf("  2 - Generar curva robots vs mangos\n");
        printf("  3 - Análisis con redundancia\n");
        printf("\nEjemplos:\n");
        printf("  %s 1 20 5          # Encontrar robots para 20 mangos, 5 simulaciones\n", argv[0]);
        printf("  %s 2 10 30 5 3     # Curva de 10-30 mangos, incr=5, 3 sims\n", argv[0]);
        printf("  %s 3 20 5 0.05 5   # 20 mangos, 5 robots base, 5%% fallo, 5 sims\n", argv[0]);
        return 1;
    }
    
    int modo = atoi(argv[1]);
    
    switch (modo) {
        case 1: {
            // Análisis simple
            int num_mangos = (argc >= 3) ? atoi(argv[2]) : 20;
            int num_sims = (argc >= 4) ? atoi(argv[3]) : 5;
            
            config_base.num_mangos = num_mangos;
            encontrar_num_robots_optimo(&config_base, 0.95, num_sims);
            break;
        }
        
        case 2: {
            // Generar curva
            int min_mangos = (argc >= 3) ? atoi(argv[2]) : 10;
            int max_mangos = (argc >= 4) ? atoi(argv[3]) : 30;
            int incremento = (argc >= 5) ? atoi(argv[4]) : 5;
            int num_sims = (argc >= 6) ? atoi(argv[5]) : 3;
            
            generar_curva_robots_mangos(&config_base, min_mangos, max_mangos, 
                                       incremento, num_sims, 0.95);
            break;
        }
        
        case 3: {
            // Análisis con redundancia
            int num_mangos = (argc >= 3) ? atoi(argv[2]) : 20;
            int robots_base = (argc >= 4) ? atoi(argv[3]) : 5;
            float prob_fallo = (argc >= 5) ? atof(argv[4]) : 0.05;
            int num_sims = (argc >= 6) ? atoi(argv[5]) : 5;
            
            config_base.num_mangos = num_mangos;
            config_base.num_robots = robots_base;
            
            analizar_con_redundancia(&config_base, prob_fallo, num_sims);
            break;
        }
        
        default:
            printf("Modo inválido: %d\n", modo);
            return 1;
    }
    
    return 0;
}