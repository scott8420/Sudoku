#pragma once
#include "Stats.hpp"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>

#include <array>
#include <sigc++/signal.h>

namespace sudoku {

// StatsPanel — the right drawer's content: the game clock (with a Pause button)
// and a per-difficulty solve record, twin of the left drawer's StrategyPanel.
// Like StrategyPanel it is a PURE VIEW: it renders whatever it is handed and
// emits signals (reset, pause) for MainWindow to act on. It owns no Stats, holds
// no timer state, and performs no persistence — MainWindow drives it.
class StatsPanel : public Gtk::Box {
public:
    StatsPanel();

    // Emitted when the user presses Reset. MainWindow shows the confirmation and,
    // if accepted, clears the Stats and calls set_stats() again.
    sigc::signal<void()>& signal_reset_requested() { return m_sig_reset; }

    // Emitted when the user presses Pause/Resume. MainWindow flips the timer.
    sigc::signal<void()>& signal_pause_toggled() { return m_sig_pause; }

    // Re-render the four rows from the given record. Cheap; call on open and
    // after every solve or reset.
    void set_stats(const Stats& s);

    // Timer view: the formatted elapsed time, and the Pause button's label/enable
    // state. MainWindow owns the clock and pushes these.
    void set_time(const Glib::ustring& formatted);
    void set_paused(bool paused);              // toggles the button label Pause<->Resume
    void set_pause_enabled(bool enabled);      // greyed out when idle / solved

private:
    Gtk::Label*  m_time_value = nullptr;   // the mm:ss clock
    Gtk::Button* m_pause      = nullptr;    // Pause / Resume
    Gtk::Grid    m_grid;

    // Per-row value cells (indexed by model::rank): played / best / average.
    std::array<Gtk::Label*, 4> m_played{};
    std::array<Gtk::Label*, 4> m_best{};
    std::array<Gtk::Label*, 4> m_avg{};

    sigc::signal<void()> m_sig_reset;
    sigc::signal<void()> m_sig_pause;
};

}  // namespace sudoku
