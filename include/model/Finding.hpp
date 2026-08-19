#pragma once
#include <optional>
#include <vector>

namespace sudoku::model {

// ─────────────────────────────────────────────────────────────────────────────
// The findings vocabulary — LOAD-BEARING.
//
// A Finding is the engine's unit of explanation: "technique T applies here;
// this is the pattern that justifies it; this is what it does to the board."
// It is deliberately PRESENTATION-FREE — no colour, no prose, no widgets. The
// UI keys off `technique` to choose a colour treatment and to look up the
// human-language sentence; the model reports only structure. That is the seam
// that lets sudoku::model lift into any project: the teacher travels with the
// generator, and nothing GTK crosses the line.
//
// The whole technique ladder is built on this one shape, so it is designed up
// front to fit the obscure techniques (X-wing, XY-wing, Swordfish…) without
// reshaping. Any inference, however exotic, is expressible as:
//
//     (premise cells, the digits at play, the houses they live in)
//         ⟶  (pencil marks to strike)  and/or  (a value to place)
//
// `cells` + `digits` + `houses` carry the premise; `eliminations` + optional
// `placement` carry the consequence. A hidden single fills one cell, one
// digit, one house, and a placement. A naked pair fills two cells, two digits,
// one house, and a list of eliminations. An X-wing fills four cells, one
// digit, two houses, and eliminations. Same struct throughout.
//
// If this shape is wrong, now is the cheap moment to change it — no UI is
// built on it yet.
// ─────────────────────────────────────────────────────────────────────────────

enum class Technique {
    NakedSingle,       // a cell with exactly one candidate
    HiddenSingle,      // a digit with exactly one home in a house
    LockedCandidates,  // pointing / claiming (box⇄line interactions)
    NakedPair,         // two cells sharing the same two candidates in a house
    HiddenPair,        // two digits confined to the same two cells in a house
    // New rungs slot in here as one self-contained unit each; the vocabulary
    // above already accommodates them:
    //   XWing, XYWing, Swordfish, ...
};

// A stable identifier string for a technique (e.g. "naked-single"). This is an
// ID for logs/tests/skins to key on — NOT the user-facing sentence, which is
// the UI's job. Defined in Techniques.cpp.
const char* name(Technique t);

// A unit of the board: one row, one column, or one box (index 0..8).
enum class HouseKind { Row, Col, Box };
struct House { HouseKind kind; int index; };

struct FCell        { int row, col; };          // a board position
struct CandidateRef { int row, col, digit; };   // one specific pencil mark
struct Placement    { int row, col, digit; };   // a value to write

struct Finding {
    Technique technique;

    // ── Premise: what justifies the inference ─────────────────────────────────
    // Which fields are populated depends on the technique, but every technique
    // speaks in this vocabulary, so the UI can highlight uniformly.
    std::vector<FCell> cells;
    std::vector<int>   digits;
    std::vector<House> houses;

    // ── Consequence: what it does ─────────────────────────────────────────────
    std::vector<CandidateRef> eliminations;   // marks to strike
    std::optional<Placement>  placement;      // a value to write, if any

    // True iff applying this finding actually changes the board. The rater and
    // the ladder-solver only ever apply progressing findings, which is what
    // makes each solve step attributable to exactly one technique.
    bool is_progress() const {
        return placement.has_value() || !eliminations.empty();
    }
};

}  // namespace sudoku::model
