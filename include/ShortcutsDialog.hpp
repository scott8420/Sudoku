#pragma once
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/separator.h>
#include <gtkmm/window.h>

#include <string>

namespace sudoku {

// Keyboard & mouse reference window.
//
// Replaces the deprecated GtkShortcutsWindow (deprecated GTK 4.18) with a
// hand-rolled Gtk::Window -- the same non-deprecated, dependency-free pattern
// Folio settled on, minus its Notebook (Sudoku's shortcut set is small and its
// mouse story is one line, so a single scrolled Grid with a "Mouse" section
// heading is enough).
//
// The content is NOT hand-listed here: it is rendered by walking the pure
// shortcut_registry() (Shortcuts.hpp), which is also what the app wires the
// accelerators from -- one source of truth, so the dialog can never drift from
// the live accel table. Non-modal + hide_on_close, so MainWindow builds it once
// and reuses it.
class ShortcutsDialog : public Gtk::Window {
public:
    ShortcutsDialog();
    void show(Gtk::Window& parent);

private:
    // Grid helpers (each returns the next free row).
    int add_heading(Gtk::Grid& grid, const std::string& title, int row);
    int add_row(Gtk::Grid& grid, const std::string& keys, const std::string& desc, int row);
    int add_spacer(Gtk::Grid& grid, int row);

    Gtk::Box            m_root{Gtk::Orientation::VERTICAL};
    Gtk::ScrolledWindow m_scroll;
    Gtk::Grid           m_grid;
    Gtk::Button         m_btn_close{"Close"};
};

}  // namespace sudoku
