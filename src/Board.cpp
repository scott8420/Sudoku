#include "Board.hpp"
#include "Log.hpp"
#include "WidgetRegistry.hpp"
#include "model/Solver.hpp"

#include <gdk/gdkkeysyms.h>

#include <algorithm>
#include <sigc++/functors/mem_fun.h>
#include <string>

namespace sudoku {

Board::Board() {
    set_name("board");
    registry::add("board", this);

    set_draw_func(sigc::mem_fun(*this, &Board::on_draw));
    set_content_width(480);
    set_content_height(480);
    set_expand(true);
    set_focusable(true);

    auto key = Gtk::EventControllerKey::create();
    key->signal_key_pressed().connect(sigc::mem_fun(*this, &Board::on_key), false);
    add_controller(key);

    auto click = Gtk::GestureClick::create();
    click->signal_pressed().connect(sigc::mem_fun(*this, &Board::on_click));
    add_controller(click);
}

Board::~Board() { registry::remove(this); }

void Board::set_puzzle(const model::Grid& g) {
    m_grid = g;
    // Solve a copy once so wrong-answer checking has the unique solution to
    // compare against. The puzzle is generated unique, so this is deterministic.
    m_solution = g;
    model::Solver::solve(m_solution);
    m_pending_col = -1;
    changed();  // also clears any teaching overlay
}

void Board::fill_candidates() {
    for (int r = 0; r < model::N; ++r)
        for (int c = 0; c < model::N; ++c)
            if (m_grid.empty(r, c) && !m_grid.given(r, c))
                m_grid.set_notes(r, c, m_grid.candidates(r, c));
    changed();
}

void Board::changed() {
    // A grid mutation makes any teaching overlay stale — drop it so highlights
    // never lie about the current board.
    m_teaching = false;
    m_teach_findings.clear();
    m_signal_changed.emit();
    queue_draw();
}

void Board::select(int r, int c) {
    m_sel_r = std::clamp(r, 0, model::N - 1);
    m_sel_c = std::clamp(c, 0, model::N - 1);
    queue_draw();
}

void Board::move_selection(int dr, int dc) { select(m_sel_r + dr, m_sel_c + dc); }

void Board::input_digit(int d) {
    if (!has_selection() || m_grid.given(m_sel_r, m_sel_c)) return;
    if (m_mode == Mode::Guess) {
        if (!m_grid.place(m_sel_r, m_sel_c, d)) return;

        // Auto-erase, gated on the digit being RIGHT. A correct placement is
        // genuinely spoken-for in its row/column/box, so pruning it from the
        // peers' pencil marks is help. A wrong one proves nothing, and pruning
        // on it destroys the player's own reasoning — reasoning that clearing
        // the bad guess cannot bring back. So the model no longer prunes on its
        // own; the Board, which holds the solution, asks for it only when the
        // placement is confirmed. Its own marks are kept either way and surface
        // again if the value is cleared.
        //
        // If the solution is unknown (Solver failed on set_puzzle — shouldn't
        // happen for a generated puzzle), fall back to pruning unconditionally:
        // the s7 behaviour, which is the safer default when we can't judge.
        const int truth = m_solution.value(m_sel_r, m_sel_c);
        if (truth == 0 || truth == d) {
            m_grid.clear_notes(m_sel_r, m_sel_c);
            m_grid.erase_peer_notes(m_sel_r, m_sel_c, d);
        }
    } else {
        if (m_grid.empty(m_sel_r, m_sel_c)) m_grid.toggle_note(m_sel_r, m_sel_c, d);
        else return;
    }
    changed();
}

void Board::clear_selected() {
    if (!has_selection() || m_grid.given(m_sel_r, m_sel_c)) return;
    // Two-stage erase, now that notes survive a placement. First press takes the
    // value off and the retained pencil marks reappear; a second press on the
    // now-empty cell clears those. Wiping both at once would re-open the same
    // hole from the other side — a bad guess plus a Delete would still cost the
    // player their notes.
    if (!m_grid.empty(m_sel_r, m_sel_c)) m_grid.clear(m_sel_r, m_sel_c);
    else                                 m_grid.clear_notes(m_sel_r, m_sel_c);
    changed();
}

int Board::digit_count(int d) const {
    int n = 0;
    for (int r = 0; r < model::N; ++r)
        for (int c = 0; c < model::N; ++c)
            if (m_grid.value(r, c) == d) ++n;
    return n;
}

int Board::show_technique(model::Technique t) {
    m_teach_work = model::WorkGrid::from(m_grid);
    m_teach_findings.clear();
    for (const auto& rung : model::ladder())
        if (rung.id == t) { m_teach_findings = rung.find_all(m_teach_work); break; }
    m_teaching = true;
    queue_draw();
    return int(m_teach_findings.size());
}

void Board::clear_highlights() {
    m_teaching = false;
    m_teach_findings.clear();
    queue_draw();
}

bool Board::cell_at(double x, double y, int& r, int& c) const {
    if (m_cell <= 0) return false;
    int cc = int((x - m_ox) / m_cell);
    int rr = int((y - m_oy) / m_cell);
    if (rr < 0 || rr >= model::N || cc < 0 || cc >= model::N) return false;
    r = rr; c = cc;
    return true;
}

void Board::on_click(int /*n_press*/, double x, double y) {
    m_signal_activity.emit();   // first click starts the clock
    grab_focus();
    int r, c;
    if (cell_at(x, y, r, c)) select(r, c);
}

bool Board::on_key(guint keyval, guint /*keycode*/, Gdk::ModifierType /*state*/) {
    m_signal_activity.emit();   // first keypress starts the clock
    if (keyval == GDK_KEY_comma) return true;  // accepted separator in "a,c"

    auto letter_index = [](guint kv) -> int {
        if (kv >= GDK_KEY_a && kv <= GDK_KEY_i) return int(kv - GDK_KEY_a);
        if (kv >= GDK_KEY_A && kv <= GDK_KEY_I) return int(kv - GDK_KEY_A);
        return -1;
    };
    int li = letter_index(keyval);
    if (li >= 0) {
        if (m_pending_col < 0) m_pending_col = li;      // first letter = column
        else { select(li, m_pending_col); m_pending_col = -1; }  // second = row
        return true;
    }

    m_pending_col = -1;  // any non-letter cancels a half-typed coordinate

    if (keyval == GDK_KEY_n || keyval == GDK_KEY_space) {
        m_signal_notes_toggle.emit();   // MainWindow flips the Notes button
        return true;
    }

    if (keyval >= GDK_KEY_1 && keyval <= GDK_KEY_9) {
        input_digit(int(keyval - GDK_KEY_0));
        return true;
    }
    switch (keyval) {
        case GDK_KEY_0:
        case GDK_KEY_Delete:
        case GDK_KEY_BackSpace: clear_selected();      return true;
        case GDK_KEY_Left:      move_selection(0, -1); return true;
        case GDK_KEY_Right:     move_selection(0, +1); return true;
        case GDK_KEY_Up:        move_selection(-1, 0); return true;
        case GDK_KEY_Down:      move_selection(+1, 0); return true;
        default:                return false;
    }
}

void Board::on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height) {
    using namespace sudoku::model;
    auto use = [&](const Rgba& c) { cr->set_source_rgba(c.r, c.g, c.b, c.a); };

    const double sq     = std::min(width, height);
    const double gutter = sq * 0.055;
    const double grid   = sq - gutter;
    const double cell   = grid / N;
    const double ox     = (width - sq) / 2.0 + gutter;
    const double oy     = (height - sq) / 2.0 + gutter;
    m_ox = ox; m_oy = oy; m_cell = cell;

    auto centre_text = [&](const std::string& s, double cx, double cy) {
        Cairo::TextExtents e;
        cr->get_text_extents(s, e);
        cr->move_to(cx - e.width / 2.0 - e.x_bearing, cy - e.height / 2.0 - e.y_bearing);
        cr->show_text(s);
    };
    auto cell_rect = [&](int r, int c) { cr->rectangle(ox + c * cell, oy + r * cell, cell, cell); };

    // Teaching lookups over the current findings.
    auto is_eliminated = [&](int r, int c, int d) {
        for (const Finding& f : m_teach_findings)
            for (const CandidateRef& e : f.eliminations)
                if (e.row == r && e.col == c && e.digit == d) return true;
        return false;
    };
    auto placement_at = [&](int r, int c) -> int {
        for (const Finding& f : m_teach_findings)
            if (f.placement && f.placement->row == r && f.placement->col == c)
                return f.placement->digit;
        return 0;
    };

    // Field.
    use(m_theme.background);
    cr->paint();

    // a..i labels.
    use(m_theme.label);
    cr->set_font_size(gutter * 0.37);   // ~2/3 of the earlier size, toned down
    for (int i = 0; i < N; ++i) {
        std::string s(1, char('a' + i));
        centre_text(s, ox + i * cell + cell / 2.0, oy - gutter / 2.0);
        centre_text(s, ox - gutter / 2.0, oy + i * cell + cell / 2.0);
    }

    // Washes: selection, then teaching premise + placement, then conflicts.
    if (has_selection()) {
        use(m_theme.selection);
        cell_rect(m_sel_r, m_sel_c);
        cr->fill();
    }
    if (m_teaching) {
        use(m_theme.teach_premise);
        for (const Finding& f : m_teach_findings)
            for (const FCell& c : f.cells) { cell_rect(c.row, c.col); cr->fill(); }
        use(m_theme.teach_place);
        for (const Finding& f : m_teach_findings)
            if (f.placement) { cell_rect(f.placement->row, f.placement->col); cr->fill(); }
    }
    use(m_theme.conflict);
    if (m_show_conflicts)
        for (int r = 0; r < N; ++r)
            for (int c = 0; c < N; ++c)
                if (m_grid.value(r, c) && m_grid.conflict_at(r, c)) { cell_rect(r, c); cr->fill(); }

    // Grid lines.
    use(m_theme.grid_line);
    cr->set_line_width(1.0);
    for (int i = 0; i <= N; ++i) {
        double x = ox + i * cell; cr->move_to(x, oy); cr->line_to(x, oy + grid);
        double y = oy + i * cell; cr->move_to(ox, y); cr->line_to(ox + grid, y);
    }
    cr->stroke();
    use(m_theme.box_border);
    cr->set_line_width(3.0);
    for (int i = 0; i <= N; i += BOX) {
        double x = ox + i * cell; cr->move_to(x, oy); cr->line_to(x, oy + grid);
        double y = oy + i * cell; cr->move_to(ox, y); cr->line_to(ox + grid, y);
    }
    cr->stroke();

    // Values, then small numbers (pencil marks, or teaching candidates).
    const double third = cell / 3.0;
    auto draw_sub = [&](int r, int c, int d, const Rgba& col) {
        use(col);
        int sc = (d - 1) % BOX, sr = (d - 1) / BOX;   // 1,2,3 / 4,5,6 / 7,8,9
        centre_text(std::string(1, char('0' + d)),
                    ox + c * cell + (sc + 0.5) * third, oy + r * cell + (sr + 0.5) * third);
    };

    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            int v = m_grid.value(r, c);
            if (v) {
                // Colour: givens are neutral; a user digit that disagrees with
                // the solution shows red when the pref is on; otherwise it's a
                // normal entry.
                const Rgba* col = &m_theme.entry_digit;
                if (m_grid.given(r, c))
                    col = &m_theme.given_digit;
                else if (m_highlight_mistakes && m_solution.value(r, c) != 0 &&
                         v != m_solution.value(r, c))
                    col = &m_theme.mistake;
                use(*col);
                cr->set_font_size(cell * 0.6);
                centre_text(std::string(1, char('0' + v)),
                            ox + c * cell + cell / 2.0, oy + r * cell + cell / 2.0);
                continue;
            }

            cr->set_font_size(cell * 0.22);
            if (m_teaching) {
                // Show the computed candidates the technique reasons over, with
                // the ones it eliminates struck in red; the cell it fills shows
                // the placed digit large.
                if (int pd = placement_at(r, c)) {
                    use(m_theme.entry_digit);
                    cr->set_font_size(cell * 0.6);
                    centre_text(std::string(1, char('0' + pd)),
                                ox + c * cell + cell / 2.0, oy + r * cell + cell / 2.0);
                    continue;
                }
                Mask cand = m_teach_work.cand(r, c);
                for (int d = 1; d <= N; ++d) {
                    if (!has(cand, d)) continue;
                    draw_sub(r, c, d, is_eliminated(r, c, d) ? m_theme.conflict : m_theme.pencil);
                }
            } else if (m_show_notes) {
                // Normal play: the user's own pencil marks (hidden when the
                // show-notes view toggle is off; the notes stay in the model).
                Mask notes = m_grid.notes(r, c);
                for (int d = 1; d <= N; ++d)
                    if (has(notes, d)) draw_sub(r, c, d, m_theme.pencil);
            }
        }
    }

    if (auto lg = log::get(log::Area::Render)) lg->trace("board draw {}x{}", width, height);
}

}  // namespace sudoku
