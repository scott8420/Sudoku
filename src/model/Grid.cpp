#include "model/Grid.hpp"

namespace sudoku::model {

// The two factories are the only writers of `given`. They fill m_cells
// directly (they're members, so the private array is in reach) rather than
// going through place(), because place() deliberately can't set given-ness.
Grid Grid::from_givens(const std::string& s81) {
    Grid g;
    int i = 0;
    for (char ch : s81) {
        if (i >= CELLS) break;
        if (ch >= '1' && ch <= '9') {
            g.m_cells[i] = Cell{ch - '0', /*given=*/true};
            ++i;
        } else if (ch == '.' || ch == '0') {
            g.m_cells[i] = Cell{0, false};
            ++i;
        }
        // any other char (whitespace, newline) is skipped, so pretty-printed
        // 9-line boards parse too
    }
    return g;
}

Grid Grid::scratch_from_values(const std::string& s81) {
    Grid g;
    int i = 0;
    for (char ch : s81) {
        if (i >= CELLS) break;
        if (ch >= '1' && ch <= '9') { g.m_cells[i] = Cell{ch - '0', false}; ++i; }
        else if (ch == '.' || ch == '0') { g.m_cells[i] = Cell{0, false}; ++i; }
    }
    return g;
}

bool Grid::place(int r, int c, int digit) {
    if (digit < 1 || digit > N) return false;
    Cell& cell = m_cells[idx(r, c)];
    if (cell.given) return false;   // clue set is immutable
    cell.value = digit;
    // The cell's own marks are left alone. They aren't drawn while a value sits
    // here, and if the value comes back off, the player's reasoning is intact.
    // (s7 wiped them and pruned the peers from inside place(). That fired on
    // any placement, right or wrong, so a bad guess destroyed pencil marks its
    // own removal couldn't restore. Pruning is now the caller's explicit call —
    // see erase_peer_notes.)
    return true;
}

void Grid::erase_peer_notes(int r, int c, int digit) {
    if (digit < 1 || digit > N) return;
    for (int p : peers(r, c)) m_cells[p].notes &= Mask(~bit(digit));
}

void Grid::set_notes(int r, int c, Mask m) {
    Cell& cell = m_cells[idx(r, c)];
    if (cell.given || cell.value != 0) return;  // notes only on empty non-givens
    cell.notes = m;
}

bool Grid::toggle_note(int r, int c, int digit) {
    if (digit < 1 || digit > N) return false;
    Cell& cell = m_cells[idx(r, c)];
    if (cell.given || cell.value != 0) return false;  // notes only on empty non-givens
    cell.notes ^= bit(digit);
    return has(cell.notes, digit);
}

bool Grid::clear(int r, int c) {
    Cell& cell = m_cells[idx(r, c)];
    if (cell.given) return false;
    cell.value = 0;
    return true;
}

Mask Grid::candidates(int r, int c) const {
    if (!empty(r, c)) return 0;
    Mask m = ALL_DIGITS;
    for (int p : peers(r, c)) {
        int v = m_cells[p].value;
        if (v) m &= Mask(~bit(v));
    }
    return m;
}

bool Grid::complete() const {
    for (const Cell& c : m_cells) if (c.value == 0) return false;
    return true;
}

bool Grid::conflict_at(int r, int c) const {
    int v = value(r, c);
    if (v == 0) return false;
    for (int p : peers(r, c)) if (m_cells[p].value == v) return true;
    return false;
}

bool Grid::has_conflict() const {
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < N; ++c)
            if (conflict_at(r, c)) return true;
    return false;
}

std::string Grid::to_string() const {
    std::string s(CELLS, '.');
    for (int i = 0; i < CELLS; ++i)
        if (m_cells[i].value) s[i] = char('0' + m_cells[i].value);
    return s;
}

const std::vector<int>& Grid::peers(int r, int c) {
    // Built once, on first use: a flat table of the 20 peers of every cell.
    // Function-local static so there's no static-init-order concern and no
    // cost until the engine actually runs.
    static const std::array<std::vector<int>, CELLS> table = [] {
        std::array<std::vector<int>, CELLS> t;
        for (int rr = 0; rr < N; ++rr) {
            for (int cc = 0; cc < N; ++cc) {
                std::vector<int>& peers = t[idx(rr, cc)];
                int br = (rr / BOX) * BOX, bc = (cc / BOX) * BOX;
                for (int k = 0; k < N; ++k) {
                    if (k != cc) peers.push_back(idx(rr, k));  // row
                    if (k != rr) peers.push_back(idx(k, cc));  // col
                }
                for (int dr = 0; dr < BOX; ++dr)               // box
                    for (int dc = 0; dc < BOX; ++dc) {
                        int pr = br + dr, pc = bc + dc;
                        if (pr != rr && pc != cc) peers.push_back(idx(pr, pc));
                    }
            }
        }
        return t;
    }();
    return table[idx(r, c)];
}

}  // namespace sudoku::model
