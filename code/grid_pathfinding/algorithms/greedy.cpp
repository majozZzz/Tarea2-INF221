// Codecademy. (s.f.). Greedy algorithm explained. https://www.codecademy.com/article/greedy-algorithm-explained
// Laaksonen, A. (2018). Competitive programmer's handbook. https://cses.fi/book/book.pdf
#include <vector>
#include <string>
#include "../path_result.h"

PathResult solveGreedy(int n, int m, int v, int p,
                       const std::vector<std::vector<int>>& types,
                       const std::vector<std::vector<long long>>& values) {

    if (types[0][0] == 0) return {false, 0, "", 0};

    int i = 0, j = 0, lives = v;
    long long score = values[0][0];
    std::string path;
    path.reserve(n + m - 2);

    const int di[2] = {0, 1};
    const int dj[2] = {1, 0};
    const char mv[2] = {'R', 'D'};

    while (i < n - 1 || j < m - 1) {
        int bestK = -1;
        long long bestGain = 0;

        for (int k = 0; k < 2; ++k) {
            int ni = i + di[k], nj = j + dj[k];
            if (ni >= n || nj >= m) continue;

            int t = types[ni][nj];
            if (t == 0) continue;
            if (t == -1 && lives == 1) continue;

            long long gain = (t == -1) ? -static_cast<long long>(p) : values[ni][nj];
            if (bestK == -1 || gain > bestGain) {
                bestK = k;
                bestGain = gain;
            }
        }

        if (bestK == -1) return {false, 0, "", 0};

        i += di[bestK];
        j += dj[bestK];
        if (types[i][j] == -1) --lives;
        score += bestGain;
        path.push_back(mv[bestK]);
    }

    return {true, score, path, lives};
}
