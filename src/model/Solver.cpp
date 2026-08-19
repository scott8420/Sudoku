#include "model/Solver.hpp"
#include "model/WorkGrid.hpp"

#include <utility>

namespace sudoku::model {

namespace {

// Pick the empty cell with the fewest candidates (Minimum Remaining Values).
// Fewer candidates = fewer branches = a shallower search tree. Returns false
// when the board is full (a solution) or when some empty cell has zero
// candidates (a dead branch, reported via `dead`).
bool pick_mrv(const WorkGrid& w, int& br, int& bc, bool& dead) {
    dead = false;
    int best = N + 1;
    bool found = false;
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < N; ++c)
            if (w.empty(r, c)) {
                int n = count(w.cand(r, c));
                if (n == 0) { dead = true; return false; }
                if (n < best) { best = n; br = r; bc = c; found = true; }
            }
    return found;
}

// Recursive search. `count` accumulates solutions found; recursion stops early
// once `limit` is hit so uniqueness testing doesn't enumerate the whole tree.
// WorkGrid is a value type, so each branch works on its own copy and there is
// no undo step — the copy IS the checkpoint.
void search(WorkGrid w, int limit, int& count, WorkGrid* first_solution) {
    if (count >= limit) return;

    int r = 0, c = 0;
    bool dead = false;
    if (!pick_mrv(w, r, c, dead)) {
        if (dead) return;              // contradiction on this branch
        // No empty cell and not dead → fully and validly filled.
        if (count == 0 && first_solution) *first_solution = w;
        ++count;
        return;
    }

    Mask cand = w.cand(r, c);
    for (int d = 1; d <= N && count < limit; ++d) {
        if (!has(cand, d)) continue;
        WorkGrid next = w;             // branch checkpoint
        if (next.place(r, c, d))       // propagates the digit to peers
            search(std::move(next), limit, count, first_solution);
    }
}

}  // namespace

bool Solver::solve(Grid& g) {
    WorkGrid solution;
    int count = 0;
    search(WorkGrid::from(g), 1, count, &solution);
    if (count == 0) return false;

    // Write the solution's values back into the non-given cells of g.
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < N; ++c)
            if (g.empty(r, c)) g.place(r, c, solution.value(r, c));
    return true;
}

int Solver::count_solutions(const Grid& g, int limit) {
    int count = 0;
    search(WorkGrid::from(g), limit, count, nullptr);
    return count;
}

}  // namespace sudoku::model
