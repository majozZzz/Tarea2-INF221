// swolf. (2024, 25 de enero). How to measure memory usage in C++. Life Is A Journey. https://mrswolf.github.io/memory-usage-cpp/
// Free Software Foundation. (s.f.). Getting the return or frame address of a function. En Using the GNU Compiler Collection (GCC). Recuperado el 24 de noviembre de 2026, de https://gcc.gnu.org/onlinedocs/gcc/Return-Address.html
// Stack Exchange. (2014, 13 de marzo). Measuring execution time of a function in C++ [Pregunta en un foro]. Stack Overflow. https://stackoverflow.com/questions/22387586/measuring-execution-time-of-a-function-in-c
// Stack Exchange. (2009, 20 de julio). Any reason to overload global new and delete? [Pregunta en un foro]. Stack Overflow. https://stackoverflow.com/questions/1152511/any-reason-to-overload-global-new-and-delete
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <cstdlib>
#include "path_result.h"

namespace fs = std::filesystem;
namespace ch = std::chrono;

// --- Medición de Memoria ---
thread_local struct MemoryMetrics {
    size_t currentUsage = 0;
    size_t peakUsage = 0;

    void reset() {
        peakUsage = currentUsage;
    }
} metrics;

void* operator new(size_t size) {
    metrics.currentUsage += size;
    if (metrics.currentUsage > metrics.peakUsage) {
        metrics.peakUsage = metrics.currentUsage;
    }
    return malloc(size);
}

void operator delete(void* memory, size_t size) noexcept {
    metrics.currentUsage -= size;
    free(memory);
}

void operator delete(void* memory) noexcept {
    free(memory);
}

// --- Estructuras de Datos ---
struct Grid {
    int n, m, v, p;
    std::vector<std::vector<int>> types;
    std::vector<std::vector<long long>> values;
};

// --- Medición de Tiempo ---
// La primera corrida mide tiempo y memoria. Si fue muy rápida (< 1 ms) el resultado es
// ruidoso, así que se repite el algoritmo y se reporta el MÍNIMO de todas las corridas.
// La memoria siempre se toma de la primera corrida.
static volatile long long sink = 0;

template <class F>
long long refineTimeNs(long long first_ns, F&& solve) {
    constexpr long long THRESHOLD_NS = 1'000'000; // 1 ms
    constexpr int REPS = 20;

    long long best = first_ns;
    if (first_ns >= THRESHOLD_NS) return best;

    for (int k = 0; k < REPS; ++k) {
        const auto t0 = ch::steady_clock::now();
        PathResult r = solve();
        const auto t1 = ch::steady_clock::now();
        sink = sink + r.score;
        best = std::min(best, static_cast<long long>(
            ch::duration_cast<ch::nanoseconds>(t1 - t0).count()));
    }
    return best;
}

// --- Lector de Grilla ---
bool readGrid(const std::string& path, Grid& g) {
    std::ifstream in(path);
    if (!(in >> g.n >> g.m >> g.v >> g.p) || g.n <= 0 || g.m <= 0) return false;

    g.types.assign(g.n, std::vector<int>(g.m));
    g.values.assign(g.n, std::vector<long long>(g.m));
    for (auto& row : g.types)  for (int& x : row) in >> x;
    for (auto& row : g.values) for (long long& x : row) in >> x;

    return static_cast<bool>(in);
}

// --- Guardar Archivo de Salida ---
void saveOutputFile(const fs::path& output_path, const PathResult& res) {
    std::ofstream out(output_path);
    if (!res.found) {
        out << "SIN SOLUCION\n";
    } else {
        out << res.score << "\n";
        out << res.path << "\n";
        out << res.remaining_lives << "\n";
    }
}

// --- Escritura de columnas ---
void writeRow(std::ofstream& out, const Grid& g, const std::string& t, const std::string& d,
              const std::string& s, const PathResult& res, long long time_ns, size_t mem_bytes) {
    out << g.n << ',' << g.m << ',' << t << ',' << d << ',' << s << ',' << (res.found ? 0 : 1) << ',';
    if (res.found) out << res.score; else out << "NA";
    out << ',' << time_ns << ',' << mem_bytes;
}

// --- Firmas de los Algoritmos ---
extern std::size_t bf_peak_stack_bytes;
PathResult solveBruteForce(int n, int m, int v, int p,
                           const std::vector<std::vector<int>>& types,
                           const std::vector<std::vector<long long>>& values);

PathResult solveDP(int n, int m, int v, int p,
                   const std::vector<std::vector<int>>& types,
                   const std::vector<std::vector<long long>>& values);

PathResult solveGreedy(int n, int m, int v, int p,
                       const std::vector<std::vector<int>>& types,
                       const std::vector<std::vector<long long>>& values);

// --- Función Principal ---
int main() {
    const fs::path input_dir {"data/grid_input/"};
    const fs::path output_dir {"data/grid_output/"};
    const fs::path measure_dir {"data/measurements/"};

    if (!fs::exists(input_dir)) {
        std::cerr << "El directorio de entrada " << input_dir << " no existe.\n";
        return 1;
    }
    fs::create_directories(output_dir);
    fs::create_directories(measure_dir);

    std::vector<fs::path> files;
    for (const auto& e : fs::directory_iterator(input_dir))
        if (e.is_regular_file() && e.path().extension() == ".txt") files.push_back(e.path());
    std::sort(files.begin(), files.end());

    const std::regex r(R"((\d+)_(\d+)_([a-z]+)_(D\d+)_([a-z])\.txt)");

    std::ofstream csv_bf(measure_dir / "bruteforce.csv");
    std::ofstream csv_dp(measure_dir / "dp.csv");
    std::ofstream csv_g(measure_dir / "greedy.csv");
    if (!csv_bf || !csv_dp || !csv_g) {
        std::cerr << "No se pudieron crear los CSV en " << measure_dir << '\n';
        return 1;
    }
    const std::string cols = "n,m,tipo,dominio,muestra,falla,score,time_ns,mem_bytes";
    csv_bf << cols << '\n';
    csv_dp << cols << '\n';
    csv_g << cols << ",gap\n";

    for (const auto& path : files) {
        const std::string name = path.filename().string();
        std::smatch mt;
        if (!std::regex_match(name, mt, r)) continue;

        Grid g;
        if (!readGrid(path.string(), g)) {
            std::cerr << "No se pudo leer " << path << '\n';
            continue;
        }

        const std::string base_name = path.filename().stem().string();
        const std::string t = mt[3].str();
        const std::string d = mt[4].str();
        const std::string s = mt[5].str();

        std::cout << "Procesando instancia: N=" << g.n << ", M=" << g.m
                  << ", Tipo=" << t << ", Dominio=" << d << ", Muestra=" << s << '\n';

        // --- FUERZA BRUTA ---
        const bool bf_ran = (g.n + g.m <= 32);
        PathResult res_bf{false, 0, "", 0};
        long long time_bf = -1;
        size_t mem_bf = 0;

        if (bf_ran) {
            metrics.reset();
            size_t mem_start = metrics.currentUsage;
            auto start = ch::steady_clock::now();

            res_bf = solveBruteForce(g.n, g.m, g.v, g.p, g.types, g.values);

            auto end = ch::steady_clock::now();
            time_bf = ch::duration_cast<ch::nanoseconds>(end - start).count();
            mem_bf = (metrics.peakUsage - mem_start) + bf_peak_stack_bytes;

            time_bf = refineTimeNs(time_bf, [&] {
                return solveBruteForce(g.n, g.m, g.v, g.p, g.types, g.values);
            });

            saveOutputFile(output_dir / (base_name + "_bruteforce_out.txt"), res_bf);
        }

        // --- PROGRAMACIÓN DINÁMICA ---
        metrics.reset();
        size_t mem_start_dp = metrics.currentUsage;
        auto start_dp = ch::steady_clock::now();

        PathResult res_dp = solveDP(g.n, g.m, g.v, g.p, g.types, g.values);

        auto end_dp = ch::steady_clock::now();
        long long time_dp = ch::duration_cast<ch::nanoseconds>(end_dp - start_dp).count();
        size_t mem_dp = metrics.peakUsage - mem_start_dp;

        time_dp = refineTimeNs(time_dp, [&] {
            return solveDP(g.n, g.m, g.v, g.p, g.types, g.values);
        });

        saveOutputFile(output_dir / (base_name + "_dp_out.txt"), res_dp);

        // --- GREEDY ---
        metrics.reset();
        size_t mem_start_g = metrics.currentUsage;
        auto start_g = ch::steady_clock::now();

        PathResult res_g = solveGreedy(g.n, g.m, g.v, g.p, g.types, g.values);

        auto end_g = ch::steady_clock::now();
        long long time_g = ch::duration_cast<ch::nanoseconds>(end_g - start_g).count();
        size_t mem_g = metrics.peakUsage - mem_start_g;

        time_g = refineTimeNs(time_g, [&] {
            return solveGreedy(g.n, g.m, g.v, g.p, g.types, g.values);
        });

        saveOutputFile(output_dir / (base_name + "_greedy_out.txt"), res_g);

        // --- Validaciones y Métricas de Calidad ---
        // Fuerza Bruta y DP deben coincidir: tanto en haber encontrado camino como en el puntaje
        if (bf_ran && (res_bf.found != res_dp.found ||
            (res_bf.found && res_bf.score != res_dp.score))) {
            std::cerr << "[ERROR] Discrepancia entre Fuerza Bruta y DP en " << base_name
            << " (BF: " << (res_bf.found ? std::to_string(res_bf.score) : "SIN SOLUCION")
            << ", DP: " << (res_dp.found ? std::to_string(res_dp.score) : "SIN SOLUCION")
            << ")\n";
        }

        if (!res_dp.found) {
            std::cerr << "[ERROR] DP no encontró camino en " << base_name
            << " (se esperaba al menos uno)\n";
        }

        // Tasa de falla de Greedy
        bool greedy_failed = !res_g.found;

        //Brecha de Greedy
        bool has_gap = false;
        long long gap = 0;
        if (!greedy_failed && res_dp.found) {
            gap = res_dp.score - res_g.score;
            has_gap = true;
            if (gap < 0) {
                std::cerr << "[ERROR] Brecha negativa (" << gap << ") en " << base_name
                << ": Greedy superó al óptimo\n";
            }
        }

        if (bf_ran) {
            writeRow(csv_bf, g, t, d, s, res_bf, time_bf, mem_bf);
            csv_bf << '\n' << std::flush;
        }
        writeRow(csv_dp, g, t, d, s, res_dp, time_dp, mem_dp);
        csv_dp << '\n' << std::flush;

        writeRow(csv_g, g, t, d, s, res_g, time_g, mem_g);
        csv_g << ',' << (has_gap ? std::to_string(gap) : "NA") << '\n' << std::flush;
    }

    std::cout << "¡Mediciones y ejecución completadas exitosamente!\n";
    return 0;
}
