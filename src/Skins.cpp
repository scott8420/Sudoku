#include "Skins.hpp"
#include "Log.hpp"

#include <glibmm/miscutils.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <optional>

namespace sudoku::skins {

namespace {
namespace fs = std::filesystem;
using nlohmann::json;

fs::path skins_dir() {
    return fs::path(Glib::get_user_data_dir()) / "sudoku" / "skins";
}

// "#rrggbb" or "#rrggbbaa" → Rgba. Returns nullopt on anything malformed so the
// caller keeps the fallback token.
std::optional<Rgba> parse_color(const std::string& in) {
    std::string s = in;
    if (!s.empty() && s[0] == '#') s.erase(0, 1);
    auto nib = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    auto byte = [&](size_t i) -> int {
        int h = nib(s[i]), l = nib(s[i + 1]);
        return (h < 0 || l < 0) ? -1 : h * 16 + l;
    };
    if (s.size() != 6 && s.size() != 8) return std::nullopt;
    int r = byte(0), g = byte(2), b = byte(4);
    int a = (s.size() == 8) ? byte(6) : 255;
    if (r < 0 || g < 0 || b < 0 || a < 0) return std::nullopt;
    return Rgba{r / 255.0, g / 255.0, b / 255.0, a / 255.0};
}

std::string to_hex(const Rgba& c) {
    auto ch = [](double v) { return std::clamp(int(v * 255.0 + 0.5), 0, 255); };
    char buf[10];
    std::snprintf(buf, sizeof buf, "#%02x%02x%02x%02x", ch(c.r), ch(c.g), ch(c.b), ch(c.a));
    return buf;
}

Theme from_json(const json& j) {
    Theme t = Theme::default_dark();   // every missing token keeps this value
    if (j.contains("name") && j["name"].is_string()) t.name = j["name"].get<std::string>();
    auto tok = [&](const char* key, Rgba& field) {
        if (j.contains(key) && j[key].is_string())
            if (auto c = parse_color(j[key].get<std::string>())) field = *c;
    };
    tok("background", t.background);       tok("grid_line", t.grid_line);
    tok("box_border", t.box_border);       tok("label", t.label);
    tok("given_digit", t.given_digit);     tok("entry_digit", t.entry_digit);
    tok("pencil", t.pencil);               tok("selection", t.selection);
    tok("selection_notes", t.selection_notes);
    tok("conflict", t.conflict);           tok("mistake", t.mistake);
    tok("teach_premise", t.teach_premise); tok("teach_place", t.teach_place);
    return t;
}

// A second built-in preset (a warmer, higher-contrast dark) so the picker has
// more than one option out of the box.
Theme slate() {
    Theme t = Theme::default_dark();
    t.name          = "Slate";
    t.background    = {0.09, 0.10, 0.12, 1.0};
    t.grid_line     = {0.26, 0.28, 0.33, 1.0};
    t.box_border    = {0.62, 0.66, 0.74, 1.0};
    t.given_digit   = {0.96, 0.97, 0.99, 1.0};
    t.entry_digit   = {0.98, 0.76, 0.40, 1.0};
    t.selection     = {0.30, 0.34, 0.42, 0.55};
    return t;
}
}  // namespace

std::vector<Theme> builtin() { return {Theme::default_dark(), slate()}; }

std::vector<Theme> user() {
    std::vector<Theme> out;
    std::error_code ec;
    fs::path dir = skins_dir();
    if (!fs::exists(dir, ec)) return out;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (ec) break;
        if (entry.path().extension() != ".json") continue;
        try {
            std::ifstream f(entry.path());
            json j;
            f >> j;
            Theme t = from_json(j);
            if (t.name.empty()) t.name = entry.path().stem().string();
            out.push_back(std::move(t));
        } catch (const std::exception& e) {
            if (auto lg = log::get(log::Area::Io))
                lg->warn("skin skipped: {} ({})", entry.path().string(), e.what());
        }
    }
    return out;
}

std::vector<Theme> all() {
    std::vector<Theme> v = builtin();
    for (auto& t : user()) v.push_back(std::move(t));
    return v;
}

void ensure_sample() {
    std::error_code ec;
    fs::path dir = skins_dir();
    fs::create_directories(dir, ec);
    if (ec) return;

    // Only seed when the dir has no skins yet, so we never clobber the user's.
    bool has_json = false;
    for (const auto& e : fs::directory_iterator(dir, ec)) {
        if (e.path().extension() == ".json") { has_json = true; break; }
    }
    if (has_json) return;

    Theme d = Theme::default_dark();
    json j;
    j["name"]          = "Example (copy me)";
    j["background"]    = to_hex(d.background);
    j["grid_line"]     = to_hex(d.grid_line);
    j["box_border"]    = to_hex(d.box_border);
    j["label"]         = to_hex(d.label);
    j["given_digit"]   = to_hex(d.given_digit);
    j["entry_digit"]   = to_hex(d.entry_digit);
    j["pencil"]        = to_hex(d.pencil);
    j["selection"]     = to_hex(d.selection);
    j["selection_notes"] = to_hex(d.selection_notes);
    j["conflict"]      = to_hex(d.conflict);
    j["mistake"]       = to_hex(d.mistake);
    j["teach_premise"] = to_hex(d.teach_premise);
    j["teach_place"]   = to_hex(d.teach_place);

    try {
        std::ofstream f(dir / "example.json");
        f << j.dump(2) << "\n";
    } catch (...) { /* non-fatal: a missing template just means no example */ }
}

}  // namespace sudoku::skins
