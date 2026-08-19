// main.cpp — Sudoku entry point.
//
// Thin by design: construct the application and run it. Logging, the window,
// and the model all live behind Application::create(). See ../docs/ARC.md for
// the current arc and ../docs/ARCHITECTURE.md for the seams.

#include "Application.hpp"

int main(int argc, char* argv[]) {
    auto app = sudoku::Application::create();
    return app->run(argc, argv);
}
