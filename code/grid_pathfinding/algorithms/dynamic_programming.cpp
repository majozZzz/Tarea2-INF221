#include <vector>
#include <string>
#include <algorithm>
#include <climits>
#include <cstddef>
#include "../path_result.h"

// Diseño: tabulación bottom-up (pull). Estado (i, j, l) = mejor puntaje al llegar a (i, j)
// con exactamente l vidas restantes (1 <= l <= v). Tiempo y memoria O(N*M*V).
// Tablas planas: dp (long long) y from (1 byte: 'R' o 'D' = de dónde se llegó).
// Vidas y celda previas se deducen: si (i, j) es mina, el estado previo tenía l + 1 vidas.
PathResult solveDP(int n, int m, int v, int p,
                   const std::vector<std::vector<int>>& types,
                   const std::vector<std::vector<long long>>& values) {

    if (types[0][0] == 0 || types[n - 1][m - 1] == 0) {
        return {false, 0, "", 0};
    }

    const long long NEG = LLONG_MIN / 2; // estado inalcanzable
    const std::size_t L = static_cast<std::size_t>(v) + 1; // vidas 0..v (la 0 nunca se usa)
    auto idx = [&](int i, int j, int l) {
        return (static_cast<std::size_t>(i) * m + j) * L + l;
    };

    std::vector<long long> dp(static_cast<std::size_t>(n) * m * L, NEG);
    std::vector<char> from(static_cast<std::size_t>(n) * m * L, 0);

    dp[idx(0, 0, v)] = values[0][0];

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            if (types[i][j] == 0 || (i == 0 && j == 0)) continue;

            const bool mine = (types[i][j] == -1);
            const long long gain = mine ? -static_cast<long long>(p) : values[i][j];
            const int dl = mine ? 1 : 0; // vidas que cuesta entrar a esta celda

            // l = vidas al quedar en (i, j); debe ser >= 1
            for (int l = 1; l + dl <= v; ++l) {
                const int pl = l + dl; // vidas antes de entrar
                long long best = NEG;
                char mv = 0;

                // Las murallas siempre tienen NEG, no hace falta revisarlas
                if (j > 0) {
                    long long c = dp[idx(i, j - 1, pl)];
                    if (c > best) { best = c; mv = 'R'; }
                }
                if (i > 0) {
                    long long c = dp[idx(i - 1, j, pl)];
                    if (c > best) { best = c; mv = 'D'; }
                }

                if (best != NEG) {
                    dp[idx(i, j, l)] = best + gain;
                    from[idx(i, j, l)] = mv;
                }
            }
        }
    }

    // Mejor puntaje final; ante empate se prefiere terminar con más vidas
    long long best_score = NEG;
    int best_lives = -1;
    for (int l = v; l >= 1; --l) {
        if (dp[idx(n - 1, m - 1, l)] > best_score) {
            best_score = dp[idx(n - 1, m - 1, l)];
            best_lives = l;
        }
    }

    if (best_score == NEG) {
        return {false, 0, "", 0};
    }

    // Reconstrucción desde la meta
    std::string path;
    path.reserve(n + m - 2);
    int i = n - 1, j = m - 1, l = best_lives;
    while (i != 0 || j != 0) {
        char mv = from[idx(i, j, l)];
        path.push_back(mv);
        if (types[i][j] == -1) ++l; // devolver la vida de la mina
        if (mv == 'R') --j; else --i;
    }
    std::reverse(path.begin(), path.end());

    return {true, best_score, path, best_lives};
}
