#include "Shortcuts.hpp"

#include <cctype>
#include <map>

namespace sudoku {

// ─────────────────────────────────────────────────────────────────────────────
// Accel formatting
// ─────────────────────────────────────────────────────────────────────────────
namespace {

std::string lower(const std::string& s) {
    std::string o;
    o.reserve(s.size());
    for (char c : s) o += char(std::tolower((unsigned char)c));
    return o;
}

// GTK key name -> display glyph. Single alpha keys uppercase; named keys map to a
// glyph where one reads better, otherwise the name is prettified. Covers the keys
// the registry actually uses; falls through cleanly.
std::string map_key(const std::string& key) {
    if (key.empty()) return "";
    static const std::map<std::string, std::string> named = {
        {"comma", ","},   {"period", "."},   {"question", "?"},
        {"slash", "/"},   {"minus", "\u2212"}, {"plus", "+"},
        {"equal", "="},   {"space", "Space"},
        {"Return", "Enter"}, {"Escape", "Esc"},
        {"Left", "\u2190"}, {"Right", "\u2192"}, {"Up", "\u2191"}, {"Down", "\u2193"},
        {"Delete", "Del"}, {"BackSpace", "Backspace"},
    };
    auto it = named.find(key);
    if (it != named.end()) return it->second;
    if (key.size() == 1) {
        std::string s = key;
        s[0] = char(std::toupper((unsigned char)s[0]));
        return s;
    }
    std::string s = key;
    for (char& c : s)
        if (c == '_') c = ' ';
    return s;
}

}  // namespace

std::string format_accel(const std::string& accel) {
    bool ctrl = false, alt = false, shift = false, super = false;
    std::string key;
    std::size_t i = 0;
    while (i < accel.size()) {
        if (accel[i] == '<') {
            std::size_t j = accel.find('>', i);
            if (j == std::string::npos) break;
            const std::string ml = lower(accel.substr(i + 1, j - i - 1));
            if (ml == "ctrl" || ml == "control" || ml == "primary") ctrl = true;
            else if (ml == "alt" || ml == "mod1")                    alt = true;
            else if (ml == "shift")                                  shift = true;
            else if (ml == "super" || ml == "meta" || ml == "mod4")  super = true;
            i = j + 1;
        } else {
            key = accel.substr(i);
            break;
        }
    }
    const std::string k = map_key(key);
    std::string out;
    auto add = [&](const std::string& s) {
        if (!out.empty()) out += "+";
        out += s;
    };
    if (ctrl)  add("Ctrl");
    if (alt)   add("Alt");
    if (shift) add("Shift");
    if (super) add("Super");
    if (!k.empty()) add(k);
    return out;
}

std::string ShortcutSpec::display_keys() const {
    if (!keys.empty()) return keys;
    std::string out;
    for (const auto& a : accels) {
        if (!out.empty()) out += "  /  ";
        out += format_accel(a);
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// The registry  (authored section [A-Z] -> row)
// ─────────────────────────────────────────────────────────────────────────────
const std::vector<ShortcutSpec>& shortcut_registry() {
    // Fields: {section, action, accels, keys, description}
    // action non-empty => wired via set_accels_for_action. keys empty => derived.
    static const std::vector<ShortcutSpec> kReg = {
        // ── Board (cell navigation + entry; handled in Board::on_key) ──────────
        {"Board", "", {}, "A\u2013I, A\u2013I", "Select a cell (column, then row)"},
        {"Board", "", {}, "\u2190 \u2191 \u2192 \u2193", "Move the selection"},
        {"Board", "", {}, "1\u20139", "Enter a digit (toggles a note in Notes mode)"},
        {"Board", "", {}, "0 / Del / Backspace", "Clear the cell"},

        // ── Game ───────────────────────────────────────────────────────────────
        {"Game", "win.new-game", {"<Ctrl>n"}, "", "New game"},

        // ── General ──────────────────────────────────────────────────────────────
        {"General", "win.preferences", {"<Ctrl>comma"}, "", "Preferences\u2026"},
        {"General", "win.shortcuts", {"<Ctrl>question"}, "", "Keyboard shortcuts (this window)"},
        {"General", "win.quit", {"<Ctrl>q", "<Ctrl>w"}, "", "Quit"},

        // ── Mouse ────────────────────────────────────────────────────────────────
        {"Mouse", "", {}, "Click", "Select a cell"},

        // ── Notes ────────────────────────────────────────────────────────────────
        {"Notes", "", {}, "N / Space", "Toggle Notes / Guess mode"},
        {"Notes", "win.toggle-notes", {"<Ctrl>h"}, "", "Show / hide pencil marks"},
    };
    return kReg;
}

// ─────────────────────────────────────────────────────────────────────────────
// Collision detection
// ─────────────────────────────────────────────────────────────────────────────
std::vector<std::string> find_accel_collisions() {
    std::map<std::string, std::pair<int, bool>> seen;  // accel -> {count, any_action}
    for (const auto& s : shortcut_registry())
        for (const auto& a : s.accels) {
            auto& e = seen[a];
            e.first += 1;
            if (!s.action.empty()) e.second = true;
        }
    std::vector<std::string> out;
    for (const auto& [accel, e] : seen)
        if (e.first >= 2 && e.second) out.push_back(accel);  // std::map keeps it sorted+unique
    return out;
}

}  // namespace sudoku
