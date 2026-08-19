#pragma once
#include "model/Rater.hpp"   // model::Difficulty, rank(), name()

#include <array>
#include <string>

namespace sudoku {

// Stats — the app's persistent solve record, and its first file-it-writes.
//
// One Record per difficulty band, keyed by the band's name() ("easy" ...
// "expert"). The store lives at <data>/sudoku/stats.json, loaded once at
// construction and rewritten on every solve and on reset. This is UI-side and
// std-only: the pure engine never sees it (mirror of Skins, which is the other
// file-it-writes seam).
//
// Fidelity note (CANON: round-trip is non-negotiable): the file persists the
// SUM of solve times, not a rounded average. Average is presented as
// sum / played at read time, so it never accumulates rounding drift across
// sessions. best is the fastest solve; 0 means "no games yet".
//
// Robustness (CANON: a missing/malformed store is not fatal): if the file is
// absent, unparseable, or missing keys, the affected records read as zero. A
// user never loses the app to a bad stats file.
struct Record {
    int  played = 0;   // games solved in this band
    int  best   = 0;   // fastest solve, seconds; 0 == none yet
    long sum    = 0;   // total solve seconds (for drift-free average)

    // Presented average (seconds), rounded; 0 when no games recorded.
    int avg() const { return played > 0 ? int(sum / played) : 0; }
};

class Stats {
public:
    Stats() { load(); }   // pulls the store immediately, robust to absence

    const Record& record(model::Difficulty d) const { return m_rec[model::rank(d)]; }

    // Record a solve: bump played, fold the time into sum + best, persist.
    void record_solve(model::Difficulty d, int seconds);

    // Zero every band and persist (the confirm-guarded Reset). Clears the file's
    // contents to zeros rather than deleting it, so the store stays valid.
    void reset();

private:
    void load();          // read the store; absence/malformed -> zeros
    void save() const;    // write the store (creates the dir if needed)

    std::array<Record, 4> m_rec{};   // indexed by model::rank(Difficulty)
};

// Format a whole-second duration as MM:SS, or H:MM:SS past an hour. Shared by
// the timer, the success dialog, and the stats panel so every surface reads the
// same way. Stats owns time semantics, so the helper lives here.
std::string format_duration(int seconds);

}  // namespace sudoku
