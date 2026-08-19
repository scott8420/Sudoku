#pragma once
#include "Theme.hpp"
#include "model/Grid.hpp"
#include "model/Technique.hpp"
#include "model/WorkGrid.hpp"

#include <gtkmm/drawingarea.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/eventcontrollerkey.h>

#include <sigc++/signal.h>
#include <vector>

namespace sudoku {

// Board — the interactive Cairo rendering of a puzzle, plus the teaching
// overlay.
//
// The model is the truth; the Board renders it and turns input into model
// mutations (grid.place / toggle_note), never the reverse. The teaching overlay
// is the visible face of the model's technique engine: show_technique runs a
// rung's find_all on the current board and paints the result — the "possible
// numbers", the premise cells, the eliminations, the placement. The prose that
// goes with it lives in Teach.hpp (UI side); the Board only paints structure.
class Board : public Gtk::DrawingArea {
public:
    enum class Mode { Guess, Notes };

    Board();
    ~Board() override;

    void set_puzzle(const model::Grid& g);
    const model::Grid& grid() const { return m_grid; }

    // Apply a skin. The board draws entirely from theme tokens, so re-theming
    // is just swapping the struct and repainting.
    void set_theme(const Theme& t) { m_theme = t; queue_draw(); }
    const Theme& theme() const { return m_theme; }

    // Mode is now VISIBLE on the board (the selection wash switches to the
    // Notes token), so setting it has to repaint. Without the queue_draw the
    // wash would only change on the next unrelated redraw — the feature would
    // look broken rather than absent.
    void set_mode(Mode m) { m_mode = m; queue_draw(); }
    Mode mode() const { return m_mode; }

    void input_digit(int d);
    void clear_selected();
    void move_selection(int dr, int dc);
    void select(int r, int c);

    bool has_selection() const { return m_sel_r >= 0 && m_sel_c >= 0; }
    int  digit_count(int d) const;
    bool digit_complete(int d) const { return digit_count(d) >= model::N; }
    bool solved() const { return m_grid.solved(); }

    // Preference: whether conflicting duplicates get a wash. On by default
    // (permissive help); a Preferences switch flips it.
    void set_show_conflicts(bool b) { m_show_conflicts = b; queue_draw(); }
    bool show_conflicts() const { return m_show_conflicts; }

    // Preference: whether a user digit that disagrees with the solution renders
    // in red (strict feedback). On by default.
    void set_highlight_mistakes(bool b) { m_highlight_mistakes = b; queue_draw(); }
    bool highlight_mistakes() const { return m_highlight_mistakes; }

    // View toggle: whether the user's pencil marks are drawn. Non-destructive —
    // the notes stay in the model, they're just hidden. On by default.
    void set_show_notes(bool b) { m_show_notes = b; queue_draw(); }
    bool show_notes() const { return m_show_notes; }

    // Fill every empty non-given cell's notes with its currently-valid
    // candidates (one-shot). Auto-erase then keeps them pruned as you play.
    void fill_candidates();

    // Teaching overlay. show_technique paints where the technique applies on the
    // current board and returns how many applications were found. Any grid
    // mutation clears the overlay (the lesson would be stale).
    int  show_technique(model::Technique t);
    void clear_highlights();

    sigc::signal<void()>& signal_changed() { return m_signal_changed; }

    // Emitted when the user presses the notes/guess mode-toggle key (n / Space).
    // The Notes ToggleButton is the single source of truth, so MainWindow flips
    // the button in response rather than the Board setting its own mode here.
    sigc::signal<void()>& signal_notes_toggle() { return m_signal_notes_toggle; }

    // Emitted on any direct user interaction with the board (a click or a key).
    // MainWindow uses it to start the timer on first activity -- the clock does
    // not run until the player actually touches the puzzle.
    sigc::signal<void()>& signal_activity() { return m_signal_activity; }

private:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);
    bool on_key(guint keyval, guint keycode, Gdk::ModifierType state);
    void on_click(int n_press, double x, double y);

    bool cell_at(double x, double y, int& r, int& c) const;
    void changed();

    model::Grid m_grid;
    model::Grid m_solution;   // the unique solution of the current puzzle (for wrong-answer check)
    Theme m_theme       = Theme::default_dark();
    Mode  m_mode        = Mode::Guess;
    int   m_sel_r       = 0;
    int   m_sel_c       = 0;
    int   m_pending_col = -1;
    bool  m_show_conflicts    = true;
    bool  m_highlight_mistakes = true;
    bool  m_show_notes         = true;   // view toggle: draw the user's pencil marks

    // Teaching overlay state.
    bool                        m_teaching = false;
    std::vector<model::Finding> m_teach_findings;
    model::WorkGrid             m_teach_work;   // candidate snapshot for the overlay

    double m_ox = 0, m_oy = 0, m_cell = 0;      // cached geometry for hit-testing

    sigc::signal<void()> m_signal_changed;
    sigc::signal<void()> m_signal_notes_toggle;
    sigc::signal<void()> m_signal_activity;
};

}  // namespace sudoku
