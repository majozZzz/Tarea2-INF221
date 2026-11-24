// cp-algorithms. (s.f.). Introduction to dynamic programming. https://cp-algorithms.com/dynamic_programming/intro-to-dp.html
// GeeksforGeeks. (s.f.). Tabulation vs memoization. https://www.geeksforgeeks.org/dsa/tabulation-vs-memoization/
// Laaksonen, A. (2018). Competitive programmer's handbook. https://cses.fi/book/book.pdf
#include <vector>
#include <string>
#include <algorithm>
#include <climits>
#include <cstddef>
#include "../path_result.h"

PathResult solveDP(int n, int m, int v, int p,
                   const std::vector<std::vector<int>>& types,
                   const std::vector<std::vector<long long>>& values) {

    if (types[0][0] == 0 || types[n - 1][m - 1] == 0) {
        return {false, 0, "", 0};
    }

    const long long NEG = LLONG_MIN / 2;
    const std::size_t L = static_cast<std::size_t>(v) + 1;
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
            const int dl = mine ? 1 : 0;

            for (int l = 1; l + dl <= v; ++l) {
                const int pl = l + dl;
                long long best = NEG;
                char mv = 0;

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

    std::string path;
    path.reserve(n + m - 2);
    int i = n - 1, j = m - 1, l = best_lives;
    while (i != 0 || j != 0) {
        char mv = from[idx(i, j, l)];
        path.push_back(mv);
        if (types[i][j] == -1) ++l;
        if (mv == 'R') --j; else --i;
    }
    std::reverse(path.begin(), path.end());

    return {true, best_score, path, best_lives};
}
