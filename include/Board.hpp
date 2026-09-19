#pragma once
#include "Theme.hpp"
#include "model/Grid.hpp"
#include "model/Technique.hpp"
#include "model/WorkGrid.hpp"

#include <gtkmm/drawingarea.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/printoperation.h>
#include <gtkmm/window.h>

#include <sigc++/signal.h>
#include <string>
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

    // Selection has two distinct verbs, because givens are not selectable and
    // the two paths need to answer that differently.
    //
    //   select()        — a TARGETED landing (click, a..i coordinate jump). The
    //                     user named one cell. If it's a given, the honest
    //                     answer is "nothing is selected", so it deselects.
    //   move_selection()— TRAVEL (arrow keys). The user named a direction, not
    //                     a cell, so givens are skipped over rather than landed
    //                     on. Deselecting here would strand the keyboard player
    //                     the moment a clue sat next to them.
    void move_selection(int dr, int dc);
    void select(int r, int c);
    void deselect();

    bool has_selection() const { return m_sel_r >= 0 && m_sel_c >= 0; }
    int  digit_count(int d) const;

    // How many of digit d are placed CORRECTLY -- givens plus user entries that
    // agree with the solution. digit_count() counts occurrences; this counts
    // the ones that are actually right. The difference is what "spent" has to
    // mean: nine 5s on the board with one in the wrong cell is not nine 5s
    // solved, and treating it as such greys out the button the player needs in
    // order to place the real one.
    int  digit_correct_count(int d) const;

    // A digit is spent when all nine of its cells are correctly filled. This
    // one predicate drives three things -- the pad button's colour, its
    // sensitivity, and the refusal in input_digit() -- so they cannot drift.
    //
    // It does consult the solution, which is the oracle tradeoff the note
    // pruning already makes. A convenience gate is the mild end of it: the most
    // it can do is withhold a digit, and clearing a cell always gives it back.
    // It never places anything for the player.
    bool digit_complete(int d) const { return digit_correct_count(d) >= model::N; }
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

    // ── Rendering, factored out so the printer can reuse it ──────────────────
    //
    // `on_draw` used to BE the drawing routine. It is now a caller: it fills in
    // the on-screen options and hands off to render(). Printing fills in a
    // different set (light theme, every screen affordance off) and calls the
    // same code, so the page can never drift from the board.
    //
    // Every layer that is a screen affordance rather than puzzle content is a
    // flag here, because paper wants none of them: a selection wash is a caret,
    // a conflict wash is live feedback, and printing the mistake red would hand
    // the solver a partial answer key.
    struct RenderOpts {
        const Theme* theme      = nullptr;  // required; null falls back to the board's
        bool paint_background   = true;     // false on paper -- the page is already white
        bool labels             = true;     // the a..i gutter coordinates
        bool selection          = true;
        bool notes_mode         = false;    // selection wash picks the Notes token
        bool teaching           = false;
        bool conflicts          = true;
        bool mistakes           = true;
        bool notes              = true;     // the player's pencil marks
    };

    // Where render() actually put the grid. on_draw caches this for hit-testing;
    // print discards it. Returned rather than stored so that printing a page
    // cannot overwrite the geometry the mouse depends on.
    struct Geometry { double ox = 0, oy = 0, cell = 0; };

    Geometry render(const Cairo::RefPtr<Cairo::Context>& cr,
                    int width, int height, const RenderOpts& o) const;

    // The mini solution grid for the printed page -- drawn small, digits only,
    // so the solver can cover it and check afterwards. Separate from render()
    // because it is a different object: a key, not a board.
    void draw_solution_key(const Cairo::RefPtr<Cairo::Context>& cr,
                           double x, double y, double size, const Theme& t) const;

    // False when Solver failed on set_puzzle (shouldn't happen for a generated
    // puzzle). The printer omits the key rather than printing a grid of blanks.
    bool has_solution() const;

    // Difficulty is the window's to know, not the board's, so the caller passes
    // the caption text in. The board supplies the ink.
    void print(Gtk::Window& parent, const std::string& subtitle);

private:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);
    bool on_key(guint keyval, guint keycode, Gdk::ModifierType state);
    void on_click(int n_press, double x, double y);

    bool cell_at(double x, double y, int& r, int& c) const;
    void select_first_open();   // land on the first editable cell, or deselect
    void changed();

    model::Grid m_grid;
    model::Grid m_solution;   // the unique solution of the current puzzle (for wrong-answer check)

    // Outlives print(): the preview path renders asynchronously after run()
    // returns, so the operation cannot be a local. Cleared on signal_done.
    Glib::RefPtr<Gtk::PrintOperation> m_print_op;
    Theme m_theme       = Theme::default_dark();
    Mode  m_mode        = Mode::Guess;
    // No selection until a puzzle is set. (0,0) was the old default and it is
    // a given about as often as any other cell — so the board could boot with
    // an unselectable cell washed.
    int   m_sel_r       = -1;
    int   m_sel_c       = -1;
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
