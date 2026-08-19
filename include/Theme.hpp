#pragma once
#include <string>

namespace sudoku {

// ─────────────────────────────────────────────────────────────────────────────
// Theme — the board's presentation tokens. LOAD-BEARING (soft).
//
// The board never hardcodes a colour; it draws from named tokens, so a skin is
// data, not code. This struct is the token SET a JSON skin file populates at
// runtime (see Skins.hpp) — so the *set of token names* here is the skin
// format's compatibility surface. Adding or renaming a token is a format change.
//
// A skin JSON is a flat map: { "name": "...", "background": "#rrggbb[aa]", ... }.
// Missing tokens fall back to default_dark(), so a partial or malformed skin
// still yields a usable Theme (round-trip robustness).
// ─────────────────────────────────────────────────────────────────────────────

struct Rgba { double r, g, b, a; };

struct Theme {
    std::string name;   // display name (shown in the skin picker)

    Rgba background;    // board field
    Rgba grid_line;     // thin cell separators
    Rgba box_border;    // thick 3×3 box separators
    Rgba label;         // a..i row/column coordinates (dim chrome)
    Rgba given_digit;   // immutable clue digits
    Rgba entry_digit;   // user-placed digits
    Rgba pencil;        // small "possible number" pencil marks — matches `label`
                        // by default: both are quiet annotation around the
                        // digits, not the digits themselves. Separate tokens so
                        // a skin can still split them.
    Rgba selection;     // selected-cell wash (Guess mode)
    Rgba selection_notes; // selected-cell wash while in Notes mode (warm cue)
    Rgba conflict;      // conflicting-digit highlight (also teaching eliminations)
    Rgba mistake;       // a user digit that disagrees with the solution (wrong answer)
    Rgba teach_premise; // teaching: cells that justify a technique
    Rgba teach_place;   // teaching: a cell a technique fills in

    static Theme default_dark() {
        return Theme{
            /*name         */ "Midnight",
            /*background   */ {0.11, 0.12, 0.14, 1.0},
            /*grid_line    */ {0.30, 0.32, 0.36, 1.0},
            /*box_border   */ {0.55, 0.58, 0.64, 1.0},
            /*label        */ {0.42, 0.45, 0.52, 0.85},
            /*given_digit  */ {0.92, 0.93, 0.95, 1.0},
            /*entry_digit  */ {0.45, 0.72, 0.98, 1.0},
            /*pencil       */ {0.42, 0.45, 0.52, 0.85},
            /*selection    */ {0.20, 0.40, 0.66, 0.45},
            /*selection_notes*/ {0.85, 0.33, 0.10, 0.68},
            /*conflict     */ {0.88, 0.36, 0.36, 0.45},
            /*mistake      */ {0.95, 0.38, 0.38, 1.0},
            /*teach_premise*/ {0.85, 0.70, 0.25, 0.28},
            /*teach_place  */ {0.35, 0.72, 0.45, 0.40},
        };
    }
};

}  // namespace sudoku
