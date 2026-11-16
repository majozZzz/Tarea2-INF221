#include <vector>
#include <string>
#include "../path_result.h"

// Diseño: regla greedy del enunciado. En cada paso se evalúan Derecha (R) y Abajo (D);
// se descartan las celdas fuera de la grilla, las murallas y las minas si queda 1 vida.
// Gana la de mayor ganancia neta (g si es normal, -P si es mina); en empate, Derecha
// (R se evalúa primero y la comparación es estrictamente '>').
// Tiempo O(N+M); memoria extra O(N+M) (solo el string del camino).
PathResult solveGreedy(int n, int m, int v, int p,
                       const std::vector<std::vector<int>>& types,
                       const std::vector<std::vector<long long>>& values) {

    if (types[0][0] == 0) return {false, 0, "", 0};

    int i = 0, j = 0, lives = v;
    long long score = values[0][0];
    std::string path;
    path.reserve(n + m - 2); // un greedy nunca da más de N+M-2 pasos

    const int di[2] = {0, 1};       // R, D
    const int dj[2] = {1, 0};
    const char mv[2] = {'R', 'D'};

    while (i < n - 1 || j < m - 1) {
        int bestK = -1;
        long long bestGain = 0;

        for (int k = 0; k < 2; ++k) {
            int ni = i + di[k], nj = j + dj[k];
            if (ni >= n || nj >= m) continue;           // fuera de la grilla

            int t = types[ni][nj];
            if (t == 0) continue;                       // muralla
            if (t == -1 && lives == 1) continue;        // mina con 1 vida: destrucción inminente

            long long gain = (t == -1) ? -static_cast<long long>(p) : values[ni][nj];
            if (bestK == -1 || gain > bestGain) {       // '>' estricto: empate -> Derecha
                bestK = k;
                bestGain = gain;
            }
        }

        if (bestK == -1) return {false, 0, "", 0};      // sin movimiento seguro: SIN SOLUCION

        i += di[bestK];
        j += dj[bestK];
        if (types[i][j] == -1) --lives;
        score += bestGain;
        path.push_back(mv[bestK]);
    }

    return {true, score, path, lives};
}
