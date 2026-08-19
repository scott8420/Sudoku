#include "MainWindow.hpp"
#include "Application.hpp"
#include "Css.hpp"
#include "Log.hpp"
#include "Skins.hpp"
#include "Teach.hpp"
#include "WidgetRegistry.hpp"
#include "widgets/Widgets.hpp"

#include <gdkmm/display.h>
#include <giomm/asyncresult.h>
#include <giomm/menu.h>
#include <glibmm/main.h>
#include <gtkmm/alertdialog.h>
#include <gtkmm/box.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/headerbar.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/overlay.h>
#include <gtkmm/stylecontext.h>

#include <cctype>
#include <string>
#include <vector>

namespace sudoku {

MainWindow::MainWindow(Application& /*app*/) {
    set_name("main_window");
    registry::add("main_window", this);

    // Install the app stylesheet once (the drawer tab's invisible-until-hover
    // behaviour lives here, in CSS, not in C++). Application priority so it
    // overrides theme defaults for our named classes.
    {
        auto provider = Gtk::CssProvider::create();
        provider->load_from_data(std::string(SUDOKU_CSS));
        Gtk::StyleContext::add_provider_for_display(
            Gdk::Display::get_default(), provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    set_title("Sudoku");
    set_default_size(860, 720);

    // ── Header bar ────────────────────────────────────────────────────────────
    auto* header = Gtk::make_managed<Gtk::HeaderBar>();
    set_titlebar(*header);

    auto* new_btn = Gtk::make_managed<widgets::Button>("hb_new_game", "New Game");
    new_btn->set_action_name("win.new-game");
    header->pack_start(*new_btn);

    m_difficulty_model = Gtk::StringList::create(
        std::vector<Glib::ustring>{"Easy", "Medium", "Hard", "Expert"});
    auto* dd = Gtk::make_managed<widgets::DropDown>("hb_difficulty", m_difficulty_model);
    dd->set_selected(1);
    m_difficulty = dd;
    header->pack_end(*dd);

    // Hamburger menu → Preferences / About / Quit.
    auto menu = Gio::Menu::create();
    menu->append("Preferences", "win.preferences");
    menu->append("Keyboard Shortcuts", "win.shortcuts");
    menu->append("About Sudoku", "win.about");
    menu->append("Quit", "win.quit");
    auto* hamburger = Gtk::make_managed<Gtk::MenuButton>();
    hamburger->set_icon_name("open-menu-symbolic");
    hamburger->set_tooltip_text("Menu");
    hamburger->set_menu_model(menu);
    header->pack_end(*hamburger);

    // ── Actions ───────────────────────────────────────────────────────────────
    add_action("new-game", sigc::mem_fun(*this, &MainWindow::on_new_game));
    add_action("about", sigc::mem_fun(*this, &MainWindow::on_about));
    add_action("preferences", sigc::mem_fun(*this, &MainWindow::on_preferences));
    add_action("shortcuts", sigc::mem_fun(*this, &MainWindow::on_shortcuts));
    add_action("toggle-notes", [this]() {
        m_board.set_show_notes(!m_board.show_notes());   // non-destructive view toggle
        m_board.grab_focus();
    });
    add_action("quit", [this]() { close(); });

    // About + Preferences are hide-on-close singletons owned by the window.
    m_about.set_program_name("Sudoku");
    m_about.set_version("0.1.0");
    // Logo from the compiled-in gresource icon theme (registered in
    // Application::on_activate). The SVG is a clean viewBox-only symbolic icon
    // (Folio's shape) so the theme scales it to the logo area and recolours it.
    m_about.set_logo_icon_name("io.github.example.Sudoku-symbolic");
    m_about.set_comments("A modern Sudoku for GNOME with a built-in strategy teacher.");
    m_about.set_license_type(Gtk::License::GPL_3_0);
    m_about.set_hide_on_close(true);

    m_prefs.signal_show_conflicts().connect(
        [this](bool on) { m_board.set_show_conflicts(on); });
    m_prefs.signal_highlight_mistakes().connect(
        [this](bool on) { m_board.set_highlight_mistakes(on); });

    // Skins: seed the sample template, load builtin + user skins, populate the
    // picker, and apply the first (the default dark). The board draws from theme
    // tokens, so applying a skin is just set_theme.
    skins::ensure_sample();
    m_skins = skins::all();
    {
        std::vector<Glib::ustring> names;
        for (const Theme& t : m_skins) names.push_back(t.name);
        m_prefs.set_skins(names, 0);
    }
    if (!m_skins.empty()) m_board.set_theme(m_skins.front());
    m_prefs.signal_skin().connect([this](int i) {
        if (i >= 0 && i < int(m_skins.size())) m_board.set_theme(m_skins[i]);
    });

    // ── Strategy panel signals (revealer/overlay assembled below) ─────────────
    m_strategy_panel.signal_technique().connect([this](model::Technique t) {
        int n = m_board.show_technique(t);
        m_strategy_panel.set_explanation(
            display_name(t) + " — " + std::to_string(n) +
            (n == 1 ? " place found\n\n" : " places found\n\n") + explain(t));
    });
    m_strategy_panel.signal_cleared().connect([this]() { m_board.clear_highlights(); });

    // ── Stats panel: guarded reset ────────────────────────────────────────────
    // The panel is a pure view; it emits when Reset is pressed. The confirm needs
    // a window in scope, so it lives here. On accept: clear the store, re-feed.
    m_stats_panel.signal_reset_requested().connect([this]() {
        auto dlg = Gtk::AlertDialog::create();
        dlg->set_modal(true);
        dlg->set_message("Reset statistics?");
        dlg->set_detail("This clears every recorded game, best, and average time. "
                        "It cannot be undone.");
        dlg->set_buttons(std::vector<Glib::ustring>{"Cancel", "Reset"});
        dlg->set_default_button(0);
        dlg->set_cancel_button(0);
        dlg->choose(*this, [this, dlg](const Glib::RefPtr<Gio::AsyncResult>& res) {
            int idx = 0;
            try { idx = dlg->choose_finish(res); } catch (const Glib::Error&) { return; }
            if (idx == 1) { m_stats.reset(); m_stats_panel.set_stats(m_stats); }
        });
    });
    m_stats_panel.set_stats(m_stats);   // initial render from the loaded store
    m_stats_panel.signal_pause_toggled().connect(
        sigc::mem_fun(*this, &MainWindow::toggle_pause));

    // ── Content: [drawer | (board over control bar)] ──────────────────────────
    auto* bar = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    bar->set_spacing(6);
    bar->set_margin(10);
    bar->set_halign(Gtk::Align::CENTER);

    auto* notes = Gtk::make_managed<widgets::ToggleButton>("mode_notes", "Notes");
    notes->signal_toggled().connect([this, notes]() {
        m_board.set_mode(notes->get_active() ? Board::Mode::Notes : Board::Mode::Guess);
        m_board.grab_focus();
    });
    m_notes_mode = notes;
    bar->append(*notes);

    // The n/Space key on the board flips the Notes button; the button's own
    // toggled handler then sets the board mode, so the button stays the truth.
    m_board.signal_notes_toggle().connect([this]() {
        if (m_notes_mode) m_notes_mode->set_active(!m_notes_mode->get_active());
    });

    // Any direct board interaction (click or key) starts the clock on the first
    // touch — the timer does not run before the player engages the puzzle.
    m_board.signal_activity().connect(sigc::mem_fun(*this, &MainWindow::note_activity));

    // One-shot: fill every empty cell's notes with its valid candidates.
    auto* fill = Gtk::make_managed<widgets::Button>("fill_notes", "Fill");
    fill->set_tooltip_text("Fill in all valid candidates as notes");
    fill->signal_clicked().connect([this]() { note_activity(); m_board.fill_candidates(); m_board.grab_focus(); });
    bar->append(*fill);

    for (int d = 1; d <= 9; ++d) {
        auto* b = Gtk::make_managed<widgets::Button>("np_" + std::to_string(d),
                                                     std::to_string(d));
        b->signal_clicked().connect([this, d]() { note_activity(); m_board.input_digit(d); m_board.grab_focus(); });
        m_numpad[d - 1] = b;
        bar->append(*b);
    }

    auto* play = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    play->append(m_board);
    m_board.set_expand(true);
    play->append(*bar);
    play->set_hexpand(true);

    // ── Drawers as overlays (Folio's proven pattern) ──────────────────────────
    // The play area is the overlay base; the drawer bits float on top so opening
    // never reshuffles the board. Two twins: the left teaches (Strategies), the
    // right reports (Stats). Stacking order (bottom→top): scrim, tabs, revealers
    // — an open drawer covers its tab, and the shared scrim catches clicks
    // outside to close whichever is open.
    m_revealer.set_child(m_strategy_panel);
    m_revealer.set_transition_type(Gtk::RevealerTransitionType::SLIDE_RIGHT);
    m_revealer.set_transition_duration(240);
    m_revealer.set_halign(Gtk::Align::START);
    m_revealer.set_valign(Gtk::Align::FILL);
    m_revealer.set_reveal_child(false);

    m_right_revealer.set_child(m_stats_panel);
    m_right_revealer.set_transition_type(Gtk::RevealerTransitionType::SLIDE_LEFT);
    m_right_revealer.set_transition_duration(240);
    m_right_revealer.set_halign(Gtk::Align::END);
    m_right_revealer.set_valign(Gtk::Align::FILL);
    m_right_revealer.set_reveal_child(false);

    auto* scrim = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    scrim->set_name("drawer-scrim");
    scrim->set_hexpand(true);
    scrim->set_vexpand(true);
    scrim->set_visible(false);
    {
        auto click = Gtk::GestureClick::create();
        click->signal_released().connect([this](int, double, double) { close_drawers(); });
        scrim->add_controller(click);
    }
    m_scrim = scrim;

    // Each tab is a plain button; its "invisible until hover" look is pure CSS
    // (.drawer-tab / .drawer-tab-right, mirrored). Click toggles its drawer.
    auto* tab = Gtk::make_managed<widgets::Button>("drawer_tab", "\u203A");  // ›
    tab->add_css_class("drawer-tab");
    tab->set_halign(Gtk::Align::START);
    tab->set_valign(Gtk::Align::CENTER);
    tab->set_tooltip_text("Strategies");
    tab->signal_clicked().connect([this]() { toggle_left(); });

    auto* right_tab = Gtk::make_managed<widgets::Button>("stats_tab", "\u2039");  // ‹
    right_tab->add_css_class("drawer-tab-right");
    right_tab->set_halign(Gtk::Align::END);
    right_tab->set_valign(Gtk::Align::CENTER);
    right_tab->set_tooltip_text("Stats");
    right_tab->signal_clicked().connect([this]() { toggle_right(); });

    auto* overlay = Gtk::make_managed<Gtk::Overlay>();
    overlay->set_child(*play);
    overlay->add_overlay(*scrim);
    overlay->add_overlay(*tab);
    overlay->add_overlay(*right_tab);
    overlay->add_overlay(m_revealer);
    overlay->add_overlay(m_right_revealer);
    set_child(*overlay);

    m_board.signal_changed().connect(sigc::mem_fun(*this, &MainWindow::refresh_controls));

    // Grab keyboard focus once the window is mapped (grabbing in the ctor, before
    // realize, would no-op).
    signal_map().connect([this]() { m_board.grab_focus(); });

    // Pause the clock whenever the window isn't the active toplevel (walked away,
    // switched apps) and resume when it comes back -- the tick simply doesn't
    // count while inactive; the Running/Paused state is untouched.
    property_is_active().signal_changed().connect([this]() {
        m_active = property_is_active().get_value();
    });

    // One persistent per-second tick for the app's life; on_timer_tick decides
    // whether this second actually counts (Running AND active).
    m_timer_conn = Glib::signal_timeout().connect_seconds(
        sigc::mem_fun(*this, &MainWindow::on_timer_tick), 1);

    on_new_game();
}

MainWindow::~MainWindow() { m_timer_conn.disconnect(); registry::remove(this); }

void MainWindow::open_left() {
    m_right_revealer.set_reveal_child(false);   // only one open at a time
    if (m_scrim) m_scrim->set_visible(true);    // catch click-away
    m_revealer.set_reveal_child(true);
}

void MainWindow::open_right() {
    m_revealer.set_reveal_child(false);
    m_stats_panel.set_stats(m_stats);           // refresh the view on open
    push_timer_view();                          // and the clock + pause state
    if (m_scrim) m_scrim->set_visible(true);
    m_right_revealer.set_reveal_child(true);
}

void MainWindow::close_drawers() {
    m_revealer.set_reveal_child(false);
    m_right_revealer.set_reveal_child(false);
    if (m_scrim) m_scrim->set_visible(false);
    m_board.grab_focus();
}

void MainWindow::toggle_left() {
    if (m_revealer.get_reveal_child()) close_drawers();
    else open_left();
}

void MainWindow::toggle_right() {
    if (m_right_revealer.get_reveal_child()) close_drawers();
    else open_right();
}

void MainWindow::timer_reset() {
    m_elapsed_secs = 0;
    m_timer_state  = TimerState::Idle;   // waits for the first activity to start
    push_timer_view();
}

void MainWindow::note_activity() {
    if (m_timer_state == TimerState::Idle) {   // first touch starts the clock
        m_timer_state = TimerState::Running;
        push_timer_view();
    }
}

void MainWindow::toggle_pause() {
    if (m_timer_state == TimerState::Running)     m_timer_state = TimerState::Paused;
    else if (m_timer_state == TimerState::Paused) m_timer_state = TimerState::Running;
    else return;   // Idle / Stopped: nothing to pause
    push_timer_view();
}

bool MainWindow::on_timer_tick() {
    // The tick is persistent; this second only counts while actively playing and
    // the window is the active toplevel.
    if (m_timer_state == TimerState::Running && m_active) {
        ++m_elapsed_secs;
        m_stats_panel.set_time(format_duration(m_elapsed_secs));
    }
    return true;
}

void MainWindow::push_timer_view() {
    m_stats_panel.set_time(format_duration(m_elapsed_secs));
    m_stats_panel.set_paused(m_timer_state == TimerState::Paused);
    m_stats_panel.set_pause_enabled(m_timer_state == TimerState::Running ||
                                    m_timer_state == TimerState::Paused);
}

void MainWindow::show_success(model::Difficulty d, int seconds) {
    // Capitalise the band name for display (name() is lowercase for file keys).
    std::string band = model::name(d);
    if (!band.empty()) band[0] = char(std::toupper((unsigned char)band[0]));

    auto dlg = Gtk::AlertDialog::create();
    dlg->set_modal(true);
    dlg->set_message("Solved!");
    dlg->set_detail("Difficulty: " + band + "\nTime: " + format_duration(seconds));
    dlg->set_buttons(std::vector<Glib::ustring>{"Close", "New Game"});
    dlg->set_default_button(1);
    dlg->set_cancel_button(0);
    dlg->choose(*this, [this, dlg](const Glib::RefPtr<Gio::AsyncResult>& res) {
        int idx = 0;
        try { idx = dlg->choose_finish(res); } catch (const Glib::Error&) { return; }
        if (idx == 1) on_new_game();
    });
}

void MainWindow::on_about() {
    m_about.set_transient_for(*this);
    m_about.present();
}

void MainWindow::on_preferences() {
    m_prefs.set_transient_for(*this);
    m_prefs.set_show_conflicts(m_board.show_conflicts());       // sync before showing
    m_prefs.set_highlight_mistakes(m_board.highlight_mistakes());
    m_prefs.present();
}

void MainWindow::on_shortcuts() {
    if (!m_shortcuts) m_shortcuts = std::make_unique<ShortcutsDialog>();
    m_shortcuts->show(*this);
}

model::Difficulty MainWindow::selected_difficulty() const {
    switch (m_difficulty ? m_difficulty->get_selected() : 1u) {
        case 0:  return model::Difficulty::Easy;
        case 1:  return model::Difficulty::Medium;
        case 2:  return model::Difficulty::Hard;
        default: return model::Difficulty::Expert;
    }
}

void MainWindow::on_new_game() {
    model::Difficulty requested = selected_difficulty();
    model::Difficulty actual    = requested;
    model::Grid puzzle = m_generator.generate(requested, /*max_attempts=*/300, &actual);

    if (auto lg = log::get(log::Area::Model))
        lg->info("new game: requested {}, got {}", model::name(requested), model::name(actual));

    m_current_difficulty = actual;   // record against the band actually generated
    m_was_solved = false;            // arm the solve latch for the new puzzle

    set_title("Sudoku");
    m_board.set_puzzle(puzzle);   // clears any teaching overlay + refreshes controls
    m_strategy_panel.reset();
    timer_reset();                // clock at 0, Idle -- starts on the first interaction
    m_board.grab_focus();
}

void MainWindow::refresh_controls() {
    const bool solved = m_board.solved();
    for (int d = 1; d <= 9; ++d)
        if (m_numpad[d - 1])
            m_numpad[d - 1]->set_sensitive(!solved && !m_board.digit_complete(d));

    if (solved && !m_was_solved) {
        // Rising edge: stop the clock, bank the result, celebrate — exactly once.
        // (signal_changed fires on every mutation, so the latch is what keeps the
        // dialog from re-opening and the stats from double-counting.)
        m_was_solved = true;
        m_timer_state = TimerState::Stopped;   // freeze the clock on solve
        push_timer_view();
        set_title("Sudoku — Solved!");
        if (auto lg = log::get(log::Area::Model)) lg->info("puzzle solved");
        m_stats.record_solve(m_current_difficulty, m_elapsed_secs);
        m_stats_panel.set_stats(m_stats);
        show_success(m_current_difficulty, m_elapsed_secs);
    }
}

}  // namespace sudoku
