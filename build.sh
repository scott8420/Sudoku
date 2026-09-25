#!/usr/bin/env bash
# Sudoku build script.
#
# Usage:  ./build.sh
#
# Configures and builds the app (./build/sudoku) and the engine selftest
# (./build/sudoku_selftest).
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ── Install dependencies — comment out if already present ─────────────────────
if command -v dnf &>/dev/null; then            # Fedora
    sudo dnf install -y \
        gtkmm4.0-devel \
        spdlog-devel \
        json-devel \
        glib2-devel \
        gcc-c++ \
        cmake
elif command -v apt-get &>/dev/null; then      # Debian / Ubuntu
    sudo apt-get install -y \
        libgtkmm-4.0-dev \
        libspdlog-dev \
        nlohmann-json3-dev \
        libglib2.0-dev-bin \
        pkg-config \
        cmake
fi

# ── Configure + build ─────────────────────────────────────────────────────────
cmake -B build
cmake --build build -- -j"$(nproc)"

echo ""
echo "Built: ./build/sudoku  and  ./build/sudoku_selftest"
