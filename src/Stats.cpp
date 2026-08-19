#include "Stats.hpp"
#include "Log.hpp"

#include <glibmm/miscutils.h>   // Glib::get_user_data_dir
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace sudoku {
namespace {

namespace fs = std::filesystem;
using nlohmann::json;

// <data>/sudoku/stats.json — sibling of the skins dir written by Skins.cpp.
fs::path stats_path() {
    return fs::path(Glib::get_user_data_dir()) / "sudoku" / "stats.json";
}

// The four bands in rank order; the JSON keys are the model's own name()s so the
// file is self-describing and matches the difficulty vocabulary everywhere else.
constexpr std::array<model::Difficulty, 4> kBands = {
    model::Difficulty::Easy, model::Difficulty::Medium,
    model::Difficulty::Hard, model::Difficulty::Expert};

}  // namespace

void Stats::record_solve(model::Difficulty d, int seconds) {
    Record& r = m_rec[model::rank(d)];
    r.played += 1;
    r.sum    += seconds;
    r.best    = (r.best == 0) ? seconds : std::min(r.best, seconds);
    save();
    if (auto lg = log::get(log::Area::Io))
        lg->info("stats: solved {} in {}s (played={}, best={}, avg={})",
                 model::name(d), seconds, r.played, r.best, r.avg());
}

void Stats::reset() {
    m_rec = {};   // every band back to zero
    save();
    if (auto lg = log::get(log::Area::Io)) lg->info("stats: reset");
}

void Stats::load() {
    m_rec = {};   // default to zeros; any read failure just leaves them zero
    fs::path path = stats_path();

    std::error_code ec;
    if (!fs::exists(path, ec)) return;   // first run — nothing to load, not an error

    try {
        std::ifstream f(path);
        json j;
        f >> j;
        for (model::Difficulty d : kBands) {
            auto it = j.find(model::name(d));
            if (it == j.end() || !it->is_object()) continue;   // missing band -> zero
            Record& r = m_rec[model::rank(d)];
            r.played = it->value("played", 0);
            r.best   = it->value("best", 0);
            r.sum    = it->value("sum", 0L);
        }
        if (auto lg = log::get(log::Area::Io))
            lg->info("stats: loaded from {}", path.string());
    } catch (const std::exception& e) {
        // Malformed store: keep the zeroed records, log, carry on (never fatal).
        m_rec = {};
        if (auto lg = log::get(log::Area::Io))
            lg->warn("stats: unreadable, starting fresh ({})", e.what());
    }
}

void Stats::save() const {
    fs::path path = stats_path();

    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);   // <data>/sudoku/ may not exist yet
    if (ec) {
        if (auto lg = log::get(log::Area::Io))
            lg->warn("stats: cannot create {} ({})", path.parent_path().string(), ec.message());
        return;
    }

    json j;
    for (model::Difficulty d : kBands) {
        const Record& r = m_rec[model::rank(d)];
        j[model::name(d)] = {{"played", r.played}, {"best", r.best}, {"sum", r.sum}};
    }

    try {
        std::ofstream f(path, std::ios::trunc);
        f << j.dump(2) << '\n';
    } catch (const std::exception& e) {
        if (auto lg = log::get(log::Area::Io))
            lg->warn("stats: write failed ({})", e.what());
    }
}

std::string format_duration(int seconds) {
    if (seconds < 0) seconds = 0;
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    char buf[16];
    if (h > 0) std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
    else       std::snprintf(buf, sizeof buf, "%02d:%02d", m, s);
    return buf;
}

}  // namespace sudoku
