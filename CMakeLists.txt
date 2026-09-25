cmake_minimum_required(VERSION 3.20)

# NOTE (s0): the CANON/Curvz "default clang, yield to CC/CXX/cache" compiler-
# selection block is intentionally omitted from this project so it configures
# with whatever compiler is present. Add it if/when it earns its place.

project(sudoku VERSION 0.1.0 LANGUAGES C CXX)

# ── Application identity ──────────────────────────────────────────────────────
# Single source of truth for the freedesktop application ID, derived from the
# project's GitHub home (github.com/scott8420/Sudoku).
#
# It used to be a source of truth in name only: the same string was also typed
# out in Application.cpp twice and in MainWindow.cpp once, with a comment asking
# the next editor to keep them in step. Now the C++ side gets it as a compile
# definition, so a change here reaches the code by rebuilding rather than by
# remembering.
#
# SUDOKU_APP_PATH is the GResource/GApplication path form -- the same id with
# the dots turned into slashes. Derived rather than written out, for the same
# reason.
set(SUDOKU_APP_ID "io.github.scott8420.Sudoku")
string(REPLACE "." "/" SUDOKU_APP_PATH "/${SUDOKU_APP_ID}")

# The resource FILE names (resources/${SUDOKU_APP_ID}.svg, .desktop) follow this
# variable automatically below. The one place the id is still spelled by hand is
# resources/sudoku.gresource.xml, whose <gresource prefix> and <file> entries
# glib-compile-resources reads literally -- rename those with the files.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ── Dependencies ──────────────────────────────────────────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-4.0)
find_package(spdlog REQUIRED)   # UI-side logging only; the model stays pure

# nlohmann/json — UI-side only (skin files). Try the system package, fall back
# to FetchContent. The pure engine never sees it.
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# ── The pure engine (sudoku::model) ───────────────────────────────────────────
# A standalone library that links NOTHING but the C++ standard library — no
# gtkmm, no glib, no spdlog. The GTK-free seam is visible in the build graph:
# a stray framework include under model/ stops this target compiling. The UI
# links it; a student can lift include/model + src/model and build it with the
# bare Makefile instead.
add_library(sudoku_model STATIC
    src/model/Grid.cpp
    src/model/WorkGrid.cpp
    src/model/Techniques.cpp
    src/model/Solver.cpp
    src/model/Rater.cpp
    src/model/Generator.cpp
)
target_include_directories(sudoku_model PUBLIC include)

# Pure-C++ engine selftest (also links the GTK-free shortcut registry so the
# accel-collision check rides along):
#   cmake --build build --target sudoku_selftest && ./build/sudoku_selftest
add_executable(sudoku_selftest src/model/selftest.cpp src/Shortcuts.cpp)
target_include_directories(sudoku_selftest PRIVATE include)
target_link_libraries(sudoku_selftest PRIVATE sudoku_model)

# ── App resources (icon bundled via GResource) ────────────────────────────────
# glib-compile-resources turns resources/sudoku.gresource.xml into a C source
# that embeds the icon and auto-registers it at load (static constructor). The
# app then adds "<app-id-path>/icons" to the icon theme's resource path, so the
# themed icon resolves by name for the window and the About-dialog logo.
find_program(GLIB_COMPILE_RESOURCES NAMES glib-compile-resources REQUIRED)
set(SUDOKU_GRESOURCE_XML ${CMAKE_CURRENT_SOURCE_DIR}/resources/sudoku.gresource.xml)
set(SUDOKU_RESOURCES_C   ${CMAKE_CURRENT_BINARY_DIR}/sudoku_resources.c)
add_custom_command(
    OUTPUT  ${SUDOKU_RESOURCES_C}
    COMMAND ${GLIB_COMPILE_RESOURCES}
            --target=${SUDOKU_RESOURCES_C} --generate-source
            --sourcedir=${CMAKE_CURRENT_SOURCE_DIR}/resources ${SUDOKU_GRESOURCE_XML}
    DEPENDS ${SUDOKU_GRESOURCE_XML}
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/${SUDOKU_APP_ID}.svg
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/${SUDOKU_APP_ID}-symbolic.svg
    COMMENT "Compiling GResources"
    VERBATIM)

# ── The app ───────────────────────────────────────────────────────────────────
add_executable(sudoku
    ${SUDOKU_RESOURCES_C}
    src/main.cpp
    src/Application.cpp
    src/MainWindow.cpp
    src/Board.cpp
    src/StrategyPanel.cpp
    src/StatsPanel.cpp
    src/ShortcutsDialog.cpp
    src/PreferencesDialog.cpp
    src/Skins.cpp
    src/Stats.cpp
    src/Shortcuts.cpp
    src/Teach.cpp
    src/Log.cpp
    src/WidgetRegistry.cpp
)
target_include_directories(sudoku PRIVATE include ${GTKMM_INCLUDE_DIRS})
# The identity above (and the version from project()), handed to the code. Only the app target gets these: the
# pure engine and the selftest have no business knowing the app has an id.
target_compile_definitions(sudoku PRIVATE
    SUDOKU_APP_ID="${SUDOKU_APP_ID}"
    SUDOKU_APP_PATH="${SUDOKU_APP_PATH}"
    SUDOKU_VERSION="${PROJECT_VERSION}")
target_link_libraries(sudoku    PRIVATE ${GTKMM_LIBRARIES} sudoku_model spdlog::spdlog nlohmann_json::nlohmann_json)
target_compile_options(sudoku   PRIVATE ${GTKMM_CFLAGS_OTHER})

# ── Install (so the app launches from the desktop) ────────────────────────────
# For a user-local launcher without root:
#   cmake --install build --prefix ~/.local
# installs the binary, the .desktop, and the icon into the XDG data dirs GNOME
# already searches. Running ./build/sudoku directly still works uninstalled
# (the in-app icon comes from the bundled GResource); install is only needed for
# the app-grid entry. Names derive from SUDOKU_APP_ID.
include(GNUInstallDirs)
install(TARGETS sudoku RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES resources/${SUDOKU_APP_ID}.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
# The launcher/app-grid icon must be a NON-symbolic app icon under this exact
# name in scalable/apps; the -symbolic variant is in-app only (About logo, from
# the gresource) and is deliberately NOT installed here.
install(FILES resources/${SUDOKU_APP_ID}.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
# AppStream metadata -- what GNOME Software / Flathub read for the store page.
install(FILES resources/${SUDOKU_APP_ID}.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
