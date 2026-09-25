#pragma once
#include "Board.hpp"
#include "PreferencesDialog.hpp"
#include "ShortcutsDialog.hpp"
#include "Stats.hpp"
#include "StatsPanel.hpp"
#include "StrategyPanel.hpp"
#include "Theme.hpp"
#include "model/Generator.hpp"

#include <giomm/simpleaction.h>
#include <gtkmm/aboutdialog.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/dropdown.h>
#include <gtkmm/label.h>
#include <gtkmm/revealer.h>
#include <gtkmm/stringlist.h>
#include <gtkmm/togglebutton.h>

#include <array>
#include <memory>
#include <sigc++/connection.h>
#include <vector>

namespace Gtk { class Box; }

namespace sudoku {

class Application;

// MainWindow — the app shell. Header bar (New Game, difficulty, hamburger menu),
// the Board, and a bottom control bar (Guess/Notes toggle + 1-9 pad). Two edge
// drawers overlay the play area: the left teaches (StrategyPanel), the right
// reports (StatsPanel) — twins on the proven overlay pattern, mirrored. The game
// clock lives in the right drawer (idle until first play, pausable, auto-paused
// when the window is inactive). On solve, the timer freezes, the solve is
// recorded to Stats, and a success dialog shows the time. About and Preferences
// are hide-on-close singletons owned here.
class MainWindow : public Gtk::ApplicationWindow {
public:
    explicit MainWindow(Application& app);
    ~MainWindow() override;

private:
    void on_new_game();
    void on_clear_board();   // confirm, then Board::restart (same puzzle, clock keeps running)
    void refresh_controls();

    // Drawers. Only one opens at a time; the shared scrim closes whichever is
    // open. open_right refreshes the stats view before revealing.
    void open_left();
    void open_right();
    void close_drawers();
    void toggle_left();
    void toggle_right();

    // Timer — a small state machine, shown in the stats drawer (not the header).
    // Idle until the first interaction, manually pausable from the stats panel,
    // auto-suspended while the window is inactive, and frozen on solve.
    void timer_reset();       // new game: elapsed 0, Idle
    void note_activity();     // first click/key/entry -> Running
    void toggle_pause();      // stats Pause button: Running <-> Paused
    bool on_timer_tick();     // per-second; counts only when Running AND active
    void push_timer_view();   // reflect elapsed + pause state into the stats panel

    // Modal payoff on solve: difficulty + time + a New Game button.
    void show_success(model::Difficulty d, int seconds);

    void on_about();
    void on_preferences();
    void on_shortcuts();   // lazy-build + show the keyboard reference window
    void on_print();       // hand the board to Gtk::PrintOperation (PDF via the dialog)
    model::Difficulty selected_difficulty() const;

    Board         m_board;
    StrategyPanel m_strategy_panel;   // left drawer content (teaches)
    StatsPanel    m_stats_panel;      // right drawer content (reports)
    Gtk::Revealer m_revealer;         // left drawer
    Gtk::Revealer m_right_revealer;   // right drawer
    Gtk::Box*     m_scrim = nullptr;  // shared click-away catcher, shown only while a drawer is open

    Gtk::AboutDialog  m_about;
    PreferencesDialog m_prefs;
    std::unique_ptr<ShortcutsDialog> m_shortcuts;   // built on first open, reused (hide-on-close)
    std::vector<Theme> m_skins;   // builtin + user-discovered, index-aligned with the picker

    Stats m_stats;   // the persistent solve record (loads at construction)

    enum class TimerState { Idle, Running, Paused, Stopped };
    TimerState        m_timer_state = TimerState::Idle;
    bool              m_active      = true;   // window is the active toplevel
    sigc::connection  m_timer_conn;           // the per-second timeout (persistent)
    int               m_elapsed_secs = 0;
    bool              m_was_solved   = false;   // latch: record/stop/dialog fire once on the rising edge
    model::Difficulty m_current_difficulty = model::Difficulty::Medium;  // band of the puzzle in play

    Glib::RefPtr<Gtk::StringList> m_difficulty_model;
    Gtk::DropDown*     m_difficulty = nullptr;
    Gtk::ToggleButton* m_notes_mode = nullptr;
    Glib::RefPtr<Gio::SimpleAction> m_clear_action;   // enabled only while there is progress to lose
    std::array<Gtk::Button*, 9> m_numpad{};
    model::Generator m_generator;
};

}  // namespace sudoku
