#include "Board.hpp"
#include "Log.hpp"
#include "WidgetRegistry.hpp"
#include "model/Solver.hpp"

#include <gdk/gdkkeysyms.h>
#include <gtkmm/printoperation.h>
#include <gtkmm/printcontext.h>
#include <glibmm/main.h>

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
    // Land on the first cell the player can actually write in. The old default
    // was a hardcoded (0,0), which is a given in most generated puzzles.
    select_first_open();
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

void Board::deselect() {
    m_sel_r = -1;
    m_sel_c = -1;
    queue_draw();
}

void Board::select(int r, int c) {
    // Targeted landing. A given can't hold anything the player types, so
    // selecting one would present an editing caret over an uneditable cell --
    // the wash would promise an interaction that every input path then refuses.
    // Deselecting says the same thing without the lie.
    if (r < 0 || r >= model::N || c < 0 || c >= model::N) { deselect(); return; }
    if (m_grid.given(r, c))                               { deselect(); return; }
    m_sel_r = r;
    m_sel_c = c;
    queue_draw();
}

void Board::move_selection(int dr, int dc) {
    if (dr == 0 && dc == 0) return;

    // Travel with nothing selected re-enters the board rather than doing
    // nothing -- otherwise clicking a given would leave the arrow keys dead
    // until the player reached for the mouse again.
    if (!has_selection()) { select_first_open(); return; }

    // Step in the requested direction until an editable cell turns up. Givens
    // are passed over, not landed on. Running off the edge leaves the selection
    // where it was, which is how the old clamp behaved at the boundary.
    int r = m_sel_r + dr;
    int c = m_sel_c + dc;
    while (r >= 0 && r < model::N && c >= 0 && c < model::N) {
        if (!m_grid.given(r, c)) {
            m_sel_r = r;
            m_sel_c = c;
            queue_draw();
            return;
        }
        r += dr;
        c += dc;
    }
}

void Board::select_first_open() {
    for (int r = 0; r < model::N; ++r)
        for (int c = 0; c < model::N; ++c)
            if (!m_grid.given(r, c)) { m_sel_r = r; m_sel_c = c; queue_draw(); return; }
    deselect();   // a fully-given grid has nothing to select (shouldn't happen)
}

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

bool Board::has_solution() const {
    for (int r = 0; r < model::N; ++r)
        for (int c = 0; c < model::N; ++c)
            if (m_solution.value(r, c) == 0) return false;
    return true;
}

void Board::draw_solution_key(const Cairo::RefPtr<Cairo::Context>& cr,
                              double x, double y, double size, const Theme& t) const {
    using namespace sudoku::model;
    auto use = [&](const Rgba& c) { cr->set_source_rgba(c.r, c.g, c.b, c.a); };
    const double cell = size / N;

    // Deliberately NOT render() with a solved grid. The key is a different
    // object from the board: no gutter, no washes, hairline rules, and digits
    // small enough that it reads as a reference block rather than a second
    // puzzle competing for attention with the real one.
    use(t.grid_line);
    cr->set_line_width(0.4);
    for (int i = 0; i <= N; ++i) {
        double gx = x + i * cell; cr->move_to(gx, y); cr->line_to(gx, y + size);
        double gy = y + i * cell; cr->move_to(x, gy); cr->line_to(x + size, gy);
    }
    cr->stroke();

    use(t.box_border);
    cr->set_line_width(1.0);
    for (int i = 0; i <= N; i += BOX) {
        double gx = x + i * cell; cr->move_to(gx, y); cr->line_to(gx, y + size);
        double gy = y + i * cell; cr->move_to(x, gy); cr->line_to(x + size, gy);
    }
    cr->stroke();

    use(t.given_digit);
    cr->set_font_size(cell * 0.68);
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            const int v = m_solution.value(r, c);
            if (!v) continue;
            const std::string s(1, char('0' + v));
            Cairo::TextExtents e;
            cr->get_text_extents(s, e);
            cr->move_to(x + c * cell + cell / 2.0 - e.width / 2.0 - e.x_bearing,
                        y + r * cell + cell / 2.0 - e.height / 2.0 - e.y_bearing);
            cr->show_text(s);
        }
    }
}

void Board::print(Gtk::Window& parent, const std::string& subtitle) {
    // The operation is held as a MEMBER, not a local. run() blocks for a print
    // job, which makes a local RefPtr look correct, but GTK can finish the
    // render after run() returns -- and a local drops its last reference at
    // that point, taking the draw-page slot with it while GTK still needs it.
    // Held here and released on signal_done instead.
    //
    // A stale operation is REPLACED, never allowed to block. An earlier guard
    // returned early while one was held, which wedged printing for the rest of
    // the session any time signal_done failed to fire -- one bad job turned the
    // menu item permanently dead, silently. Worst case now is dropping a job
    // that already went wrong, instead of every job after it.
    if (m_print_op) m_print_op.reset();
    m_print_op = Gtk::PrintOperation::create();
    auto op = m_print_op;
    op->set_job_name("Sudoku");
    op->set_n_pages(1);
    op->set_use_full_page(false);   // honour the printer's margins

    const Theme paper = Theme::print_light();

    // NOTE (s11): the print dialog's "Preview" button does nothing under GNOME's
    // portal printing, and that is NOT a bug here. Portal printing has no preview
    // in its D-Bus interface, so GTK never emits ::preview to the application at
    // all -- instrumenting the signal showed it never firing while begin/draw/end
    // ran normally and run() returned APPLY. Confirmed system-wide on the same
    // machine: Curvz (same API call) and Papers fail identically, and forcing the
    // classic dialog with GTK_USE_PORTAL=0 does not restore it. Nothing to fix on
    // our side; if portal printing gains preview, this starts working unchanged.
    // Use Print to File -> PDF to see the page.

    // Released only once GTK says the job is finished. The reset is deferred to
    // an idle callback rather than done inline: the slot is owned by the very
    // object being dropped, so freeing it during its own signal emission would
    // pull the ground out from under the emitter.
    op->signal_done().connect([this](Gtk::PrintOperation::Result) {
        Glib::signal_idle().connect_once([this]() { m_print_op.reset(); });
    });

    op->signal_draw_page().connect(
        [this, paper, subtitle](const Glib::RefPtr<Gtk::PrintContext>& ctx, int) {
            auto cr = ctx->get_cairo_context();
            if (!cr) return;
            const double W = ctx->get_width();
            const double H = ctx->get_height();

            auto ink = [&](const Rgba& c) { cr->set_source_rgba(c.r, c.g, c.b, c.a); };

            // ── Heading ──────────────────────────────────────────────────────
            ink(paper.given_digit);
            cr->set_font_size(16.0);
            cr->move_to(0, 16.0);
            cr->show_text("Sudoku");
            if (!subtitle.empty()) {
                ink(paper.label);
                cr->set_font_size(10.0);
                Cairo::TextExtents e;
                cr->get_text_extents(subtitle, e);
                cr->move_to(W - e.width - e.x_bearing, 16.0);
                cr->show_text(subtitle);
            }
            const double head = 34.0;

            // ── Layout ───────────────────────────────────────────────────────
            // The key is sized first and the board takes what's left, so the
            // board shrinks on a short page rather than the key colliding with
            // it. Bottom-right corner for the key: it's the corner a sheet
            // folds over most naturally, which is the whole point of it.
            const bool   key      = has_solution();
            const double key_side = key ? std::min(W * 0.26, H * 0.26) : 0.0;
            const double cap      = key ? 13.0 : 0.0;
            const double gap      = key ? 20.0 : 0.0;

            const double board = std::min(W, H - head - key_side - cap - gap);
            if (board > 0) {
                cr->save();
                cr->translate((W - board) / 2.0, head);
                RenderOpts o;
                o.theme            = &paper;
                o.paint_background = false;   // the page is already white
                o.labels           = false;
                o.selection        = false;   // a caret is meaningless on paper
                o.teaching         = false;
                o.conflicts        = false;
                o.mistakes         = false;   // printing the red would be a partial answer key
                o.notes            = true;    // the player's own reasoning travels with them
                render(cr, int(board), int(board), o);
                cr->restore();
            }

            if (key) {
                const double kx = W - key_side;
                const double ky = H - key_side;
                ink(paper.label);
                cr->set_font_size(9.0);
                cr->move_to(kx, ky - 4.0);
                cr->show_text("Solution \u2014 fold under until finished");
                draw_solution_key(cr, kx, ky, key_side, paper);
            }
        });

    try {
        // PRINT_DIALOG gives the standard GTK dialog, which carries "Print to
        // File" -> PDF for free. No separate export path is needed.
        const auto result = op->run(Gtk::PrintOperation::Action::PRINT_DIALOG, parent);
        if (result == Gtk::PrintOperation::Result::ERROR) {
            if (auto lg = log::get(log::Area::Io))
                lg->error("print: operation reported ERROR");
        }
    } catch (const Glib::Error& e) {
        if (auto lg = log::get(log::Area::Io)) lg->error("print failed: {}", e.what());
    }
}

void Board::on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height) {
    RenderOpts o;
    o.theme            = &m_theme;
    o.paint_background = true;
    o.labels           = true;
    o.selection        = has_selection();
    o.notes_mode       = (m_mode == Mode::Notes);
    o.teaching         = m_teaching;
    o.conflicts        = m_show_conflicts;
    o.mistakes         = m_highlight_mistakes;
    o.notes            = m_show_notes;

    // Cache the geometry for hit-testing. render() RETURNS it rather than
    // writing the members, because the printer calls render() too -- if it
    // stored geometry, printing a page would silently rewrite the board's
    // click-to-cell mapping to page coordinates and every subsequent click
    // would land on the wrong square, long after the print looked fine.
    const Geometry g = render(cr, width, height, o);
    m_ox = g.ox; m_oy = g.oy; m_cell = g.cell;

    if (auto lg = log::get(log::Area::Render)) lg->trace("board draw {}x{}", width, height);
}

Board::Geometry Board::render(const Cairo::RefPtr<Cairo::Context>& cr,
                              int width, int height, const RenderOpts& o) const {
    using namespace sudoku::model;
    const Theme& theme = o.theme ? *o.theme : m_theme;
    auto use = [&](const Rgba& c) { cr->set_source_rgba(c.r, c.g, c.b, c.a); };

    const double sq     = std::min(width, height);
    const double gutter = sq * 0.055;
    const double grid   = sq - gutter;
    const double cell   = grid / N;
    const double ox     = (width - sq) / 2.0 + gutter;
    const double oy     = (height - sq) / 2.0 + gutter;

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
    if (o.paint_background) {
        use(theme.background);
        cr->paint();
    }

    // a..i labels. Off on paper: they're a screen aid for the keyboard jump,
    // and a printed puzzle has no keyboard.
    if (o.labels) {
        use(theme.label);
        cr->set_font_size(gutter * 0.37);   // ~2/3 of the earlier size, toned down
        for (int i = 0; i < N; ++i) {
            std::string s(1, char('a' + i));
            centre_text(s, ox + i * cell + cell / 2.0, oy - gutter / 2.0);
            centre_text(s, ox - gutter / 2.0, oy + i * cell + cell / 2.0);
        }
    }

    // Washes: selection, then teaching premise + placement, then conflicts.
    // The selection wash carries the mode: Guess uses the cool `selection`
    // token, Notes the warm `selection_notes`. That makes the mode readable off
    // the board itself instead of only off the Notes button in the control bar —
    // which matters most exactly when you aren't looking at the button, i.e.
    // while typing digits.
    if (o.selection && has_selection()) {
        use(o.notes_mode ? theme.selection_notes : theme.selection);
        cell_rect(m_sel_r, m_sel_c);
        cr->fill();
    }
    if (o.teaching) {
        use(theme.teach_premise);
        for (const Finding& f : m_teach_findings)
            for (const FCell& c : f.cells) { cell_rect(c.row, c.col); cr->fill(); }
        use(theme.teach_place);
        for (const Finding& f : m_teach_findings)
            if (f.placement) { cell_rect(f.placement->row, f.placement->col); cr->fill(); }
    }
    use(theme.conflict);
    if (o.conflicts)
        for (int r = 0; r < N; ++r)
            for (int c = 0; c < N; ++c)
                if (m_grid.value(r, c) && m_grid.conflict_at(r, c)) { cell_rect(r, c); cr->fill(); }

    // Grid lines.
    use(theme.grid_line);
    cr->set_line_width(1.0);
    for (int i = 0; i <= N; ++i) {
        double x = ox + i * cell; cr->move_to(x, oy); cr->line_to(x, oy + grid);
        double y = oy + i * cell; cr->move_to(ox, y); cr->line_to(ox + grid, y);
    }
    cr->stroke();
    use(theme.box_border);
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
                const Rgba* col = &theme.entry_digit;
                if (m_grid.given(r, c))
                    col = &theme.given_digit;
                else if (o.mistakes && m_solution.value(r, c) != 0 &&
                         v != m_solution.value(r, c))
                    col = &theme.mistake;
                use(*col);
                cr->set_font_size(cell * 0.6);
                centre_text(std::string(1, char('0' + v)),
                            ox + c * cell + cell / 2.0, oy + r * cell + cell / 2.0);
                continue;
            }

            cr->set_font_size(cell * 0.22);
            if (o.teaching) {
                // Show the computed candidates the technique reasons over, with
                // the ones it eliminates struck in red; the cell it fills shows
                // the placed digit large.
                if (int pd = placement_at(r, c)) {
                    use(theme.entry_digit);
                    cr->set_font_size(cell * 0.6);
                    centre_text(std::string(1, char('0' + pd)),
                                ox + c * cell + cell / 2.0, oy + r * cell + cell / 2.0);
                    continue;
                }
                Mask cand = m_teach_work.cand(r, c);
                for (int d = 1; d <= N; ++d) {
                    if (!has(cand, d)) continue;
                    draw_sub(r, c, d, is_eliminated(r, c, d) ? theme.conflict : theme.pencil);
                }
            } else if (o.notes) {
                // Normal play: the user's own pencil marks (hidden when the
                // show-notes view toggle is off; the notes stay in the model).
                Mask notes = m_grid.notes(r, c);
                for (int d = 1; d <= N; ++d)
                    if (has(notes, d)) draw_sub(r, c, d, theme.pencil);
            }
        }
    }

    return Geometry{ox, oy, cell};
}

}  // namespace sudoku
