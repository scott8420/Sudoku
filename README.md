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

`SUDOKU_APP_ID` in `CMakeLists.txt` is the placeholder
`io.github.example.Sudoku` — change it to your real reverse-DNS id
before any packaging work.
