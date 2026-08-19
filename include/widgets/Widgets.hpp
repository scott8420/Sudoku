#pragma once
#include "WidgetRegistry.hpp"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/dropdown.h>
#include <gtkmm/label.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/widget.h>

#include <string>
#include <string_view>
#include <utility>

// Discipline through inheritance (CANON). A framework widget's name is optional
// and easy to skip; over hundreds of edits the Inspector tree becomes a forest
// of anonymous nodes and warnings lose their thread back to the object. The
// Named<W> wrapper makes the name mandatory in the ONLY constructor and
// registers into the address book by construction — the wrong thing (an
// unnamed widget) simply isn't expressible in app code.
//
// One template, every wrapper as an alias, so the discipline costs almost
// nothing and new wrappers are one-line additions as demand appears (CANON:
// "substrate widening follows demand, not symmetry"). Custom subclasses that
// carry real logic (MainWindow, Board) name and register themselves directly
// rather than through this template.
namespace sudoku::widgets {

template <class W>
class Named : public W {
public:
    template <class... Args>
    explicit Named(std::string_view name, Args&&... args)
        : W(std::forward<Args>(args)...) {
        this->set_name(std::string(name));
        registry::add(name, this);
    }
    ~Named() override { registry::remove(this); }
};

using Box          = Named<Gtk::Box>;
using Button       = Named<Gtk::Button>;
using DropDown     = Named<Gtk::DropDown>;
using Label        = Named<Gtk::Label>;
using ToggleButton = Named<Gtk::ToggleButton>;

}  // namespace sudoku::widgets
