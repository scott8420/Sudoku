#pragma once
#include "model/Finding.hpp"
#include "model/WorkGrid.hpp"
#include <functional>
#include <optional>
#include <vector>

namespace sudoku::model {

// A technique is a pure function of the current pencil-mark state.
//
//   find(w)      → the FIRST place the technique applies, or nothing.
//   find_all(w)  → every place it applies right now.
//
// `find` is what the cheapest-first ladder-solver calls: apply one step, then
// re-scan from the cheapest rung. `find_all` is what the teaching filter
// calls: the user picks a strategy and the board lights up everywhere it
// applies at once, each spot carrying its own explanation. Two surfaces, one
// mechanism — the same detection code serves the solver and the teacher.
struct TechniqueEntry {
    Technique id;
    std::function<std::optional<Finding>(const WorkGrid&)> find;
    std::function<std::vector<Finding>(const WorkGrid&)>   find_all;
};

// The ladder: techniques in strictly increasing cost order. Cheapest-first is
// what makes the difficulty histogram a real number instead of a choice — the
// rater always attributes a step to the simplest technique that could have
// made it, never to a fancy one that merely *also* could have. The ORDER here
// is the difficulty vocabulary; adding a rung is adding a word to it.
const std::vector<TechniqueEntry>& ladder();

// Index of a technique within ladder() (its rung number). Used by the rater's
// histogram and by the "which rungs did this solve use" classification.
int ladder_rank(Technique t);

}  // namespace sudoku::model
