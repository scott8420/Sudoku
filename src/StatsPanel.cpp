#include "StatsPanel.hpp"
#include "WidgetRegistry.hpp"
#include "widgets/Widgets.hpp"

namespace sudoku {
namespace {

// The four bands, in the same rank order the grid rows are laid out in.
constexpr std::array<model::Difficulty, 4> kBands = {
    model::Difficulty::Easy, model::Difficulty::Medium,
    model::Difficulty::Hard, model::Difficulty::Expert};

const char* kRowLabel[4] = {"Easy", "Medium", "Hard", "Expert"};

// A grid label with consistent alignment/margins. Values are right-aligned so
// the numbers form clean columns; the leading level name is left-aligned.
Gtk::Label* cell(const Glib::ustring& text, double xalign) {
    auto* l = Gtk::make_managed<Gtk::Label>(text);
    l->set_xalign(xalign);
    l->set_margin_start(6);
    l->set_margin_end(6);
    l->set_margin_top(3);
    l->set_margin_bottom(3);
    return l;
}

}  // namespace

StatsPanel::StatsPanel() : Gtk::Box(Gtk::Orientation::VERTICAL) {
    set_name("stats_panel");
    registry::add("stats_panel", this);

    set_size_request(250, -1);
    set_margin(10);
    set_spacing(8);

    auto* title = Gtk::make_managed<widgets::Label>("stats_title", "Stats");
    title->set_xalign(0.0);
    title->add_css_class("title-4");
    append(*title);

    // ── Timer row: the game clock lives here (not the header), with a Pause
    //    button so the player can walk away without inflating their time. ──────
    auto* time_row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    time_row->set_spacing(8);

    auto* time_caption = Gtk::make_managed<Gtk::Label>("Time");
    time_caption->add_css_class("dim-label");
    time_row->append(*time_caption);

    m_time_value = Gtk::make_managed<widgets::Label>("stats_time", format_duration(0));
    m_time_value->add_css_class("title-3");
    m_time_value->add_css_class("numeric");
    m_time_value->set_hexpand(true);
    m_time_value->set_xalign(0.0);
    time_row->append(*m_time_value);

    m_pause = Gtk::make_managed<widgets::Button>("stats_pause", "Pause");
    m_pause->set_tooltip_text("Pause the clock (auto-pauses when the window is inactive)");
    m_pause->signal_clicked().connect([this]() { m_sig_pause.emit(); });
    time_row->append(*m_pause);
    append(*time_row);

    // Header row + one row per band. Column 0 = level, 1 = played, 2 = best,
    // 3 = average. Header labels are dim; value cells are filled by set_stats().
    m_grid.set_column_spacing(10);
    m_grid.set_row_spacing(2);
    m_grid.set_hexpand(true);

    auto* h_level  = cell("Level",  0.0);
    auto* h_played = cell("Played", 1.0);
    auto* h_best   = cell("Best",   1.0);
    auto* h_avg    = cell("Avg",    1.0);
    for (Gtk::Label* h : {h_level, h_played, h_best, h_avg}) h->add_css_class("dim-label");
    m_grid.attach(*h_level,  0, 0);
    m_grid.attach(*h_played, 1, 0);
    m_grid.attach(*h_best,   2, 0);
    m_grid.attach(*h_avg,    3, 0);

    for (int i = 0; i < 4; ++i) {
        int row = i + 1;
        m_grid.attach(*cell(kRowLabel[i], 0.0), 0, row);
        m_played[i] = cell("0",  1.0);  m_grid.attach(*m_played[i], 1, row);
        m_best[i]   = cell("\u2014", 1.0);  m_grid.attach(*m_best[i],   2, row);  // em dash
        m_avg[i]    = cell("\u2014", 1.0);  m_grid.attach(*m_avg[i],    3, row);
    }
    append(m_grid);

    // Spacer pushes Reset to the bottom, mirroring StrategyPanel's expanding tail.
    auto* spacer = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    spacer->set_vexpand(true);
    append(*spacer);

    auto* reset = Gtk::make_managed<widgets::Button>("stats_reset", "Reset stats");
    reset->set_tooltip_text("Clear all recorded games (asks for confirmation)");
    reset->add_css_class("destructive-action");
    reset->signal_clicked().connect([this]() { m_sig_reset.emit(); });
    append(*reset);
}

void StatsPanel::set_stats(const Stats& s) {
    for (int i = 0; i < 4; ++i) {
        const Record& r = s.record(kBands[i]);
        m_played[i]->set_text(std::to_string(r.played));
        if (r.played > 0) {
            m_best[i]->set_text(format_duration(r.best));
            m_avg[i]->set_text(format_duration(r.avg()));
        } else {
            m_best[i]->set_text("\u2014");   // em dash — no games yet
            m_avg[i]->set_text("\u2014");
        }
    }
}

void StatsPanel::set_time(const Glib::ustring& formatted) {
    if (m_time_value) m_time_value->set_text(formatted);
}

void StatsPanel::set_paused(bool paused) {
    if (m_pause) m_pause->set_label(paused ? "Resume" : "Pause");
}

void StatsPanel::set_pause_enabled(bool enabled) {
    if (m_pause) m_pause->set_sensitive(enabled);
}

}  // namespace sudoku
