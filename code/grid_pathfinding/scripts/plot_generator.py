import math
import os

import matplotlib
import matplotlib.pyplot as plt
from matplotlib.ticker import NullFormatter, NullLocator
import numpy as np
import pandas as pd

matplotlib.use("Agg")

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
MEASUREMENTS_DIR = os.path.join(BASE_DIR, "..", "data", "measurements")
PLOTS_DIR = os.path.join(BASE_DIR, "..", "data", "plots")

V = 3
TIPOS = ["libre", "murallas", "minas", "mixta"]
DOMINIOS = ["D10", "D1000"]

ESTILO = {
    "libre":    ("tab:blue",   "o", "-"),
    "murallas": ("tab:orange", "s", "--"),
    "minas":    ("tab:green",  "^", "-."),
    "mixta":    ("tab:red",    "D", ":"),
}

# Teoría de tiempo: f(N, M) y etiqueta
TEORIA_TIEMPO = {
    "bruteforce": (lambda n, m: float(math.comb(n + m - 2, n - 1)),
                   r"Teórico $\binom{N+M-2}{N-1}$ (escalado)"),
    "dp":         (lambda n, m: float(n * m * V),
                   r"Teórico $N\cdot M\cdot V$ (escalado)"),
    "greedy":     (lambda n, m: float(n + m),
                   r"Teórico $N+M$ (escalado)"),
}

NOMBRE_ALGORITMO = {
    "bruteforce": "Fuerza bruta",
    "dp": "Programación dinámica",
    "greedy": "Greedy"
    }


# --- Carga de datos ---
def cargar(nombre):
    ruta = os.path.join(MEASUREMENTS_DIR, nombre)
    if not os.path.exists(ruta):
        print(f"[AVISO] No existe {ruta}; se omiten sus gráficos.")
        return None
    df = pd.read_csv(ruta, na_values="NA")
    df["size"] = df["n"] * df["m"]
    return df


# --- Utilidades de dibujo ---
def formatear_x(ax, sizes):
    ax.set_xscale("log", base=2)
    ax.set_xticks(sizes)
    ax.set_xticklabels([str(s) for s in sizes], rotation=45, ha="right", fontsize=8)
    ax.xaxis.set_minor_locator(NullLocator())
    ax.xaxis.set_minor_formatter(NullFormatter())
    ax.set_xlabel(r"Tamaño de la grilla ($N\cdot M$)")
    ax.grid(True, which="major", alpha=0.3)


def dibujar_curvas(ax, tab):
    for tipo in TIPOS:
        if tipo not in tab.columns:
            continue
        color, marcador, linea = ESTILO[tipo]
        ax.plot(tab.index, tab[tipo], color=color, marker=marcador, linestyle=linea,
                markersize=5, linewidth=1.5, label=tipo)


def tabla_por_tipo(df, col):
    return df.groupby(["size", "tipo"])[col].mean().unstack("tipo").sort_index()


def dimensiones(df):
    return df.drop_duplicates("size").set_index("size")[["n", "m"]].sort_index()


def calibrar(tab, f_vals):
    ref = tab["libre"] if "libre" in tab.columns else tab.mean(axis=1)
    mascara = (ref > 0) & (f_vals > 0)
    if not mascara.any():
        print("[AVISO] sin puntos para calibrar; se usa c = 1.")
        return 1.0, 0
    c = float(np.exp(np.mean(np.log(ref[mascara]) - np.log(f_vals[mascara]))))
    return c, int(mascara.sum())


# --- Tiempo / memoria vs tamaño ---
def grafico_vs_tamano(df, algoritmo, columna, ylabel, titulo, archivo):
    tab = tabla_por_tipo(df, columna)
    dims = dimensiones(df).loc[tab.index]
    sizes = list(tab.index)

    es_memoria = columna == "mem_bytes"
    if es_memoria and algoritmo == "dp":
        f = pd.Series([n * m * (V + 1) * 9 for n, m in zip(dims["n"], dims["m"])], index=tab.index, dtype=float)
        c, etiqueta = 1.0, r"Teórico $N\cdot M\cdot(V+1)\cdot 9$ bytes"
        print(f"{archivo}: teoría exacta (c = 1, sin calibración)")
    else:
        funcion, etiqueta = TEORIA_TIEMPO[algoritmo]
        if es_memoria:
            funcion, etiqueta = (lambda n, m: float(n + m)), r"Teórico $N+M$ (escalado)"
        f = pd.Series([funcion(n, m) for n, m in zip(dims["n"], dims["m"])], index=tab.index)
        c, k = calibrar(tab, f)
        print(f"{archivo}: c = {c:.4g} (ajuste log-LS con {k} puntos)")

    ceros = int((tab <= 0).sum().sum())
    if ceros:
        print(f"[AVISO] {archivo}: {ceros} valores <= 0 omitidos (escala log).")
    tab = tab.where(tab > 0)

    fig, ax = plt.subplots(figsize=(7.5, 5))
    dibujar_curvas(ax, tab)
    ax.plot(sizes, c * f, color="black", linestyle="--", linewidth=1.8, label=etiqueta)
    ax.set_yscale("log")
    formatear_x(ax, sizes)

    ax.set_ylabel(ylabel)
    ax.set_title(titulo)
    ax.legend(fontsize=8)
    fig.tight_layout()
    fig.savefig(os.path.join(PLOTS_DIR, archivo), dpi=150)
    plt.close(fig)
    print(f"Generado: {archivo}")


# --- Brecha vs tamaño ---
def grafico_brecha(g):
    sizes = sorted(g["size"].unique())
    validos = g.dropna(subset=["gap"])
    fig, axes = plt.subplots(1, 2, figsize=(13, 5))
    for ax, dom in zip(axes, DOMINIOS):
        sub = validos[validos["dominio"] == dom]
        tab = tabla_por_tipo(sub, "gap").reindex(sizes) if not sub.empty else pd.DataFrame(index=sizes)
        dibujar_curvas(ax, tab)
        formatear_x(ax, sizes)
        ax.set_ylabel(r"Brecha media $\Delta = s(C^*) - s(C_{greedy})$")
        ax.set_title(f"Dominio {dom}")
        ax.legend(fontsize=8)
    fig.suptitle("Brecha de Greedy respecto al óptimo (solo casos donde Greedy no falla)")
    fig.tight_layout()
    fig.savefig(os.path.join(PLOTS_DIR, "gap_vs_size.png"), dpi=150)
    plt.close(fig)
    print("Generado: gap_vs_size.png")


# --- Tasa de falla vs tamaño ---
def grafico_falla(g):
    sizes = sorted(g["size"].unique())
    fig, axes = plt.subplots(1, 2, figsize=(13, 5), sharey=True)
    for ax, dom in zip(axes, DOMINIOS):
        sub = g[g["dominio"] == dom]
        tab = (tabla_por_tipo(sub, "falla") * 100).reindex(sizes) if not sub.empty else pd.DataFrame(index=sizes)
        dibujar_curvas(ax, tab)
        formatear_x(ax, sizes)
        ax.set_ylim(-5, 105)
        ax.set_ylabel("Tasa de falla de Greedy (%)")
        ax.set_title(f"Dominio {dom}")
        ax.legend(fontsize=8)
    fig.suptitle("Tasa de falla de Greedy (SIN SOLUCION) según tamaño y tipo de grilla")
    fig.tight_layout()
    fig.savefig(os.path.join(PLOTS_DIR, "failure_rate_vs_size.png"), dpi=150)
    plt.close(fig)
    print("Generado: failure_rate_vs_size.png")


def main():
    os.makedirs(PLOTS_DIR, exist_ok=True)
    datos = {
        "bruteforce": cargar("bruteforce.csv"),
        "dp": cargar("dp.csv"),
        "greedy": cargar("greedy.csv"),
    }

    for alg, df in datos.items():
        if df is None or df.empty:
            continue
        nombre = NOMBRE_ALGORITMO[alg]
        grafico_vs_tamano(df, alg, "time_ns", "Tiempo de ejecución (ns)",
                          f"Tiempo vs tamaño — {nombre}", f"time_{alg}.png")
        grafico_vs_tamano(df, alg, "mem_bytes", "Memoria máxima (bytes)",
                          f"Memoria vs tamaño — {nombre}", f"mem_{alg}.png")

    g = datos["greedy"]
    if g is not None and not g.empty:
        grafico_brecha(g)
        grafico_falla(g)


if __name__ == "__main__":
    main()
