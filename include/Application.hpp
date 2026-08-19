#pragma once
#include <gtkmm/application.h>

namespace sudoku {

class MainWindow;

// Application — the Gtk::Application in the Curvz shape: create() hands back a
// RefPtr, the constructor stands up logging, on_startup registers accels
// (application-scope, so they survive any window reshuffle), and on_activate
// builds the window. Kept intentionally thin; the game lives in MainWindow and
// the model.
class Application : public Gtk::Application {
public:
    static Glib::RefPtr<Application> create();

protected:
    Application();
    void on_startup() override;
    void on_activate() override;

private:
    MainWindow* m_main_window = nullptr;
};

}  // namespace sudoku
