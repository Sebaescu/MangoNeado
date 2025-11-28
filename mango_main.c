#include "mango_system.h"

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    srand(time(NULL));
    
    ConfiguracionSistema config;
    
    // Leer parametros o usar defaults
    if (argc >= 5) {
        config.velocidad_banda = atof(argv[1]);
        config.tamano_caja = atof(argv[2]);
        config.longitud_banda = atof(argv[3]);
        config.num_robots = atoi(argv[4]);
        config.num_mangos = (argc >= 6) ? atoi(argv[5]) : 20;
        config.prob_fallo = (argc >= 7) ? atof(argv[6]) : 0.0;
        config.usar_redundancia = (argc >= 8) ? atoi(argv[7]) : 0;
        
        // Validar parametros
        if (config.velocidad_banda <= 0 || config.tamano_caja <= 0 || 
            config.longitud_banda <= 0 || config.num_robots <= 0 || 
            config.num_mangos <= 0) {
            printf("Error: Todos los parámetros deben ser positivos\n");
            return 2;
        }
        if (config.prob_fallo < 0.0 || config.prob_fallo > 1.0) {
            printf("Error: Probabilidad de fallo debe estar entre 0 y 1\n");
            return 2;
        }
        if (config.num_robots > MAX_ROBOTS) {
            printf("Error: Máximo %d robots permitidos\n", MAX_ROBOTS);
            return 2;
        }
        if (config.num_mangos > MAX_MANGOS) {
            printf("Error: Máximo %d mangos permitidos\n", MAX_MANGOS);
            return 2;
        }
    } else {
        // Si no pasan parametros usar estos
        config.velocidad_banda = 10.0;
        config.tamano_caja = 50.0;
        config.longitud_banda = 200.0;
        config.num_robots = 5;
        config.num_mangos = 20;
        config.prob_fallo = 0.0;
        config.usar_redundancia = 0;
        
        printf("Uso: %s <velocidad_banda> <tamano_caja> <longitud_banda> "
               "<num_robots> [num_mangos] [prob_fallo] [usar_redundancia]\n", 
               argv[0]);
        printf("Usando configuración por defecto...\n\n");
    }
    
    printf("\n=== INICIANDO SIMULACIÓN ===\n");
    printf("Mangos: %d | Robots: %d | Velocidad: %.2f cm/s | Caja: %.2f cm\n\n",
           config.num_mangos, config.num_robots, config.velocidad_banda, 
           config.tamano_caja);
    
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