#pragma once
#include "model/Grid.hpp"
#include "model/Rater.hpp"
#include <cstdint>
#include <random>

namespace sudoku::model {

class WorkGrid;  // fwd — used only in a private method signature

// Generator — produce puzzles that are (a) uniquely solvable and (b) rated at
// a requested difficulty band.
//
// The design follows CANON's coupling of generation to the rater: difficulty
// is never "remove more clues", it's "keep digging while the rated band stays
// at or below the target, then accept only if it landed exactly on target."
// Clue count is a side effect; the histogram is the truth. Because the rater
// runs on every trial removal, generation is the engine's most expensive
// operation — still milliseconds for a 9×9, but the place to look first if a
// future board size makes it feel slow.
//
// Digging is 180°-rotationally symmetric (a cell and its partner are removed
// together) for the clean, modern look a good Sudoku surface wants.
class Generator {
public:
    Generator();                              // seeded from std::random_device
    explicit Generator(std::uint32_t seed);   // reproducible

    Grid full_solution();   // a random complete, valid grid (no cell marked given)
    Grid unique_puzzle();   // dug as far as uniqueness allows; any resulting band

    // A puzzle rated exactly `target`, by generate → dig-within-band → retry.
    // Bounded by max_attempts. If the budget runs out, returns the closest-
    // rated puzzle found and reports its actual band via `actual` (if non-null).
    Grid generate(Difficulty target, int max_attempts = 300, Difficulty* actual = nullptr);

private:
    bool fill(WorkGrid& w);   // randomized backtracking fill of a full solution
    std::mt19937 m_rng;
};

}  // namespace sudoku::model
