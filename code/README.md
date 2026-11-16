## Grid pathfinding

Algoritmos: Fuerza Bruta, Programación Dinámica, Greedy.

Todos los comandos se ejecutan desde `code/grid_pathfinding/`, porque el
programa usa rutas relativas a `data/`.

```
code/grid_pathfinding/
├── algorithms/
│   ├── brute_force.cpp
│   ├── dynamic_programming.cpp
│   └── greedy.cpp
├── scripts/
│   ├── grid_generator.py     (entregado con el material)
│   └── plot_generator.py
├── data/
│   ├── grid_input/           instancias {n}_{m}_{t}_{d}_{s}.txt
│   ├── grid_output/          salidas {n}_{m}_{t}_{d}_{s}_{algoritmo}_out.txt
│   ├── measurements/         CSV con tiempo, memoria y puntaje
│   └── plots/                gráficos PNG
|__ path_result.h
├── grid_pathfinding.cpp
└── makefile
```

### Progama Principal
#### Flujo del programa

`grid_pathfinding.cpp` ejecuta sistemáticamente los tres algoritmos sobre todas
las instancias de `data/grid_input/`:

1. Lista los archivos `.txt` de `data/grid_input/` en orden alfabético y se
   queda con los que cumplen el patrón `{n}_{m}_{tipo}_{dominio}_{muestra}.txt`
   (los demás se ignoran). Crea `data/grid_output/` y `data/measurements/` si no existen.
2. Para cada instancia lee la grilla (`N M V P`, matriz de tipos y matriz de valores).
3. Ejecuta **fuerza bruta** solo si `N + M ≤ 32`, luego **DP** y **greedy**
   (estos dos siempre), midiendo tiempo y memoria de cada uno.
4. Escribe un archivo de salida por algoritmo y una fila por algoritmo en su CSV.
5. Verifica la consistencia de los resultados (ver más abajo).

#### Medición de tiempo

- Se usa `std::chrono::steady_clock` (reloj monótono) y se reporta en
  **nanosegundos**. Solo se mide la llamada al algoritmo: la lectura de la
  grilla y la escritura de resultados quedan fuera.
- La primera ejecución de cada algoritmo mide tiempo y memoria. Si tardó menos
  de **1 ms**, la medición es ruidosa, así que el algoritmo se repite 20 veces y
  se reporta el **mínimo** de todas las corridas. Si tardó 1 ms o más, se
  reporta esa única medición.
- Las corridas repetidas acumulan el puntaje en una variable `volatile` para
  que el compilador no las elimine.

#### Medición de memoria

La memoria se mide con dos mecanismos, según el algoritmo:

- **Heap (los tres algoritmos):** se sobreescriben `operator new` y
  `operator delete` globales, que llevan la cuenta de los bytes pedidos al heap
  y su máximo. Antes de cada algoritmo se reinicia el máximo
  (`metrics.reset()`), y se reporta el **pico de heap durante la ejecución**
  menos el uso previo. Las matrices de la grilla se reservan antes y no cuentan.
- **Stack (solo fuerza bruta):** su memoria está sobre todo en la pila de la
  recursión, que `operator new` no ve. En cada llamada recursiva
  `brute_force.cpp` lee la dirección del frame (`__builtin_frame_address(0)`) y
  guarda la mayor distancia respecto del frame de `solveBruteForce`
  (`bf_peak_stack_bytes`). La memoria reportada es pico de heap + pico de stack.
  DP (tabulación) y greedy no usan recursión, así que no se mide su stack.

Detalles que conviene tener presentes:

- Siempre se toma de la **primera corrida**; las repeticiones de tiempo no la afectan.
- La medición del stack es aproximada: depende del compilador y de la
  optimización (en las mediciones del informe, ~192 bytes por nivel de
  recursión) y no incluye frames de funciones de la biblioteca estándar.
  Cualitativamente refleja el Θ(N+M) esperado.
- Un `std::string` de hasta 15 caracteres vive dentro del objeto (optimización
  SSO) y no reserva heap. Por eso, en grillas pequeñas, la memoria de greedy
  puede ser 0 bytes.
- Memoria esperada de DP: `9 · N · M · (V+1)` bytes (8 de `long long` más 1 de
  la tabla `from`), más el string del camino.

#### Archivos de salida (`data/grid_output/`)

Para cada instancia y algoritmo se escribe
`{n}_{m}_{tipo}_{dominio}_{muestra}_{algoritmo}_out.txt`, con `algoritmo` en
`bruteforce`, `dp` o `greedy`:

```
<puntaje>
<secuencia de N+M-2 movimientos R/D>
<vidas restantes al llegar a (N,M)>
```

Si no se encontró camino válido, el archivo contiene únicamente `SIN SOLUCION`.
Los archivos de fuerza bruta solo existen para instancias con `N + M ≤ 32`.

#### CSV de mediciones (`data/measurements/`)

Se generan `bruteforce.csv`, `dp.csv` y `greedy.csv`, con una fila por instancia.
El de fuerza bruta solo tiene las instancias donde se ejecutó (120 de 288).

| Columna | Descripción |
|---------|-------------|
| `n`, `m` | Filas y columnas de la grilla. |
| `tipo` | `libre`, `murallas`, `minas` o `mixta`. |
| `dominio` | `D10` o `D1000`. |
| `muestra` | `a`, `b` o `c`. |
| `falla` | `1` si el algoritmo no encontró camino (`SIN SOLUCION`), `0` si lo encontró. |
| `score` | Puntaje del camino; `NA` si `falla = 1`. |
| `time_ns` | Tiempo de ejecución en nanosegundos (ver "Medición de tiempo"). |
| `mem_bytes` | Pico de memoria de heap en bytes (ver "Medición de memoria"). |
| `gap` | Solo en `greedy.csv`: brecha Δ = s(DP) − s(greedy). `NA` si greedy falló (no es un 0, que sería una brecha real). |

La brecha usa la solución de DP como referencia del óptimo, porque la fuerza
bruta no se ejecuta en las instancias grandes.

#### Verificaciones

El programa comprueba lo siguiente e informa por `stderr` con el prefijo
`[ERROR]` (no detiene la ejecución):

- Fuerza bruta y DP coinciden, tanto en haber encontrado camino como en el
  **puntaje**, en todas las instancias donde ambos se ejecutan. El camino y las
  vidas finales pueden diferir si hay varios caminos óptimos, porque sus
  criterios de desempate no son idénticos.
- DP encuentra camino en toda instancia (el generador garantiza al menos uno).
- La brecha nunca es negativa (greedy no puede superar al óptimo).

### Scripts

#### `scripts/grid_generator.py`

Generador de casos de prueba entregado con el material de la tarea (se invoca
con `make testcases`). Produce las 288 instancias en `data/grid_input/`.

#### `scripts/plot_generator.py`

Lee los CSV de `data/measurements/` (`bruteforce.csv`, `dp.csv`, `greedy.csv`)
y guarda 8 gráficos PNG en `data/plots/`. Si falta algún CSV, avisa y omite sus
gráficos. Se ejecuta con `make plot` (o dentro de `make all`), o directamente:

```bash
python3 scripts/plot_generator.py
```

Las rutas se calculan a partir de la ubicación del script, por lo que se puede
ejecutar desde cualquier directorio. Requiere `numpy`, `pandas` y `matplotlib`.
Usa el backend `Agg`, que solo escribe archivos y no necesita pantalla.

##### Gráficos generados

| Archivo | Contenido |
|---|---|
| `time_bruteforce.png`, `time_dp.png`, `time_greedy.png` | Tiempo de ejecución (ns) vs tamaño N·M, con una curva por tipo de grilla y la curva teórica escalada. |
| `mem_bruteforce.png`, `mem_dp.png`, `mem_greedy.png` | Memoria máxima (bytes) vs tamaño N·M, con una curva por tipo de grilla y la curva teórica. |
| `gap_vs_size.png` | Brecha media Δ = s(C\*) − s(C_greedy) vs tamaño, en dos paneles (D10 y D1000). Solo se promedian los casos en que greedy no falla. |
| `failure_rate_vs_size.png` | Tasa de falla de greedy (% de `SIN SOLUCION`) vs tamaño, en dos paneles (D10 y D1000). |

##### Cómo se construyen las curvas

- Cada punto es el **promedio** de las muestras a, b y c; en tiempo y memoria
  también se promedian ambos dominios. Con tres muestras por punto, la tasa de
  falla solo puede valer 0, 33, 67 o 100 %.
- Los ejes de tamaño (N·M) están en escala logarítmica base 2. Tiempo y memoria
  también usan eje y logarítmico. Los valores 0 (por ejemplo, memoria 0 por la
  optimización SSO de `std::string` en grillas pequeñas) no se pueden dibujar
  en escala log: se omiten y el script lo avisa.
- Como M ∈ {N, 2N}, cada valor de N·M corresponde a una única forma de grilla.

##### Curvas teóricas

| Gráfico | Curva teórica f(N, M) |
|---|---|
| Tiempo, fuerza bruta | C(N+M−2, N−1) |
| Tiempo, DP | N·M·V |
| Tiempo, greedy | N+M |
| Memoria, DP | N·M·(V+1)·9 bytes (exacta, sin constante) |
| Memoria, fuerza bruta y greedy | N+M |

**Por qué se escalan.** La notación asintótica solo fija la forma de la curva.
f(N, M) cuenta unidades de trabajo y el tiempo se mide en ns, así que se
multiplica f por una constante c (ns por unidad de trabajo, que depende de la
máquina y del compilador). En ejes logarítmicos, multiplicar por c solo
desplaza la curva verticalmente (log(c·f) = log c + log f): no cambia su
pendiente ni su curvatura. Lo que se contrasta con la teoría es la forma, no el
nivel.

**Cómo se calcula c.** Por mínimos cuadrados en escala log:

> log c = media( log yᵢ − log fᵢ )   ⟹   c = media geométrica de yᵢ / fᵢ

Solo se usa la grilla `libre` (sin podas) y solo los tamaños dentro de una
**ventana de calibración**, que se sombrea en cada gráfico. La ventana excluye
los tamaños donde la teoría no debería valer:

| Gráfico | Ventana (N·M) | Motivo |
|---|---|---|
| Tiempo greedy | ≥ 4096 | Antes domina el costo fijo del reloj. |
| Tiempo DP | 128 a 8192 | Antes domina el costo fijo; después la tabla no cabe en caché y el costo por celda sube. |
| Tiempo fuerza bruta | todos | El costo por camino es casi constante desde el inicio. |
| Memoria fuerza bruta y greedy | todos los valores > 0 | — |

Fuera de la ventana la curva teórica se sigue dibujando, y ahí se ve si los
datos calzan o se separan. El script imprime por consola la constante c de cada
gráfico, con un significado físico (por ejemplo, ns por paso en greedy o ns por
camino en fuerza bruta).

##### Parámetros editables

Al inicio del script: `V` (vidas, 3), `TIPOS`, `DOMINIOS`, los diccionarios
`TEORIA_TIEMPO` y `VENTANA_CALIBRACION`, y los estilos de las curvas.
