#pragma once
#include "model/Grid.hpp"

namespace sudoku::model {

// Solver — the exhaustive backtracking engine.
//
// This is the ground truth about a board: does it have a solution, exactly
// one, or many? It is separate from the technique ladder on purpose. The
// ladder models how a *person* solves (and so measures difficulty); the Solver
// just decides the facts. The generator needs the facts — "is this puzzle
// still unique after I remove that clue?" — and doesn't care how a human would
// get there.
//
// Both entry points use fewest-candidates-first (MRV) ordering, which prunes
// the search hard enough that even minimal puzzles resolve instantly.
class Solver {
public:
    // Fill g's empty cells with a valid solution, in place. Returns false if
    // the board has no solution (g is then left partially filled). Given cells
    // are never touched. Deterministic (candidate order low→high); the
    // generator supplies its own randomized fill when it wants variety.
    static bool solve(Grid& g);

    // Count solutions, stopping once `limit` is reached. Does not mutate g.
    // count == 0 → unsolvable, 1 → unique, ≥2 → ambiguous. The default limit
    // of 2 is all uniqueness testing needs and keeps the search cheap.
    static int count_solutions(const Grid& g, int limit = 2);

    static bool has_unique_solution(const Grid& g) {
        return count_solutions(g, 2) == 1;
    }
};

}  // namespace sudoku::model
