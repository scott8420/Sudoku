#pragma once
#include <gtkmm/dropdown.h>
#include <gtkmm/stringlist.h>
#include <gtkmm/switch.h>
#include <gtkmm/window.h>

#include <sigc++/signal.h>
#include <vector>

namespace sudoku {

// PreferencesDialog — a small hide-on-close settings window (CANON's hide-on-
// close singleton shape). A pure view: it emits a signal per setting and holds
// no application state. MainWindow connects the signals to the live objects
// (the board, the skin list).
class PreferencesDialog : public Gtk::Window {
public:
    PreferencesDialog();
    ~PreferencesDialog() override;

    sigc::signal<void(bool)>& signal_show_conflicts()    { return m_sig_show_conflicts; }
    sigc::signal<void(bool)>& signal_highlight_mistakes() { return m_sig_highlight_mistakes; }
    sigc::signal<void(int)>&  signal_skin()               { return m_sig_skin; }

    // Sync the controls to current live values before the dialog is shown.
    void set_show_conflicts(bool on);
    void set_highlight_mistakes(bool on);
    void set_skins(const std::vector<Glib::ustring>& names, int active);

private:
    Gtk::Switch*   m_show_conflicts    = nullptr;
    Gtk::Switch*   m_highlight_mistakes = nullptr;
    Gtk::DropDown* m_skin              = nullptr;
    Glib::RefPtr<Gtk::StringList> m_skin_model;
    bool m_syncing = false;   // guard programmatic dropdown updates from re-emitting

    sigc::signal<void(bool)> m_sig_show_conflicts;
    sigc::signal<void(bool)> m_sig_highlight_mistakes;
    sigc::signal<void(int)>  m_sig_skin;
};

}  // namespace sudoku
