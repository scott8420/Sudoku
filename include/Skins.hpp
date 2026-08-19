#pragma once
#include "Theme.hpp"

#include <vector>

namespace sudoku::skins {

// The skin system (UI-side). A skin is a Theme; skins come from two places:
// built-in presets compiled into the app, and user JSON files discovered in the
// data dir. all() returns the combined list for the picker.
//
// Robustness follows CANON's round-trip rule: a missing or malformed token in a
// user skin falls back to default_dark()'s value rather than failing, and a
// file that won't parse at all is skipped (logged), never fatal.

std::vector<Theme> builtin();      // presets compiled in (always available)
std::vector<Theme> user();         // parsed from <data>/sudoku/skins/*.json
std::vector<Theme> all();          // builtin() then user()

// Ensure the skins dir exists and, if empty, drop one commented template so a
// user has a working example to copy. Called once at startup.
void ensure_sample();

}  // namespace sudoku::skins
