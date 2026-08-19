#pragma once
#include "model/Grid.hpp"
#include "model/Finding.hpp"
#include <vector>

namespace sudoku::model {

// Difficulty bands. Rank order matters (Easy < Medium < Hard < Expert): the
// generator digs "while the band stays ≤ target", so the enum's order is load
// bearing, not cosmetic.
enum class Difficulty { Easy, Medium, Hard, Expert };

const char* name(Difficulty d);
inline int rank(Difficulty d) { return int(d); }

// A DifficultyProfile is the fingerprint of a puzzle's solve path: a histogram
// over the ladder recording how many steps each technique carried, plus
// whether the basic ladder finished it. This is Scott's difficulty metric made
// concrete — difficulty is the DISTRIBUTION of work across the rungs, not the
// single hardest technique. Easy = all the mass on the low rungs; Expert = the
// solve leans on the rare ones (or the basic ladder can't finish at all).
struct DifficultyProfile {
    std::vector<int> histogram;  // indexed by ladder rank; size == ladder().size()
    int  steps  = 0;             // total ladder steps taken
    bool solved = false;         // did the basic ladder finish the puzzle?

    // Steps carried by rungs at or above `from_rank` — the "how much of the
    // solve was the rare stuff" number the banding is built on.
    int steps_at_or_above(int from_rank) const;

    Difficulty band() const;     // classification policy (see Rater.cpp)
};

// Rater — run the ladder as a human-style solver and report the profile.
class Rater {
public:
    // Solve g by always applying the CHEAPEST rung that makes progress,
    // recording which rung made each step. Cheapest-first is the discipline
    // that makes the histogram a real number: a step is credited to the
    // simplest technique that could have made it, never a fancier one. Does
    // not mutate g.
    static DifficultyProfile rate(const Grid& g);
};

}  // namespace sudoku::model
