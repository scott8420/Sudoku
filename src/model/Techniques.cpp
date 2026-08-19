#include "model/Technique.hpp"

#include <algorithm>
#include <utility>

// ─────────────────────────────────────────────────────────────────────────────
// The technique units.
//
// Each technique is a pure scan of a WorkGrid that reports Findings. They are
// grouped here, cheapest first, in the same order as the ladder they build.
// A technique never mutates — it only *describes* what could be done. The
// rater and the teaching filter decide what to do with the description.
//
// Every returned Finding is guaranteed to be progress (a placement, or at
// least one real elimination). Techniques that can only eliminate build their
// elimination list from marks that are actually present, so an empty list
// means "doesn't apply here" and no Finding is emitted. That guarantee is what
// lets the rater attribute each solve step to exactly one rung.
// ─────────────────────────────────────────────────────────────────────────────

namespace sudoku::model {

namespace {

// The 27 houses: 9 rows, 9 columns, 9 boxes.
const std::vector<House>& all_houses() {
    static const std::vector<House> hs = [] {
        std::vector<House> v;
        for (int i = 0; i < N; ++i) v.push_back({HouseKind::Row, i});
        for (int i = 0; i < N; ++i) v.push_back({HouseKind::Col, i});
        for (int i = 0; i < N; ++i) v.push_back({HouseKind::Box, i});
        return v;
    }();
    return hs;
}

std::vector<FCell> house_cells(House h) {
    std::vector<FCell> out;
    out.reserve(N);
    switch (h.kind) {
        case HouseKind::Row:
            for (int c = 0; c < N; ++c) out.push_back({h.index, c});
            break;
        case HouseKind::Col:
            for (int r = 0; r < N; ++r) out.push_back({r, h.index});
            break;
        case HouseKind::Box: {
            int br = (h.index / BOX) * BOX, bc = (h.index % BOX) * BOX;
            for (int r = 0; r < BOX; ++r)
                for (int c = 0; c < BOX; ++c) out.push_back({br + r, bc + c});
            break;
        }
    }
    return out;
}

// ── Naked single: a cell with exactly one candidate ──────────────────────────
std::vector<Finding> naked_single_all(const WorkGrid& w) {
    std::vector<Finding> out;
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < N; ++c)
            if (w.empty(r, c) && count(w.cand(r, c)) == 1) {
                int d = only_digit(w.cand(r, c));
                Finding f;
                f.technique = Technique::NakedSingle;
                f.cells     = {{r, c}};
                f.digits    = {d};
                f.placement = Placement{r, c, d};
                out.push_back(std::move(f));
            }
    return out;
}

// ── Hidden single: a digit with exactly one home in a house ──────────────────
std::vector<Finding> hidden_single_all(const WorkGrid& w) {
    std::vector<Finding> out;
    for (const House& h : all_houses()) {
        auto cells = house_cells(h);
        for (int d = 1; d <= N; ++d) {
            const FCell* home = nullptr;
            int homes = 0;
            for (const FCell& cell : cells)
                if (w.empty(cell.row, cell.col) && has(w.cand(cell.row, cell.col), d)) {
                    home = &cell;
                    if (++homes > 1) break;
                }
            if (homes == 1) {
                Finding f;
                f.technique = Technique::HiddenSingle;
                f.cells     = {*home};
                f.digits    = {d};
                f.houses    = {h};
                f.placement = Placement{home->row, home->col, d};
                out.push_back(std::move(f));
            }
        }
    }
    return out;
}

// ── Locked candidates: pointing (box→line) and claiming (line→box) ───────────
// Pointing: within a box, if every candidate for digit d lies on a single row
// (or column), d can be struck from that row (column) outside the box.
// Claiming: within a row (column), if every candidate for d lies in a single
// box, d can be struck from the rest of that box.
std::vector<Finding> locked_candidates_all(const WorkGrid& w) {
    std::vector<Finding> out;

    auto positions = [&](House h, int d) {
        std::vector<FCell> pos;
        for (const FCell& cell : house_cells(h))
            if (w.empty(cell.row, cell.col) && has(w.cand(cell.row, cell.col), d))
                pos.push_back(cell);
        return pos;
    };

    // Pointing — box → line.
    for (int b = 0; b < N; ++b) {
        for (int d = 1; d <= N; ++d) {
            auto pos = positions({HouseKind::Box, b}, d);
            if (pos.size() < 2) continue;  // one home would be a hidden single

            bool same_row = std::all_of(pos.begin(), pos.end(),
                                        [&](const FCell& p) { return p.row == pos[0].row; });
            bool same_col = std::all_of(pos.begin(), pos.end(),
                                        [&](const FCell& p) { return p.col == pos[0].col; });

            if (same_row) {
                int r = pos[0].row;
                std::vector<CandidateRef> elim;
                for (int c = 0; c < N; ++c)
                    if (Grid::box_of(r, c) != b && w.empty(r, c) && has(w.cand(r, c), d))
                        elim.push_back({r, c, d});
                if (!elim.empty()) {
                    Finding f;
                    f.technique    = Technique::LockedCandidates;
                    f.cells        = pos;
                    f.digits       = {d};
                    f.houses       = {{HouseKind::Box, b}, {HouseKind::Row, r}};
                    f.eliminations = std::move(elim);
                    out.push_back(std::move(f));
                }
            }
            if (same_col) {
                int c = pos[0].col;
                std::vector<CandidateRef> elim;
                for (int r = 0; r < N; ++r)
                    if (Grid::box_of(r, c) != b && w.empty(r, c) && has(w.cand(r, c), d))
                        elim.push_back({r, c, d});
                if (!elim.empty()) {
                    Finding f;
                    f.technique    = Technique::LockedCandidates;
                    f.cells        = pos;
                    f.digits       = {d};
                    f.houses       = {{HouseKind::Box, b}, {HouseKind::Col, c}};
                    f.eliminations = std::move(elim);
                    out.push_back(std::move(f));
                }
            }
        }
    }

    // Claiming — line → box.
    for (HouseKind lk : {HouseKind::Row, HouseKind::Col}) {
        for (int i = 0; i < N; ++i) {
            for (int d = 1; d <= N; ++d) {
                auto pos = positions({lk, i}, d);
                if (pos.size() < 2) continue;

                int b = Grid::box_of(pos[0].row, pos[0].col);
                bool same_box = std::all_of(pos.begin(), pos.end(), [&](const FCell& p) {
                    return Grid::box_of(p.row, p.col) == b;
                });
                if (!same_box) continue;

                std::vector<CandidateRef> elim;
                for (const FCell& cell : house_cells({HouseKind::Box, b})) {
                    bool on_line = (lk == HouseKind::Row) ? (cell.row == i) : (cell.col == i);
                    if (!on_line && w.empty(cell.row, cell.col) &&
                        has(w.cand(cell.row, cell.col), d))
                        elim.push_back({cell.row, cell.col, d});
                }
                if (!elim.empty()) {
                    Finding f;
                    f.technique    = Technique::LockedCandidates;
                    f.cells        = pos;
                    f.digits       = {d};
                    f.houses       = {{lk, i}, {HouseKind::Box, b}};
                    f.eliminations = std::move(elim);
                    out.push_back(std::move(f));
                }
            }
        }
    }

    return out;
}

// ── Naked pair: two cells in a house sharing the same two candidates ─────────
// Those two digits can be struck from every other cell in the house.
std::vector<Finding> naked_pair_all(const WorkGrid& w) {
    std::vector<Finding> out;
    for (const House& h : all_houses()) {
        auto cells = house_cells(h);
        std::vector<FCell> empties;
        for (const FCell& cell : cells)
            if (w.empty(cell.row, cell.col)) empties.push_back(cell);

        for (size_t i = 0; i < empties.size(); ++i) {
            Mask mi = w.cand(empties[i].row, empties[i].col);
            if (count(mi) != 2) continue;
            for (size_t j = i + 1; j < empties.size(); ++j) {
                Mask mj = w.cand(empties[j].row, empties[j].col);
                if (mj != mi) continue;  // identical two-candidate set

                std::vector<int> pair;
                for (int d = 1; d <= N; ++d) if (has(mi, d)) pair.push_back(d);

                std::vector<CandidateRef> elim;
                for (const FCell& cell : empties) {
                    if ((cell.row == empties[i].row && cell.col == empties[i].col) ||
                        (cell.row == empties[j].row && cell.col == empties[j].col))
                        continue;
                    for (int d : pair)
                        if (has(w.cand(cell.row, cell.col), d))
                            elim.push_back({cell.row, cell.col, d});
                }
                if (!elim.empty()) {
                    Finding f;
                    f.technique    = Technique::NakedPair;
                    f.cells        = {empties[i], empties[j]};
                    f.digits       = pair;
                    f.houses       = {h};
                    f.eliminations = std::move(elim);
                    out.push_back(std::move(f));
                }
            }
        }
    }
    return out;
}

// ── Hidden pair: two digits confined to the same two cells in a house ────────
// In those two cells, every OTHER candidate can be struck.
std::vector<Finding> hidden_pair_all(const WorkGrid& w) {
    std::vector<Finding> out;
    for (const House& h : all_houses()) {
        auto cells = house_cells(h);

        auto homes_of = [&](int d) {
            std::vector<FCell> hs;
            for (const FCell& cell : cells)
                if (w.empty(cell.row, cell.col) && has(w.cand(cell.row, cell.col), d))
                    hs.push_back(cell);
            return hs;
        };
        auto same_two = [](const std::vector<FCell>& a, const std::vector<FCell>& b) {
            if (a.size() != 2 || b.size() != 2) return false;
            auto eq = [](const FCell& x, const FCell& y) {
                return x.row == y.row && x.col == y.col;
            };
            return (eq(a[0], b[0]) && eq(a[1], b[1])) || (eq(a[0], b[1]) && eq(a[1], b[0]));
        };

        for (int d1 = 1; d1 <= N; ++d1) {
            auto h1 = homes_of(d1);
            if (h1.size() != 2) continue;
            for (int d2 = d1 + 1; d2 <= N; ++d2) {
                auto h2 = homes_of(d2);
                if (!same_two(h1, h2)) continue;

                Mask keep = Mask(bit(d1) | bit(d2));
                std::vector<CandidateRef> elim;
                for (const FCell& cell : h1) {
                    Mask extra = Mask(w.cand(cell.row, cell.col) & ~keep);
                    for (int d = 1; d <= N; ++d)
                        if (has(extra, d)) elim.push_back({cell.row, cell.col, d});
                }
                if (!elim.empty()) {
                    Finding f;
                    f.technique    = Technique::HiddenPair;
                    f.cells        = h1;
                    f.digits       = {d1, d2};
                    f.houses       = {h};
                    f.eliminations = std::move(elim);
                    out.push_back(std::move(f));
                }
            }
        }
    }
    return out;
}

// Wrap a find_all into "return the first application, if any". Every Finding
// these produce is progress by construction, so first() is a valid step.
std::optional<Finding> first_of(std::vector<Finding> all) {
    if (all.empty()) return std::nullopt;
    return std::move(all.front());
}

}  // namespace

const char* name(Technique t) {
    switch (t) {
        case Technique::NakedSingle:      return "naked-single";
        case Technique::HiddenSingle:     return "hidden-single";
        case Technique::LockedCandidates: return "locked-candidates";
        case Technique::NakedPair:        return "naked-pair";
        case Technique::HiddenPair:       return "hidden-pair";
    }
    return "unknown";
}

const std::vector<TechniqueEntry>& ladder() {
    // The order IS the cost order IS the difficulty vocabulary. A new rung is a
    // new entry inserted at its cost position; nothing else changes.
    static const std::vector<TechniqueEntry> rungs = {
        {Technique::NakedSingle,
         [](const WorkGrid& w) { return first_of(naked_single_all(w)); }, naked_single_all},
        {Technique::HiddenSingle,
         [](const WorkGrid& w) { return first_of(hidden_single_all(w)); }, hidden_single_all},
        {Technique::LockedCandidates,
         [](const WorkGrid& w) { return first_of(locked_candidates_all(w)); }, locked_candidates_all},
        {Technique::NakedPair,
         [](const WorkGrid& w) { return first_of(naked_pair_all(w)); }, naked_pair_all},
        {Technique::HiddenPair,
         [](const WorkGrid& w) { return first_of(hidden_pair_all(w)); }, hidden_pair_all},
    };
    return rungs;
}

int ladder_rank(Technique t) {
    const auto& rungs = ladder();
    for (int i = 0; i < int(rungs.size()); ++i)
        if (rungs[i].id == t) return i;
    return -1;
}

}  // namespace sudoku::model
