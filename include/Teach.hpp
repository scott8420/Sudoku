#pragma once
#include "model/Finding.hpp"

#include <glibmm/ustring.h>

namespace sudoku {

// UI-side prose for techniques — the presentation half of the model/UI seam.
//
// The model emits Findings keyed by Technique and knows nothing about words the
// user reads. These functions turn a technique id into a display name and a
// one-sentence explanation. Prose lives here, on the UI side of the line, never
// in sudoku::model — which is what keeps the engine liftable and lets a
// different front-end narrate the same Findings its own way.
Glib::ustring display_name(model::Technique t);
Glib::ustring explain(model::Technique t);

}  // namespace sudoku
