#include "StrategyPanel.hpp"
#include "Teach.hpp"
#include "WidgetRegistry.hpp"
#include "widgets/Widgets.hpp"

#include <gtkmm/button.h>

namespace sudoku {

StrategyPanel::StrategyPanel() : Gtk::Box(Gtk::Orientation::VERTICAL) {
    set_name("strategy_panel");
    registry::add("strategy_panel", this);

    set_size_request(250, -1);
    set_margin(10);
    set_spacing(8);

    auto* title = Gtk::make_managed<widgets::Label>("sp_title", "Strategies");
    title->set_xalign(0.0);
    title->add_css_class("title-4");
    append(*title);

    // One row per ladder rung, in cost order (the same order the rater uses).
    for (const auto& rung : model::ladder()) {
        m_row_tech.push_back(rung.id);
        auto* row = Gtk::make_managed<Gtk::Label>(display_name(rung.id));
        row->set_xalign(0.0);
        row->set_margin(6);
        m_list.append(*row);  // ListBox wraps each child in a ListBoxRow
    }
    m_list.set_selection_mode(Gtk::SelectionMode::SINGLE);
    m_list.signal_row_selected().connect(sigc::mem_fun(*this, &StrategyPanel::on_row_selected));
    append(m_list);

    auto* clear = Gtk::make_managed<widgets::Button>("sp_clear", "Clear highlights");
    clear->signal_clicked().connect([this]() {
        m_list.unselect_all();
        m_sig_cleared.emit();
        set_explanation("Select a strategy to see where it applies.");
    });
    append(*clear);

    m_explain.set_wrap(true);
    m_explain.set_xalign(0.0);
    m_explain.set_yalign(0.0);
    m_explain.set_vexpand(true);
    append(m_explain);

    reset();
}

void StrategyPanel::reset() {
    m_list.unselect_all();
    set_explanation("Select a strategy to see where it applies.");
}

void StrategyPanel::on_row_selected(Gtk::ListBoxRow* row) {
    if (!row) return;  // deselection
    int i = row->get_index();
    if (i >= 0 && i < int(m_row_tech.size())) m_sig_technique.emit(m_row_tech[i]);
}

}  // namespace sudoku
