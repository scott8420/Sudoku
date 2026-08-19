#include "model/Rater.hpp"
#include "model/Technique.hpp"
#include "model/WorkGrid.hpp"

namespace sudoku::model {

const char* name(Difficulty d) {
    switch (d) {
        case Difficulty::Easy:   return "easy";
        case Difficulty::Medium: return "medium";
        case Difficulty::Hard:   return "hard";
        case Difficulty::Expert: return "expert";
    }
    return "unknown";
}

int DifficultyProfile::steps_at_or_above(int from_rank) const {
    int n = 0;
    for (int i = from_rank; i < int(histogram.size()); ++i) n += histogram[i];
    return n;
}

Difficulty DifficultyProfile::band() const {
    // ── Banding policy (first cut) ────────────────────────────────────────────
    // With the current five-rung ladder the rungs group naturally:
    //   rank 0..1  singles          (the "basic" line)
    //   rank 2     locked candidates (intermediate)
    //   rank 3..4  pairs            (advanced, for now the top of the ladder)
    //
    //   not solved by the ladder → Expert  (needs a technique past hidden pair)
    //   only singles used        → Easy
    //   locked candidates, no pairs → Medium
    //   any pair technique used  → Hard
    //
    // As real rungs land above hidden pair (X-wing, XY-wing…), "Expert" stops
    // being "the ladder gave up" and becomes "the solve genuinely leaned on the
    // rare rungs" — at which point this policy tightens to use
    // steps_at_or_above() thresholds. The histogram is the durable truth; this
    // function is just today's readout of it.
    if (!solved) return Difficulty::Expert;

    const int locked_rank = ladder_rank(Technique::LockedCandidates);
    const int pair_rank   = ladder_rank(Technique::NakedPair);

    if (steps_at_or_above(pair_rank) > 0)   return Difficulty::Hard;
    if (steps_at_or_above(locked_rank) > 0) return Difficulty::Medium;
    return Difficulty::Easy;
}

DifficultyProfile Rater::rate(const Grid& g) {
    DifficultyProfile prof;
    prof.histogram.assign(ladder().size(), 0);

    WorkGrid w = WorkGrid::from(g);
    const auto& rungs = ladder();

    // Cheapest-first: each pass, walk the ladder from the bottom and apply the
    // first rung that produces a progressing finding. Then start over from the
    // bottom — a placement often re-enables a cheaper technique elsewhere, and
    // we always want to credit the cheapest one.
    bool progressed = true;
    while (progressed && !w.solved()) {
        progressed = false;
        for (int i = 0; i < int(rungs.size()); ++i) {
            auto found = rungs[i].find(w);
            if (!found || !found->is_progress()) continue;

            if (found->placement) {
                const Placement& p = *found->placement;
                w.place(p.row, p.col, p.digit);
            }
            for (const CandidateRef& e : found->eliminations)
                w.eliminate(e.row, e.col, e.digit);

            prof.histogram[i] += 1;
            prof.steps        += 1;
            progressed = true;
            break;  // restart from the cheapest rung
        }
    }

    prof.solved = w.solved();
    return prof;
}

}  // namespace sudoku::model
