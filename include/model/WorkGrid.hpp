#pragma once
#include "model/Grid.hpp"
#include <array>
#include <string>

namespace sudoku::model {

// WorkGrid — the engine's mutable solving state.
//
// Grid is the authoritative puzzle: values plus immutable givens, the thing
// that serializes and that the UI shows. WorkGrid is the scratchpad that the
// solver, the techniques, and the rater operate on. It carries a value for
// every cell AND a live candidate mask for every empty cell, and it keeps the
// two in lockstep so a technique never reads a stale pencil mark.
//
// This is a real seam, not a convenience copy: "what the puzzle is" (Grid)
// and "what solving it looks like right now" (WorkGrid) are different
// concepts with different invariants, and separating them keeps each type
// honest. Techniques are pure functions of a WorkGrid; the rater owns one and
// drives it; Grid never gains a candidate field it would have to keep valid.
//
// The candidate invariant: for every empty cell, cand(r,c) is exactly the set
// of digits not yet placed among that cell's peers. place() restores it by
// striking the placed digit from every peer; eliminate() removes one mark by
// hand (that's what a technique's finding does). A filled cell has cand == 0.
class WorkGrid {
public:
    // Seed from a board: values copied, candidate masks set to Grid's naive
    // candidates for empty cells. The invariant holds by construction.
    static WorkGrid from(const Grid& g);

    int  value(int r, int c) const { return m_val[Grid::idx(r, c)]; }
    Mask cand (int r, int c) const { return m_cand[Grid::idx(r, c)]; }
    bool empty(int r, int c) const { return m_val[Grid::idx(r, c)] == 0; }

    // Place a digit and restore the invariant (clear this cell's marks, strike
    // the digit from every peer's marks). Returns false if `digit` was not a
    // candidate here — i.e. a contradiction — which the solver reads as a dead
    // branch. WorkGrid is a value type (fixed arrays), so the backtracking
    // solver copies it cheaply and never has to undo.
    bool place(int r, int c, int digit);

    // Remove one pencil mark. Returns true iff a mark was actually there, so
    // the caller can tell real progress from a no-op.
    bool eliminate(int r, int c, int digit);

    bool solved() const;             // every cell filled
    std::string to_string() const;   // Grid-style 81-char values, '.' for empty

private:
    std::array<int,  CELLS> m_val{};
    std::array<Mask, CELLS> m_cand{};
};

}  // namespace sudoku::model
