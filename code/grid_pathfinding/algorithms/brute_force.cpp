// Laaksonen, A. (2018). Competitive programmer's handbook. https://cses.fi/book/book.pdf
// Peroni, S. (s.f.). Brute-force algorithms. En Computational thinking and programming. https://comp-think.github.io/book/06.pdf
// Programming Algorithms. (s.f.). Brute force. https://www.programmingalgorithms.com/algorithm/brute-force/cpp/
#include <cstddef>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include "../path_result.h"

std::size_t bf_peak_stack_bytes = 0;
static std::uintptr_t bf_stack_base = 0;


static const int  DI[2]  = {0, 1};
static const int  DJ[2]  = {1, 0};
static const char MOV[2] = {'R', 'D'};

void explorePaths(int i, int j, int n, int m, int p, const std::vector<std::vector<int>>& types,
                  const std::vector<std::vector<long long>>& values, int current_lives,
                  long long current_score, std::string& current_path, bool& found_any,
                  long long& max_score, std::string& best_path, int& best_lives) {

    std::uintptr_t here = reinterpret_cast<std::uintptr_t>(__builtin_frame_address(0));
    bf_peak_stack_bytes = std::max(bf_peak_stack_bytes,
                          static_cast<std::size_t>(bf_stack_base - here));

    if (i == n - 1 && j == m - 1) {
        if (!found_any || current_score > max_score) {
            max_score = current_score;
            best_path = current_path;
            best_lives = current_lives;
            found_any = true;
        }
        return;
    }

    for (int k = 0; k < 2; k++) {
        int ni = i + DI[k];
        int nj = j + DJ[k];

        if (ni >= n || nj >= m) continue;
        int cell_type = types[ni][nj];
        if (cell_type == 0) continue;

        int next_lives = current_lives;
        long long next_score = current_score;
        if (cell_type == -1) {
            next_lives--;
            next_score -= p;
        } else {
            next_score += values[ni][nj];
        }

        if (next_lives <= 0) continue;

        current_path.push_back(MOV[k]);
        explorePaths(ni, nj, n, m, p, types, values,
                     next_lives, next_score, current_path,
                     found_any, max_score, best_path, best_lives);
        current_path.pop_back();
    }
}

PathResult solveBruteForce(int n, int m, int v, int p,
                           const std::vector<std::vector<int>>& types,
                           const std::vector<std::vector<long long>>& values) {
    bool found_any = false;
    long long max_score = 0;
    std::string best_path;
    int best_lives = 0;

    if (types[0][0] == 0) return {false, 0, "", 0};

    std::string current_path;
    bf_peak_stack_bytes = 0;
    bf_stack_base = reinterpret_cast<std::uintptr_t>(__builtin_frame_address(0));
    explorePaths(0, 0, n, m, p, types, values,
                v, values[0][0], current_path,
                found_any, max_score, best_path, best_lives);

    if (!found_any) return {false, 0, "", 0};
    return {true, max_score, best_path, best_lives};
}
