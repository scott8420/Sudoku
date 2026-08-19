#pragma once
// Shortcut registry: the single source of truth for every keyboard/mouse
// shortcut in Sudoku. GTK-free (pure data + string helpers), so it is
// sandbox-testable and pulled into both the wiring (Application drives
// set_accels_for_action from it) and the documentation (ShortcutsDialog).
// Register a shortcut once here; its accelerator and its dialog row both come
// from this list, so the two can never drift apart, and a pure test guards
// against two chords colliding.
//
// Adapted from Folio's s98 Shortcuts registry. Folio needed a tab axis
// (keyboard vs mouse) and a context axis (main window vs a separate focus
// window); Sudoku has one window and a thin mouse story, so both axes collapse
// away — a "Mouse" section heading in the single list is enough.
#include <string>
#include <vector>

namespace sudoku {

struct ShortcutSpec {
    std::string              section;      // group heading, e.g. "Game" (authored A-Z)
    std::string              action;       // "win.new-game" -- empty => doc-only (not a GAction)
    std::vector<std::string> accels;       // GTK accel form(s), e.g. {"<Ctrl>n"}; wired iff action set
    std::string              keys;         // explicit display; empty => derived from accels
    std::string              description;

    // Human-readable key string for the dialog: `keys` if set, else the accels
    // formatted and joined with " / ".
    std::string display_keys() const;
};

// One GTK accelerator string -> human display. "<Ctrl><Shift>n" -> "Ctrl+Shift+N".
// Modifiers emitted in canonical order (Ctrl, Alt, Shift, Super); named keys
// (comma, question, Left, space, ...) map to their glyphs where sensible.
std::string format_accel(const std::string& accel);

// The registry -- authored in section (A-Z) -> row order so a consumer can walk
// it linearly and start a new heading whenever the section changes.
const std::vector<ShortcutSpec>& shortcut_registry();

// Collision detector. An accel collides when it is claimed by two or more specs
// and at least one of them is a wired GAction (a GAction accelerator fires
// regardless of focus, so any other behaviour on the same chord shadows it or is
// shadowed). Returns the offending accel strings (sorted, unique); empty => clean.
std::vector<std::string> find_accel_collisions();

}  // namespace sudoku
