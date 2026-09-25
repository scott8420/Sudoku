# Sudoku

A modern Sudoku game for GNOME, built in C++ on GTK4 / gtkmm-4.0, with a
built-in strategy teacher.

## Features

- **Generated puzzles in four bands** (Easy, Medium, Hard, Expert), rated by
  which human solving techniques a puzzle actually requires.
- **Strategies drawer** (left edge): pick a technique and the board shows where
  it applies right now -- candidates, the premise cells, the eliminations, and
  the placement -- with a plain-language explanation.
- **Play assistance**: Guess / Notes modes, 3x3 pencil marks, one-click
  candidate Fill, conflict and mistake highlighting (each switchable in
  Preferences), and a Clear button that resets the board to its starting clues.
- **Stats drawer** (right edge): a game clock that starts on your first move and
  pauses when the window loses focus, plus per-difficulty solves, best, and
  average times.
- **Skins**: built-in board themes plus your own JSON skins.
- **Keyboard play** throughout (Keyboard Shortcuts in the menu), and **Print**
  with a small solution key.

## How difficulty is rated

Most Sudoku apps rate a puzzle by how many clues it has, or by how hard a
computer found it to solve. Neither tracks how hard the puzzle feels to a
person. This one rates a puzzle by the work a human solver would do.

1. **A ladder of human techniques, cheapest first.** Naked single, hidden
   single, locked candidates, naked pair, hidden pair. The order is the
   difficulty scale.
2. **Solve it the way a person would.** At every step the rater takes the
   *simplest* technique that makes progress, then starts again from the
   bottom. A step is always credited to the easiest technique that could have
   made it, never to a fancier one that also could have.
3. **Record the whole solve path, not one number.** The result is a histogram:
   how many steps each technique carried. That fingerprint separates a puzzle
   that needs one hard move from one that needs twenty.
4. **Read the band from the fingerprint.** Singles only is Easy. Locked
   candidates is Medium. Pairs is Hard. A puzzle the ladder can't finish is
   Expert.
5. **Generate to the target.** The generator removes clues one at a time,
   keeping a removal only if the puzzle stays uniquely solvable *and* its band
   stays at or below the one you asked for.

The same ladder powers the Strategies drawer. The techniques that rated the
puzzle are the ones it teaches, so the difficulty label and the lessons always
agree.

Today's bands read *which* techniques a solve needed. The histogram is
recorded so that, as higher techniques join the ladder (X-wing, XY-wing, ...),
the bands can move to thresholds on *how much* of the solve leaned on them.
The engine is dependency-free C++17 (`include/model`, `src/model`); the
policy is `DifficultyProfile::band()` in `src/model/Rater.cpp`.

## Build

Fedora:

```bash
sudo dnf install gtkmm4.0-devel spdlog-devel json-devel glib2-devel cmake gcc-c++
cmake -B build
cmake --build build
./build/sudoku
```

Or run `./build.sh`, which installs the packages (dnf or apt) and builds.

The solving engine (`include/model`, `src/model`) is standard C++17 with no
dependencies. `make run` builds and runs its selftest with nothing but a
compiler.

## Install

```bash
cmake --install build --prefix ~/.local
```

installs the binary, the `.desktop` launcher, the icon, and the AppStream
metadata into the XDG data directories GNOME already searches.

## Application ID

`io.github.scott8420.Sudoku`, set once as `SUDOKU_APP_ID` in
`CMakeLists.txt` and handed to the code as a compile definition. The resource
filenames follow it; `resources/sudoku.gresource.xml` is the one file that
spells it out by hand.

## License

MIT -- see [LICENSE](LICENSE).
