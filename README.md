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
