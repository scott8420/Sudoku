#include "model/Generator.hpp"
#include "model/Solver.hpp"
#include "model/WorkGrid.hpp"

#include <algorithm>
#include <cstdlib>
#include <numeric>

namespace sudoku::model {

Generator::Generator() : m_rng(std::random_device{}()) {}
Generator::Generator(std::uint32_t seed) : m_rng(seed) {}

bool Generator::fill(WorkGrid& w) {
    // First empty cell in reading order; shuffled candidate digits give a
    // different valid solution each run. WorkGrid is a value type, so the
    // checkpoint-and-restore backtrack is just a copy.
    int r = -1, c = -1;
    for (int i = 0; i < CELLS && r < 0; ++i)
        if (w.empty(i / N, i % N)) { r = i / N; c = i % N; }
    if (r < 0) return true;  // full → solved

    std::vector<int> digits;
    Mask m = w.cand(r, c);
    for (int d = 1; d <= N; ++d) if (has(m, d)) digits.push_back(d);
    std::shuffle(digits.begin(), digits.end(), m_rng);

    for (int d : digits) {
        WorkGrid save = w;
        if (w.place(r, c, d) && fill(w)) return true;
        w = save;
    }
    return false;
}

Grid Generator::full_solution() {
    WorkGrid w = WorkGrid::from(Grid{});  // empty board, all candidates open
    fill(w);
    return Grid::scratch_from_values(w.to_string());
}

Grid Generator::unique_puzzle() {
    std::string vals = full_solution().to_string();  // 81 digits, no dots

    // Only the lower half of the symmetric pairs (indices 0..40) drives the
    // dig; each carries its 180° partner. Processing each pair once keeps the
    // loop honest and avoids re-testing an already-decided pair.
    std::vector<int> order(41);
    std::iota(order.begin(), order.end(), 0);
    std::shuffle(order.begin(), order.end(), m_rng);

    for (int i : order) {
        int j = CELLS - 1 - i;
        if (vals[i] == '.') continue;

        char a = vals[i], b = vals[j];
        vals[i] = '.';
        if (j != i) vals[j] = '.';

        if (Solver::count_solutions(Grid::from_givens(vals), 2) != 1) {
            vals[i] = a;  // removal broke uniqueness → put it back
            vals[j] = b;
        }
    }
    return Grid::from_givens(vals);
}

Grid Generator::generate(Difficulty target, int max_attempts, Difficulty* actual) {
    Grid best;
    int  best_gap = 1000;
    Difficulty best_band = Difficulty::Easy;

    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        std::string vals = full_solution().to_string();

        std::vector<int> order(41);
        std::iota(order.begin(), order.end(), 0);
        std::shuffle(order.begin(), order.end(), m_rng);

        // Dig while the rating stays at or below target. A removal is accepted
        // only if the puzzle stays unique AND its band doesn't exceed target;
        // that upper bound is what keeps an "easy" request from digging its way
        // into a hard puzzle.
        for (int i : order) {
            int j = CELLS - 1 - i;
            if (vals[i] == '.') continue;

            char a = vals[i], b = vals[j];
            vals[i] = '.';
            if (j != i) vals[j] = '.';

            Grid trial = Grid::from_givens(vals);
            bool ok = Solver::count_solutions(trial, 2) == 1 &&
                      rank(Rater::rate(trial).band()) <= rank(target);
            if (!ok) { vals[i] = a; vals[j] = b; }
        }

        Grid puzzle = Grid::from_givens(vals);
        Difficulty band = Rater::rate(puzzle).band();
        if (band == target) {
            if (actual) *actual = band;
            return puzzle;
        }

        int gap = std::abs(rank(band) - rank(target));
        if (gap < best_gap) { best_gap = gap; best = puzzle; best_band = band; }
    }

    if (actual) *actual = best_band;
    return best;
}

}  // namespace sudoku::model
