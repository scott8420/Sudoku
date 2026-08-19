#include "WidgetRegistry.hpp"
#include "Log.hpp"

#include <gtkmm/widget.h>

#include <unordered_map>

namespace sudoku::registry {

namespace {

// Two maps so both directions are O(1): name→widget for find() (the gdb
// lookup), widget→name for remove() (the destructor path, which only has the
// pointer). They're kept in sync at every add/remove.
std::unordered_map<std::string, Gtk::Widget*>& by_name() {
    static std::unordered_map<std::string, Gtk::Widget*> m;
    return m;
}
std::unordered_map<Gtk::Widget*, std::string>& by_ptr() {
    static std::unordered_map<Gtk::Widget*, std::string> m;
    return m;
}

}  // namespace

void add(std::string_view name, Gtk::Widget* w) {
    std::string key(name);
    auto& names = by_name();
    if (names.count(key)) {
        // A live duplicate name means two widgets share an address — exactly
        // the collision CANON warns about. We don't throw here (this is the
        // diagnostic half, not the scriptable registry), but we flag it loudly
        // so the name clash is visible in the log.
        if (auto lg = log::get(log::Area::App))
            lg->warn("widget registry: duplicate name '{}'", key);
    }
    names[key]     = w;
    by_ptr()[w]    = key;
}

void remove(Gtk::Widget* w) {
    auto& ptrs = by_ptr();
    auto it = ptrs.find(w);
    if (it == ptrs.end()) return;
    by_name().erase(it->second);
    ptrs.erase(it);
}

Gtk::Widget* find(std::string_view name) {
    auto& names = by_name();
    auto it = names.find(std::string(name));
    return it == names.end() ? nullptr : it->second;
}

void dump() {
    auto lg = log::get(log::Area::App);
    if (!lg) return;
    lg->info("widget registry: {} named widgets", by_name().size());
    for (const auto& [name, w] : by_name())
        lg->info("  {} -> {}", name, static_cast<const void*>(w));
}

}  // namespace sudoku::registry
