# Sudoku

A modern Sudoku game for GNOME, built on GTK4 / gtkmm-4.0.

## Status

**s0 — bootstrap.** This is a bare gtkmm-4.0 CMake stub: it configures,
builds, and opens an empty window. No game logic yet. The project docs
(what it is, what it's doing, what we believe, session handoffs) live in
the sibling `../docs/` repo — start there.

## Build

```bash
./build.sh          # installs deps (Debian/Ubuntu) then builds
# or, manually:
cmake -B build
cmake --build build
./build/sudoku
```

Requires `gtkmm-4.0` development headers (`libgtkmm-4.0-dev` on
Debian/Ubuntu), `cmake` (>= 3.20), and `pkg-config`.

## Application ID

`io.github.scott8420.Sudoku`, set once as `SUDOKU_APP_ID` in
`CMakeLists.txt`. The build hands it to the code as a compile
definition (`SUDOKU_APP_ID`, plus the slash form `SUDOKU_APP_PATH`),
and the resource filenames follow the variable, so the id is never
retyped in C++. The single exception is
`resources/sudoku.gresource.xml`, which `glib-compile-resources` reads
literally — change that file and the three files beside it together if
the id ever moves.
