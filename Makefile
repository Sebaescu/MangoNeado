CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -O2
LDFLAGS = -lrt -lpthread -lm

CORE_SRC = mango_core.c
MAIN_SRC = mango_main.c
ANALYSIS_SRC = mango_analysis.c
HEADER = mango_system.h
CORE_OBJ = mango_core.o

MAIN_EXEC = mango_simulator
ANALYSIS_EXEC = mango_analysis

all: $(MAIN_EXEC) $(ANALYSIS_EXEC)

$(CORE_OBJ): $(CORE_SRC) $(HEADER)
	$(CC) $(CFLAGS) -c $(CORE_SRC) -o $(CORE_OBJ)
	@echo "✓ Funciones core compiladas"

$(MAIN_EXEC): $(MAIN_SRC) $(CORE_OBJ) $(HEADER)
	$(CC) $(CFLAGS) $(MAIN_SRC) $(CORE_OBJ) -o $(MAIN_EXEC) $(LDFLAGS)
	@echo "✓ Programa principal compilado: $(MAIN_EXEC)"

$(ANALYSIS_EXEC): $(ANALYSIS_SRC) $(CORE_OBJ) $(HEADER)
	$(CC) $(CFLAGS) $(ANALYSIS_SRC) $(CORE_OBJ) -o $(ANALYSIS_EXEC) $(LDFLAGS)
	@echo "✓ Programa de análisis compilado: $(ANALYSIS_EXEC)"

clean:
	rm -f $(MAIN_EXEC) $(ANALYSIS_EXEC)
	rm -f *.o *.csv
	@echo "✓ Archivos limpiados"

clean-ipc:
	@echo "Limpiando recursos IPC..."
	@rm -f /dev/shm/mango_* 2>/dev/null || true
	@echo "✓ Recursos IPC limpiados"

test: $(MAIN_EXEC)
	@echo ""
	@echo "=== PRUEBA DEL SIMULADOR ==="
	@echo "Configuración: 10 cm/s, caja 50cm, banda 200cm, 4 robots, 6 mangos"
	@echo ""
	./$(MAIN_EXEC) 10 50 200 4 6

test-analysis: $(ANALYSIS_EXEC)
	@echo ""
	@echo "=== PRUEBA DE ANÁLISIS: BÚSQUEDA DE ROBOTS ÓPTIMOS ==="
	@echo "Buscando número óptimo de robots para 6 mangos (2 simulaciones)"
	@echo ""
	./$(ANALYSIS_EXEC) 1 6 2
	@echo ""

test-curve: $(ANALYSIS_EXEC)
	@echo ""
	@echo "=== PRUEBA DE ANÁLISIS: GENERACIÓN DE CURVA ==="
	@echo "Generando curva para 4-8 mangos con incremento 2 (1 simulación)"
	@echo ""
	./$(ANALYSIS_EXEC) 2 4 8 2 1
	@echo ""
	@if [ -f curva_robots_mangos.csv ]; then \
		echo "✓ Archivo generado: curva_robots_mangos.csv"; \
		echo "Contenido:"; \
		cat curva_robots_mangos.csv; \
	fi

test-redundancy: $(ANALYSIS_EXEC)
	@echo ""
	@echo "=== PRUEBA DE ANÁLISIS: REDUNDANCIA CON FALLOS ==="
	@echo "Analizando redundancia con 8 mangos, probabilidad fallo 10% (1 simulación)"
	@echo ""
	./$(ANALYSIS_EXEC) 3 8 1 0.1 1
	@echo ""
	@if [ -f analisis_redundancia.csv ]; then \
		echo "✓ Archivo generado: analisis_redundancia.csv"; \
		echo "Contenido:"; \
		cat analisis_redundancia.csv; \
	fi

test-all: clean-ipc test test-analysis test-curve
	@echo ""
	@echo "=============================================="
	@echo "✓ TODAS LAS PRUEBAS COMPLETADAS"
	@echo "=============================================="
	@echo ""

help:
	@echo "Uso del Makefile:"
	@echo ""
	@echo "Compilación:"
	@echo "  make all             - Compilar todos los programas"
	@echo "  make clean           - Limpiar archivos compilados y CSV"
	@echo "  make clean-ipc       - Limpiar recursos IPC del sistema"
	@echo ""
	@echo "Pruebas:"
	@echo "  make test            - Prueba rápida del simulador"
	@echo "  make test-analysis   - Prueba búsqueda de robots óptimos"
	@echo "  make test-curve      - Prueba generación de curva"
	@echo "  make test-redundancy - Prueba análisis con redundancia"
	@echo "  make test-all        - Ejecutar todas las pruebas"
	@echo ""
	@echo "Ayuda:"
	@echo "  make help            - Mostrar esta ayuda"
	@echo ""

.PHONY: all clean clean-ipc test test-analysis test-curve test-redundancy test-all help
