#include "Teach.hpp"

namespace sudoku {

Glib::ustring display_name(model::Technique t) {
    switch (t) {
        case model::Technique::NakedSingle:      return "Naked Single";
        case model::Technique::HiddenSingle:     return "Hidden Single";
        case model::Technique::LockedCandidates: return "Locked Candidates";
        case model::Technique::NakedPair:        return "Naked Pair";
        case model::Technique::HiddenPair:       return "Hidden Pair";
    }
    return "Unknown";
}

Glib::ustring explain(model::Technique t) {
    switch (t) {
        case model::Technique::NakedSingle:
            return "A cell has only one possible number left, so that number "
                   "must go there.";
        case model::Technique::HiddenSingle:
            return "Within a row, column, or box, a number has only one cell it "
                   "can go in — even if that cell has other candidates.";
        case model::Technique::LockedCandidates:
            return "When a number's only spots in a box all lie on one line (or "
                   "vice versa), it can be removed from the rest of that line "
                   "(or box).";
        case model::Technique::NakedPair:
            return "Two cells in a unit share the same two candidates, so those "
                   "two numbers can be removed from every other cell in the unit.";
        case model::Technique::HiddenPair:
            return "Two numbers can only go in the same two cells of a unit, so "
                   "every other candidate can be removed from those two cells.";
    }
    return "";
}

}  // namespace sudoku
