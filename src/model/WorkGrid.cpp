#include "model/WorkGrid.hpp"

namespace sudoku::model {

WorkGrid WorkGrid::from(const Grid& g) {
    WorkGrid w;
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            int i = Grid::idx(r, c);
            w.m_val[i]  = g.value(r, c);
            w.m_cand[i] = g.empty(r, c) ? g.candidates(r, c) : Mask(0);
        }
    }
    return w;
}

bool WorkGrid::place(int r, int c, int digit) {
    int i = Grid::idx(r, c);
    // A placement must be a live candidate; if it isn't, the branch that led
    // here is contradictory. Reporting false lets the solver prune.
    if (!has(m_cand[i], digit) && m_val[i] == 0) return false;
    m_val[i]  = digit;
    m_cand[i] = 0;
    for (int p : Grid::peers(r, c)) m_cand[p] &= Mask(~bit(digit));
    return true;
}

bool WorkGrid::eliminate(int r, int c, int digit) {
    int i = Grid::idx(r, c);
    if (!has(m_cand[i], digit)) return false;  // already gone — not progress
    m_cand[i] &= Mask(~bit(digit));
    return true;
}

bool WorkGrid::solved() const {
    for (int v : m_val) if (v == 0) return false;
    return true;
}

std::string WorkGrid::to_string() const {
    std::string s(CELLS, '.');
    for (int i = 0; i < CELLS; ++i) if (m_val[i]) s[i] = char('0' + m_val[i]);
    return s;
}

}  // namespace sudoku::model
