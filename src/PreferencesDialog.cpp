#include "PreferencesDialog.hpp"
#include "WidgetRegistry.hpp"

#include <gtkmm/box.h>
#include <gtkmm/label.h>

namespace sudoku {

namespace {
// One labelled-control row: "<label>            [control]".
Gtk::Box* pref_row(const Glib::ustring& text, Gtk::Widget& control) {
    auto* row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    row->set_spacing(12);
    auto* label = Gtk::make_managed<Gtk::Label>(text);
    label->set_xalign(0.0);
    label->set_hexpand(true);
    control.set_valign(Gtk::Align::CENTER);
    row->append(*label);
    row->append(control);
    return row;
}
}  // namespace

PreferencesDialog::PreferencesDialog() {
    set_name("preferences_dialog");
    registry::add("preferences_dialog", this);

    set_title("Preferences");
    set_default_size(360, -1);
    set_resizable(false);
    set_hide_on_close(true);

    auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    box->set_margin(18);
    box->set_spacing(12);

    auto* sw = Gtk::make_managed<Gtk::Switch>();
    sw->set_active(true);
    sw->property_active().signal_changed().connect(
        [this, sw]() { m_sig_show_conflicts.emit(sw->get_active()); });
    m_show_conflicts = sw;
    box->append(*pref_row("Show conflicts", *sw));

    auto* mk = Gtk::make_managed<Gtk::Switch>();
    mk->set_active(true);
    mk->property_active().signal_changed().connect(
        [this, mk]() { m_sig_highlight_mistakes.emit(mk->get_active()); });
    m_highlight_mistakes = mk;
    box->append(*pref_row("Highlight mistakes", *mk));

    m_skin_model = Gtk::StringList::create(std::vector<Glib::ustring>{});
    auto* dd = Gtk::make_managed<Gtk::DropDown>();
    dd->set_model(m_skin_model);
    dd->property_selected().signal_changed().connect([this, dd]() {
        if (m_syncing) return;   // programmatic set_skins should not re-emit
        m_sig_skin.emit(int(dd->get_selected()));
    });
    m_skin = dd;
    box->append(*pref_row("Skin", *dd));

    set_child(*box);
}

PreferencesDialog::~PreferencesDialog() { registry::remove(this); }

void PreferencesDialog::set_show_conflicts(bool on) {
    if (m_show_conflicts) m_show_conflicts->set_active(on);
}

void PreferencesDialog::set_highlight_mistakes(bool on) {
    if (m_highlight_mistakes) m_highlight_mistakes->set_active(on);
}

void PreferencesDialog::set_skins(const std::vector<Glib::ustring>& names, int active) {
    if (!m_skin) return;
    m_syncing = true;
    m_skin_model = Gtk::StringList::create(names);
    m_skin->set_model(m_skin_model);
    if (active >= 0 && active < int(names.size())) m_skin->set_selected(guint(active));
    m_syncing = false;
}

}  // namespace sudoku
