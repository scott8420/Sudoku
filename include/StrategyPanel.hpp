#pragma once
#include "model/Technique.hpp"

#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/listboxrow.h>

#include <sigc++/signal.h>
#include <vector>

namespace sudoku {

// StrategyPanel — the drawer content: a list of solving strategies and an
// explanation area. It is a pure view: it knows the ladder's technique ids and
// emits a signal when one is chosen; it does not touch the board or the model.
// MainWindow connects the signal to the board's teaching overlay.
class StrategyPanel : public Gtk::Box {
public:
    StrategyPanel();

    sigc::signal<void(model::Technique)>& signal_technique() { return m_sig_technique; }
    sigc::signal<void()>&                 signal_cleared()   { return m_sig_cleared; }

    void set_explanation(const Glib::ustring& text) { m_explain.set_text(text); }
    void reset();  // unselect + default hint text

private:
    void on_row_selected(Gtk::ListBoxRow* row);

    Gtk::ListBox m_list;
    Gtk::Label   m_explain;
    std::vector<model::Technique> m_row_tech;  // parallel to the list rows

    sigc::signal<void(model::Technique)> m_sig_technique;
    sigc::signal<void()>                 m_sig_cleared;
};

}  // namespace sudoku
