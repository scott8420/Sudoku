#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// sudoku::model — the pure puzzle engine.
//
// Everything under this namespace is standard C++17 and nothing else: no
// gtkmm, no glib, no spdlog, not even in the .cpp files. That is a hard
// contract, not a preference — a student can lift the whole model/ subtree
// into any project and compile it with a bare `g++ -std=c++17`, no
// alterations. The GTK UI is a separate consumer on the other side of this
// line; presentation (colour, prose, widgets) never crosses into here.
//
// Grid is the root of the model's vocabulary: the 9×9 dimensions, the
// digit-set bit representation, and the board geometry (peers, houses) all
// live here because the rest of the engine — WorkGrid, the techniques, the
// rater, the generator — is written in these terms.
// ─────────────────────────────────────────────────────────────────────────────

namespace sudoku::model {

// ── Dimensions ────────────────────────────────────────────────────────────────
inline constexpr int N     = 9;      // side length
inline constexpr int BOX   = 3;      // box side length
inline constexpr int CELLS = N * N;  // 81

// ── Digit-set as a bit mask ───────────────────────────────────────────────────
// A cell's candidate set (its pencil marks) is a 9-bit mask: bit (d-1) is set
// iff digit d is possible. Masks are the engine's inner-loop currency, so the
// helpers are tiny and free. They are written without compiler builtins
// (no __builtin_popcount) so the "compiles anywhere with std C++17" promise
// holds on every toolchain, not just GCC/Clang.
using Mask = std::uint16_t;
inline constexpr Mask ALL_DIGITS = 0x1FF;             // digits 1..9

inline constexpr Mask bit(int digit) { return Mask(1u << (digit - 1)); }
inline bool has(Mask m, int digit)   { return (m & bit(digit)) != 0; }
inline int  count(Mask m) { int n = 0; while (m) { m &= Mask(m - 1); ++n; } return n; }

// The sole digit of a one-bit mask; 0 if the mask is empty or has >1 bit.
inline int only_digit(Mask m) {
    for (int d = 1; d <= N; ++d) if (m == bit(d)) return d;
    return 0;
}

// ── A single cell ─────────────────────────────────────────────────────────────
struct Cell {
    int  value = 0;      // 0 = empty, 1..9 = filled
    bool given = false;  // a clue fixed at construction; immutable thereafter
    Mask notes = 0;      // user pencil marks (in memory only); meaningful when empty
};

// ── The board ─────────────────────────────────────────────────────────────────
//
// Grid holds values and given-flags. Given-ness is immutable *by construction*:
// the only code that can mark a cell as a given is a named factory
// (from_givens), and there is no public setter that promotes an ordinary cell
// to a given afterward. So the frame from CANON holds — the wrong thing (a
// mutated clue) isn't guarded against at every call site; it's simply absent
// from the type's surface. place()/clear() refuse given cells and report it.
class Grid {
public:
    Grid() = default;  // empty board, nothing given

    // A puzzle: '1'..'9' become givens, '.'/'0'/space become empty cells.
    // This is the ONLY path that sets given=true — the immutability guarantee
    // rests on that being true.
    static Grid from_givens(const std::string& s81);

    // A scratch board with the same values but NOTHING marked given (every
    // cell mutable). The generator digs and tests on scratch boards; a
    // delivered puzzle always comes back through from_givens().
    static Grid scratch_from_values(const std::string& s81);

    int  value(int r, int c) const { return m_cells[idx(r, c)].value; }
    bool given(int r, int c) const { return m_cells[idx(r, c)].given; }
    bool empty(int r, int c) const { return m_cells[idx(r, c)].value == 0; }

    // Write / erase a digit. Both refuse a given cell (return false) so a
    // caller can never corrupt the clue set; place() also rejects an
    // out-of-range digit. Neither enforces Sudoku legality — that belongs to
    // the solver and conflict-check, not to storage.
    //
    // Neither touches pencil marks. Notes are the player's own reasoning, and
    // whether a placement should invalidate any of it is a judgement about
    // CORRECTNESS — which this layer deliberately doesn't make (the model has
    // no answer key). The caller that knows the solution asks for the pruning
    // explicitly, via erase_peer_notes() below.
    bool place(int r, int c, int digit);
    bool clear(int r, int c);

    // ── User pencil marks ─────────────────────────────────────────────────────
    // Notes are in-memory play state, not part of the puzzle: they live on the
    // non-given cells and are the user's "possible numbers." Nothing here is
    // serialized — there is no save file (yet), so notes are simply session
    // state on the model.
    //
    // A note mask SURVIVES a value being written over it. A filled cell shows
    // its value, not its marks, so the retained mask is inert until the value
    // is cleared — at which point the player's reasoning is still there. That
    // is the whole point: a guess, right or wrong, must not silently destroy
    // work the player did before making it.
    Mask notes(int r, int c) const { return m_cells[idx(r, c)].notes; }
    bool toggle_note(int r, int c, int digit);  // empty non-given only; returns new state
    void set_notes(int r, int c, Mask m);        // set the whole mask (Fill candidates)
    void clear_notes(int r, int c) { m_cells[idx(r, c)].notes = 0; }

    // Strike `digit` from the pencil marks of every peer of (r,c) — the pruning
    // that a CONFIRMED placement justifies. Explicit and caller-driven on
    // purpose: only a caller holding the solution can know a placement is
    // confirmed, and this layer holds no solution. Givens are peers like any
    // other cell (their masks are empty anyway). Pure bookkeeping — it never
    // reads or writes a value.
    void erase_peer_notes(int r, int c, int digit);

    // The naive candidate mask for an empty cell: every digit not already
    // present among its 20 peers. Derived from values only, so it can never
    // disagree with the board. Returns 0 for a filled cell. This is the seed
    // pencil-mark set that WorkGrid and the techniques whittle down.
    Mask candidates(int r, int c) const;

    bool complete() const;                 // every cell filled (not necessarily valid)
    bool has_conflict() const;             // any peer pair shares a digit
    bool conflict_at(int r, int c) const;  // does this filled cell clash with a peer?
    bool solved() const { return complete() && !has_conflict(); }

    std::string to_string() const;         // 81 chars, '.' for empty; round-trips

    // ── Geometry, shared by the whole engine ──────────────────────────────────
    static int idx(int r, int c)    { return r * N + c; }
    static int box_of(int r, int c) { return (r / BOX) * BOX + (c / BOX); }

    // The 20 peers of a cell (same row, column, or box; excludes itself),
    // as flat 0..80 indices. Computed once and cached.
    static const std::vector<int>& peers(int r, int c);

private:
    std::array<Cell, CELLS> m_cells{};
};

}  // namespace sudoku::model
