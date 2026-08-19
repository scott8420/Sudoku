// selftest.cpp — a dependency-free exercise of the whole sudoku::model engine.
//
// Build it with nothing but a C++17 compiler:
//
//   g++ -std=c++17 -Iinclude src/model/*.cpp -o selftest && ./selftest
//
// There is no gtkmm here and no test framework — just a main() that generates,
// rates, solves, and teaches, then prints PASS/FAIL on a handful of invariants.
// It doubles as documentation: this is exactly how a consumer (the GTK UI, or a
// student who lifted model/ into their own project) drives the engine.

#include "model/Generator.hpp"
#include "model/Grid.hpp"
#include "model/Rater.hpp"
#include "model/Solver.hpp"
#include "model/Technique.hpp"
#include "model/WorkGrid.hpp"

#include "Shortcuts.hpp"   // UI-side but GTK-free: the accel-collision check rides here

#include <iostream>
#include <string>

using namespace sudoku::model;

namespace {

int g_checks = 0;
int g_failed = 0;

void check(bool cond, const std::string& what) {
    ++g_checks;
    if (!cond) {
        ++g_failed;
        std::cout << "  FAIL: " << what << "\n";
    }
}

int clue_count(const Grid& g) {
    int n = 0;
    for (char ch : g.to_string()) if (ch != '.') ++n;
    return n;
}

void print_grid(const Grid& g) {
    for (int r = 0; r < N; ++r) {
        std::cout << "    ";
        for (int c = 0; c < N; ++c) {
            int v = g.value(r, c);
            std::cout << (v ? char('0' + v) : '.') << ' ';
            if (c % BOX == BOX - 1 && c != N - 1) std::cout << "| ";
        }
        std::cout << "\n";
        if (r % BOX == BOX - 1 && r != N - 1)
            std::cout << "    ------+-------+------\n";
    }
}

void print_histogram(const DifficultyProfile& prof) {
    const auto& rungs = ladder();
    std::cout << "    steps=" << prof.steps << " solved=" << (prof.solved ? "yes" : "no")
              << " band=" << name(prof.band()) << "\n    histogram:";
    for (int i = 0; i < int(rungs.size()); ++i)
        std::cout << ' ' << name(rungs[i].id) << '=' << prof.histogram[i];
    std::cout << "\n";
}

// Print one finding the way the UI eventually will — but as plain text here,
// proving the engine hands out enough structure to narrate without knowing
// anything about presentation.
void print_finding(const Finding& f) {
    std::cout << "      " << name(f.technique) << " @";
    for (const FCell& c : f.cells) std::cout << " (" << c.row << ',' << c.col << ')';
    std::cout << "  digits:";
    for (int d : f.digits) std::cout << ' ' << d;
    if (f.placement)
        std::cout << "  => place " << f.placement->digit << " at ("
                  << f.placement->row << ',' << f.placement->col << ')';
    if (!f.eliminations.empty()) {
        std::cout << "  => strike";
        for (const CandidateRef& e : f.eliminations)
            std::cout << ' ' << e.digit << "@(" << e.row << ',' << e.col << ')';
    }
    std::cout << "\n";
}

// A fixed, known-unique puzzle for a deterministic sanity check independent of
// the random generator.
const char* kKnownPuzzle =
    "53..7...."
    "6..195..."
    ".98....6."
    "8...6...3"
    "4..8.3..1"
    "7...2...6"
    ".6....28."
    "...419..5"
    "....8..79";

void test_known_puzzle() {
    std::cout << "== known puzzle ==\n";
    Grid g = Grid::from_givens(kKnownPuzzle);
    print_grid(g);

    check(g.to_string() == std::string(kKnownPuzzle), "known puzzle round-trips");
    check(Solver::has_unique_solution(g), "known puzzle is unique");

    Grid solved = g;
    check(Solver::solve(solved), "known puzzle solves");
    check(solved.solved(), "known solution is complete and conflict-free");

    // Givens must be immutable: place()/clear() refuse them.
    check(!solved.place(0, 0, 1), "given cell rejects place()");
    check(!solved.clear(0, 0),    "given cell rejects clear()");

    DifficultyProfile prof = Rater::rate(g);
    print_histogram(prof);
    std::cout << "\n";
}

// The note semantics that the wrong-guess bug turned on. place() must be inert
// with respect to pencil marks; pruning is erase_peer_notes(), called only by a
// caller that knows the placement is correct. These are pure-model checks — the
// correctness GATE itself lives in the Board (it holds the solution), so what
// the engine has to guarantee is exactly this: no pruning happens unasked.
void test_note_semantics() {
    std::cout << "== note semantics ==\n";
    Grid g = Grid::from_givens(kKnownPuzzle);

    // (0,2) and (0,3) are empty and share row 0; (1,2) shares row-0's column 2.
    g.set_notes(0, 2, Mask(bit(1) | bit(2) | bit(4)));
    g.set_notes(0, 3, Mask(bit(2) | bit(4) | bit(9)));
    g.set_notes(1, 2, Mask(bit(2) | bit(7)));

    // A placement leaves every mask alone — its own and its peers'.
    check(g.place(0, 2, 4), "place() on an empty non-given succeeds");
    check(g.notes(0, 2) == Mask(bit(1) | bit(2) | bit(4)),
          "place() keeps the cell's own notes");
    check(has(g.notes(0, 3), 4), "place() does not prune a row peer's notes");
    check(has(g.notes(1, 2), 2), "place() does not prune a column peer's notes");

    // Clearing the value gives the player's reasoning back untouched.
    check(g.clear(0, 2), "clear() on a non-given succeeds");
    check(g.notes(0, 2) == Mask(bit(1) | bit(2) | bit(4)),
          "notes survive a place/clear round trip");

    // The pruning a confirmed placement justifies, asked for explicitly.
    g.place(0, 2, 4);
    g.erase_peer_notes(0, 2, 4);
    check(!has(g.notes(0, 3), 4), "erase_peer_notes strikes the digit from a row peer");
    check(has(g.notes(0, 3), 2),  "erase_peer_notes leaves other digits alone");
    check(has(g.notes(1, 2), 2),  "erase_peer_notes strikes only the placed digit");
    check(g.notes(0, 2) == Mask(bit(1) | bit(2) | bit(4)),
          "erase_peer_notes does not touch the placed cell itself");

    std::cout << "\n";
}

void test_generation() {
    std::cout << "== generation (band-targeted) ==\n";
    Generator gen(/*seed=*/12345u);

    for (Difficulty target : {Difficulty::Easy, Difficulty::Medium,
                              Difficulty::Hard, Difficulty::Expert}) {
        Difficulty actual = Difficulty::Easy;
        Grid puzzle = gen.generate(target, /*max_attempts=*/300, &actual);

        std::cout << "-- requested " << name(target) << ", got " << name(actual)
                  << ", clues=" << clue_count(puzzle) << " --\n";
        print_grid(puzzle);

        check(Solver::has_unique_solution(puzzle),
              std::string("generated ") + name(target) + " puzzle is unique");
        check(Grid::from_givens(puzzle.to_string()).to_string() == puzzle.to_string(),
              std::string("generated ") + name(target) + " puzzle round-trips");

        Grid solved = puzzle;
        check(Solver::solve(solved) && solved.solved(),
              std::string("generated ") + name(target) + " puzzle solves cleanly");

        DifficultyProfile prof = Rater::rate(puzzle);
        print_histogram(prof);
        std::cout << "\n";
    }
}

void test_teaching_filter() {
    std::cout << "== teaching filter (find_all per technique) ==\n";
    // A medium puzzle so the intermediate rungs actually light up.
    Generator gen(/*seed=*/777u);
    Difficulty actual = Difficulty::Easy;
    Grid puzzle = gen.generate(Difficulty::Hard, 300, &actual);
    std::cout << "puzzle (" << name(actual) << ", clues=" << clue_count(puzzle) << "):\n";
    print_grid(puzzle);

    WorkGrid w = WorkGrid::from(puzzle);
    for (const TechniqueEntry& rung : ladder()) {
        auto findings = rung.find_all(w);
        std::cout << "  " << name(rung.id) << ": " << findings.size() << " application(s)\n";
        // Show the first couple so the structured output is visible.
        for (size_t i = 0; i < findings.size() && i < 2; ++i) print_finding(findings[i]);
        // Every reported finding must be real progress — the guarantee the
        // rater relies on.
        for (const Finding& f : findings)
            check(f.is_progress(), std::string(name(rung.id)) + " findings are progress");
    }
    std::cout << "\n";
}

}  // namespace

int main() {
    std::cout << "sudoku::model selftest\n"
                 "======================\n\n";

    test_known_puzzle();
    test_note_semantics();
    test_generation();
    test_teaching_filter();

    // Shortcut registry (UI-side but pure): no two chords may collide, and every
    // wired row must carry an action string. Guards the one-source-of-truth seam
    // that Application wiring and ShortcutsDialog both read from.
    std::cout << "== shortcut registry ==\n";
    auto collisions = sudoku::find_accel_collisions();
    for (const auto& a : collisions) std::cout << "  COLLISION: " << a << "\n";
    check(collisions.empty(), "shortcut registry is accel-collision-free");
    std::cout << "  " << sudoku::shortcut_registry().size() << " specs registered\n\n";

    std::cout << "======================\n"
              << g_checks << " checks, " << g_failed << " failed — "
              << (g_failed == 0 ? "PASS" : "FAIL") << "\n";
    return g_failed == 0 ? 0 : 1;
}
