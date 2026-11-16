# Ejecutar siempre desde code/grid_pathfinding/ (el programa usa rutas relativas a data/).
.PHONY: help all run debug clean testcases plot
.SILENT:

CXX      ?= g++
CXXFLAGS := -std=c++17
SRC      := grid_pathfinding.cpp algorithms/brute_force.cpp algorithms/dynamic_programming.cpp algorithms/greedy.cpp
PYTHON   ?= python3
SEED     ?= 2026
ARGS     ?=

help:
	echo "Opciones disponibles:"
	echo "  make help       - Muestra este mensaje (default)"
	echo "  make testcases  - Genera los casos de prueba (SEED=2026; ARGS='--max-n 16' para limitar)"
	echo "  make run        - Compila (con -O2) y ejecuta las mediciones"
	echo "  make debug      - Compila (con -g -Wall) y ejecuta en modo debug"
	echo "  make plot       - Genera los gráficos a partir de los CSV"
	echo "  make all        - Ejecuta las mediciones y genera los gráficos (requiere casos de prueba)"
	echo "  make clean      - Elimina binarios, salidas, mediciones y gráficos (conserva grid_input)"
	echo "  Variables: PYTHON='uv run' para usar uv, SEED=<n>, ARGS=<opciones del generador>"

all: run plot

run: grid_pathfinding
	echo "Ejecutando..."
	./grid_pathfinding

debug: grid_pathfinding_debug
	echo "Ejecutando [debug]..."
	./grid_pathfinding_debug

grid_pathfinding: $(SRC) $(HDR)
	echo "Compilando..."
	$(CXX) $(CXXFLAGS) -O2 -o $@ $(SRC)

grid_pathfinding_debug: $(SRC) $(HDR)
	echo "Compilando [debug]..."
	$(CXX) $(CXXFLAGS) -g -Wall -o $@ $(SRC)

testcases: scripts/grid_generator.py
	echo "Generando casos de prueba..."
	$(PYTHON) scripts/grid_generator.py --seed $(SEED) $(ARGS)

plot: scripts/plot_generator.py
	echo "Generando gráficos..."
	$(PYTHON) scripts/plot_generator.py

clean:
	rm -f grid_pathfinding grid_pathfinding_debug
	rm -rf data/grid_input/* data/grid_output/* data/measurements/* data/plots/*
