#include "Application.hpp"
#include "Log.hpp"
#include "MainWindow.hpp"
#include "Shortcuts.hpp"

#include <gdkmm/display.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/settings.h>
#include <gtkmm/window.h>

#include <vector>

namespace sudoku {

Glib::RefPtr<Application> Application::create() {
    return Glib::make_refptr_for_instance<Application>(new Application());
}

Application::Application()
    // SUDOKU_APP_ID comes from CMakeLists.txt. It used to be a literal here
    // with a comment asking the reader to keep it matching -- which is a rule
    // the build could enforce instead, so now it does.
    : Gtk::Application(SUDOKU_APP_ID, Gio::Application::Flags::DEFAULT_FLAGS) {
    log::init();
    if (auto lg = log::get(log::Area::App)) lg->info("Sudoku starting");
}

void Application::on_startup() {
    Gtk::Application::on_startup();

    // Accelerators are an application-scope concern (they survive window
    // construction order), so they're bound here. They're driven from the pure
    // shortcut_registry() -- the same list ShortcutsDialog documents from -- so a
    // binding and its dialog row can never drift. Register a shortcut once there.
    for (const auto& s : shortcut_registry()) {
        if (s.action.empty() || s.accels.empty()) continue;  // doc-only rows (board keys, mouse)
        std::vector<Glib::ustring> accels(s.accels.begin(), s.accels.end());
        set_accels_for_action(s.action, accels);
    }
}

void Application::on_activate() {
    // Dark-mode native: request the system's dark variant (Adwaita-dark on
    // GNOME). The board carries its own dark theme tokens; this dresses the
    // chrome to match.
    if (auto settings = Gtk::Settings::get_default())
        settings->property_gtk_application_prefer_dark_theme() = true;

    // The app icon is bundled in the GResource under the icon-theme layout; make
    // it resolvable by name. The window/launcher icon is the full-colour tile;
    // the "-symbolic" variant (recoloured) is used only for the About logo.
    if (auto display = Gdk::Display::get_default())
        Gtk::IconTheme::get_for_display(display)->add_resource_path(
            SUDOKU_APP_PATH "/icons");
    Gtk::Window::set_default_icon_name(SUDOKU_APP_ID);

    auto* win = new MainWindow(*this);
    add_window(*win);
    win->present();
    m_main_window = win;
}

}  // namespace sudoku
