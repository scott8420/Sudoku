#pragma once
#include <spdlog/spdlog.h>
#include <memory>

// Per-area logging (CANON: "Per-method logging areas with runtime toggle").
//
// Every log line belongs to an AREA named by concern, not by file. When
// something misbehaves the move is: turn the suspect area to TRACE, reproduce,
// read the trace — which is also the two-fails-drop-to-log-mode substrate from
// RULES. Areas map to spdlog named loggers sharing one file sink.
//
// This header is UI-side. sudoku::model is pure and never includes it; that is
// the seam that keeps the engine liftable. Logging lives with the GTK code
// that has a process to attach a log file to.
namespace sudoku::log {

enum class Area { App, Model, Board, Input, Io, Render };

// Create the shared file sink (in the user data dir) and register one logger
// per area. Idempotent — safe to call once at Application startup.
void init();

// The logger for an area (nullptr only if init() hasn't run yet — callers in
// early construction paths should null-check).
std::shared_ptr<spdlog::logger> get(Area area);

// Runtime level for one area. The UI can wire this to a debug menu later; for
// now it's the programmatic hook diagnosis uses.
void set_level(Area area, spdlog::level::level_enum level);

}  // namespace sudoku::log
