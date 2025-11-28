#!/bin/bash

# Script de pruebas automatizadas para el Sistema MangoNeado
# Autor: Sistema MangoNeado
# Fecha: 2025

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Contadores
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_TOTAL=0

# Función para imprimir encabezados
print_header() {
    echo -e "\n${BLUE}================================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}================================================${NC}\n"
}

# Función para imprimir resultado de test
print_test_result() {
    local test_name=$1
    local result=$2
    
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    
    if [ $result -eq 0 ]; then
        echo -e "${GREEN}✓ PASS${NC} - $test_name"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ FAIL${NC} - $test_name"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

# Función para verificar que existen los ejecutables
check_executables() {
    print_header "Verificando Ejecutables"
    
    if [ ! -f "./mango_simulator" ]; then
        echo -e "${RED}Error: mango_simulator no encontrado${NC}"
        echo "Ejecuta 'make all' primero"
        exit 1
    fi
    
    if [ ! -f "./mango_analysis" ]; then
        echo -e "${RED}Error: mango_analysis no encontrado${NC}"
        echo "Ejecuta 'make all' primero"
        exit 1
    fi
    
    echo -e "${GREEN}✓ Ejecutables encontrados${NC}"
}

# Limpiar recursos IPC antes de comenzar
cleanup_ipc() {
    print_header "Limpieza de Recursos IPC"
    make clean-ipc > /dev/null 2>&1
    echo -e "${GREEN}✓ Recursos IPC limpiados${NC}"
}

# Test 1: Configuración simple con éxito esperado
test_simple_success() {
    local test_name="Configuración Simple (10 mangos, 3 robots)"
    echo -e "\n${YELLOW}Test:${NC} $test_name"
    
    ./mango_simulator 10 50 200 3 10 > /tmp/test1.log 2>&1
    local result=$?
    
    print_test_result "$test_name" $result
    
    if [ $result -eq 0 ]; then
        local etiquetados=$(grep "Mangos etiquetados:" /tmp/test1.log | awk '{print $3}')
        echo "  → Mangos etiquetados: $etiquetados/10"
    fi
}

# Test 2: Muchos mangos, verificar que falla con pocos robots
test_insufficient_robots() {
    local test_name="Robots Insuficientes (30 mangos, 2 robots)"
    echo -e "\n${YELLOW}Test:${NC} $test_name"
    
    ./mango_simulator 10 50 200 2 30 > /tmp/test2.log 2>&1
    local result=$?
    
    # Este test debe fallar (exit code != 0)
    if [ $result -ne 0 ]; then
        print_test_result "$test_name - Fallo detectado correctamente" 0
    else
        print_test_result "$test_name - Debería haber fallado" 1
    fi
}

# Test 3: Banda rápida requiere más robots
test_fast_conveyor() {
    local test_name="Banda Rápida (20 cm/s, 20 mangos, 6 robots)"
    echo -e "\n${YELLOW}Test:${NC} $test_name"
    
    ./mango_simulator 20 50 200 6 20 > /tmp/test3.log 2>&1
    local result=$?
    
    print_test_result "$test_name" $result
}

# Test 4: Banda lenta debería funcionar con menos robots
test_slow_conveyor() {
    local test_name="Banda Lenta (5 cm/s, 20 mangos, 3 robots)"
    echo -e "\n${YELLOW}Test:${NC} $test_name"
    
    ./mango_simulator 5 50 200 3 20 > /tmp/test4.log 2>&1
    local result=$?
    
    print_test_result "$test_name" $result
}

# Test 5: Caja pequeña con pocos mangos
test_small_box() {
    local test_name="Caja Pequeña (30cm, 8 mangos, 2 robots)"
    echo -e "\n${YELLOW}Test:${NC} $test_name"
    
    ./mango_simulator 10 30 150 2 8 > /tmp/test5.log 2>&1
    local result=$?
    
    print_test_result "$test_name" $result
}

# Test 6: Caja grande con muchos mangos
test_large_box() {
    local test_name="Caja Grande (80cm, 35 mangos, 7 robots)"
    echo -e "\n${YELLOW}Test:${NC} $test_name"
    
    ./mango_simulator 10 80 250 7 35 > /tmp/test6.log 2>&1
    local result=$?
    
    print_test_result "$test_name" $result
}

# Test 7: Análisis - encontrar robots óptimos
test_analysis_optimal() {
    local test_name="Análisis: Encontrar robots óptimos (15 mangos)"
    echo -e "\n${YELLOW}Test:${NC} $test_name"
    
    timeout 60s ./mango_analysis 1 15 3 > /tmp/test7.log 2>&1
    local result=$?
    
    if [ $result -eq 124 ]; then
        echo -e "${YELLOW}  → Timeout después de 60s${NC}"
        print_test_result "$test_name" 1
    else
        print_test_result "$test_name" $result
        
        if [ $result -eq 0 ]; then
            local robots=$(grep "óptimo encontrado:" /tmp/test7.log | grep -oP '\d+' | tail -1)
            if [ ! -z "$robots" ]; then
                echo "  → Robots óptimos encontrados: $robots"
            fi
        fi
    fi
}

# Test 8: Generar curva pequeña
test_generate_curve() {
    local test_name="Generar Curva (10-20 mangos, 2 sims)"
    echo -e "\n${YELLOW}Test:${NC} $test_name"
    
    timeout 120s ./mango_analysis 2 10 20 5 2 > /tmp/test8.log 2>&1
    local result=$?
    
    if [ $result -eq 124 ]; then
        echo -e "${YELLOW}  → Timeout después de 120s${NC}"
        print_test_result "$test_name" 1
    else
        print_test_result "$test_name" $result
        
        if [ -f "curva_robots_mangos.csv" ]; then
            local lines=$(wc -l < curva_robots_mangos.csv)
            echo "  → Archivo CSV generado con $lines líneas"
        fi
    fi
}

# Test 9: Sistema con redundancia (sin fallos)
test_redundancy_no_failure() {
    local test_name="Redundancia sin Fallos (prob=0.0)"
    echo -e "\n${YELLOW}Test:${NC} $test_name"
    
    ./mango_simulator 10 50 200 4 20 0.0 1 > /tmp/test9.log 2>&1
    local result=$?
    
    print_test_result "$test_name" $result
}

# Test 10: Sistema con redundancia (con fallos)
test_redundancy_with_failure() {
    local test_name="Redundancia con Fallos (prob=0.05, robots extra)"
    echo -e "\n${YELLOW}Test:${NC} $test_name"
    
    # Usar más robots para compensar fallos
    ./mango_simulator 10 50 200 6 20 0.05 1 > /tmp/test10.log 2>&1
    local result=$?
    
    print_test_result "$test_name" $result
    
    local fallos=$(grep "ha fallado" /tmp/test10.log | wc -l)
    if [ $fallos -gt 0 ]; then
        echo "  → Fallos detectados durante simulación: $fallos"
    fi
}

# Test 11: Verificar limpieza de recursos
test_resource_cleanup() {
    local test_name="Limpieza de Recursos IPC"
    echo -e "\n${YELLOW}Test:${NC} $test_name"
    
    # Ejecutar simulación
    ./mango_simulator 10 50 200 3 10 > /dev/null 2>&1
    
    # Verificar que no quedaron recursos huérfanos
    local shm_count=$(ls /dev/shm/mango_* 2>/dev/null | wc -l)
    
    if [ $shm_count -eq 0 ]; then
        print_test_result "$test_name" 0
    else
        echo "  → Recursos IPC no limpiados: $shm_count"
        print_test_result "$test_name" 1
    fi
}

# Test 12: Manejo de señales (SIGINT)
test_signal_handling() {
    local test_name="Manejo de Señales (SIGINT)"
    echo -e "\n${YELLOW}Test:${NC} $test_name"
    
    # Iniciar simulación en background
    ./mango_simulator 5 50 200 3 20 > /tmp/test12.log 2>&1 &
    local pid=$!
    
    # Esperar un momento
    sleep 2
    
    # Enviar SIGINT
    kill -INT $pid 2>/dev/null
    
    # Esperar a que termine
    wait $pid 2>/dev/null
    
    # Verificar limpieza de recursos
    sleep 1
    local shm_count=$(ls /dev/shm/mango_* 2>/dev/null | wc -l)
    
    if [ $shm_count -eq 0 ]; then
        print_test_result "$test_name" 0
    else
        print_test_result "$test_name" 1
    fi
}

# Test 13: Parámetros inválidos
test_invalid_parameters() {
    local test_name="Parámetros Inválidos"
    echo -e "\n${YELLOW}Test:${NC} $test_name"
    
    # Velocidad negativa
    ./mango_simulator -10 50 200 3 10 > /tmp/test13.log 2>&1
    local result1=$?
    
    # Caja de tamaño 0
    ./mango_simulator 10 0 200 3 10 > /tmp/test13.log 2>&1
    local result2=$?
    
    # Debe fallar en ambos casos
    if [ $result1 -ne 0 ] && [ $result2 -ne 0 ]; then
        print_test_result "$test_name - Validación correcta" 0
    else
        print_test_result "$test_name - No valida parámetros" 1
    fi
}

# Test 14: Stress test - muchos robots
test_many_robots() {
    local test_name="Stress Test (15 robots, 40 mangos)"
    echo -e "\n${YELLOW}Test:${NC} $test_name"
    
    timeout 60s ./mango_simulator 10 50 300 15 40 > /tmp/test14.log 2>&1
    local result=$?
    
    if [ $result -eq 124 ]; then
        echo -e "${YELLOW}  → Timeout${NC}"
        print_test_result "$test_name" 1
    else
        print_test_result "$test_name" $result
    fi
}

# Test 15: Verificar que los archivos CSV se generan correctamente
test_csv_generation() {
    local test_name="Generación de Archivos CSV"
    echo -e "\n${YELLOW}Test:${NC} $test_name"
    
    # Limpiar CSVs previos
    rm -f curva_robots_mangos.csv analisis_redundancia.csv
    
    # Generar curva pequeña
    timeout 60s ./mango_analysis 2 10 15 5 2 > /dev/null 2>&1
    
    if [ -f "curva_robots_mangos.csv" ]; then
        # Verificar que tiene header y datos
        local lines=$(wc -l < curva_robots_mangos.csv)
        if [ $lines -gt 1 ]; then
            print_test_result "$test_name" 0
            echo "  → CSV generado con $lines líneas"
        else
            print_test_result "$test_name" 1
        fi
    else
        print_test_result "$test_name" 1
    fi
}

# Función principal
main() {
    clear
    print_header "SISTEMA MANGONEADO - SUITE DE PRUEBAS"
    
    echo "Fecha: $(date)"
    echo "Sistema: $(uname -s)"
    echo ""
    
    # Verificaciones previas
    check_executables
    cleanup_ipc
    
    # Ejecutar tests básicos
    print_header "TESTS BÁSICOS DE FUNCIONALIDAD"
    test_simple_success
    test_insufficient_robots
    test_fast_conveyor
    test_slow_conveyor
    test_small_box
    test_large_box
    
    # Tests de análisis
    print_header "TESTS DE ANÁLISIS Y OPTIMIZACIÓN"
    test_analysis_optimal
    test_generate_curve
    
    # Tests de redundancia
    print_header "TESTS DE REDUNDANCIA"
    test_redundancy_no_failure
    test_redundancy_with_failure
    
    # Tests de robustez
    print_header "TESTS DE ROBUSTEZ Y MANEJO DE ERRORES"
    test_resource_cleanup
    test_signal_handling
    test_invalid_parameters
    
    # Tests de rendimiento
    print_header "TESTS DE RENDIMIENTO"
    test_many_robots
    test_csv_generation
    
    # Resumen final
    print_header "RESUMEN DE RESULTADOS"
    
    echo -e "Tests ejecutados:  ${TESTS_TOTAL}"
    echo -e "${GREEN}Tests exitosos:    ${TESTS_PASSED}${NC}"
    echo -e "${RED}Tests fallidos:    ${TESTS_FAILED}${NC}"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "\n${GREEN}✓✓✓ TODOS LOS TESTS PASARON ✓✓✓${NC}"
        echo -e "${GREEN}El sistema está listo para uso${NC}\n"
    else
        echo -e "\n${YELLOW}⚠ ALGUNOS TESTS FALLARON ⚠${NC}"
        echo -e "${YELLOW}Revisa los logs en /tmp/test*.log${NC}\n"
    fi
    
    # Limpiar archivos temporales
    echo -e "\n${BLUE}Limpiando archivos temporales...${NC}"
    # rm -f /tmp/test*.log
    echo "Logs guardados en /tmp/test*.log para inspección"
    
    # Limpieza final
    cleanup_ipc
    
    # Exit code
    if [ $TESTS_FAILED -eq 0 ]; then
        exit 0
    else
        exit 1
    fi
}

# Ejecutar main
main