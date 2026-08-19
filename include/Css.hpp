#pragma once

namespace sudoku {

// Minimal application stylesheet, installed once at startup.
//
// The pattern here is lifted from Folio's focus-mode drawer (which itself
// "replaces the hover pill" — hover-driven reveal was tried and abandoned).
// The key move: the left drawer tab is made invisible-until-hover entirely in
// CSS (opacity 0 with a :hover override and a transition), NOT by toggling
// opacity/visibility from C++. The widget is always present and always
// clickable; CSS just fades it in on hover. This is the reliable idiom.
//
// This is also the seed of the s5 skin work — when the Theme JSON loader lands,
// generated rules can be appended to a sheet like this one.
inline const char* SUDOKU_CSS = R"CSS(
    /* The always-present pull tab on the left edge — quiet until hovered. */
    .drawer-tab {
        background-color: rgba(36, 39, 46, 0.72);
        color: rgba(150, 155, 168, 1);
        font-size: 16px;
        border: 1px solid rgba(90, 95, 108, 0.5);
        border-left: none;
        border-radius: 0 10px 10px 0;
        min-width: 22px;
        min-height: 200px;
        padding: 0;
        box-shadow: 2px 0 8px rgba(0, 0, 0, 0.30);
        opacity: 0;
        transition: opacity 160ms ease;
    }
    .drawer-tab:hover {
        background-color: rgba(58, 62, 72, 1);
        color: rgba(232, 234, 238, 1);
        opacity: 1;
    }

    /* The right-edge twin (Stats). Mirror of .drawer-tab: the open border and
       radius are on the LEFT edge, the shadow throws left. Same quiet-until-
       hover behaviour, same CSS-only reveal. */
    .drawer-tab-right {
        background-color: rgba(36, 39, 46, 0.72);
        color: rgba(150, 155, 168, 1);
        font-size: 16px;
        border: 1px solid rgba(90, 95, 108, 0.5);
        border-right: none;
        border-radius: 10px 0 0 10px;
        min-width: 22px;
        min-height: 200px;
        padding: 0;
        box-shadow: -2px 0 8px rgba(0, 0, 0, 0.30);
        opacity: 0;
        transition: opacity 160ms ease;
    }
    .drawer-tab-right:hover {
        background-color: rgba(58, 62, 72, 1);
        color: rgba(232, 234, 238, 1);
        opacity: 1;
    }

    /* Transparent click-away catcher; present only while a drawer is open. */
    #drawer-scrim { background-color: transparent; }

    /* Notes mode is a warm cue. The Notes ToggleButton (Named "mode_notes")
       glows orange while active and reverts to the normal button when off --
       done entirely via the :checked pseudo, no C++ class toggling. Pairs with
       the orange selection wash the Board paints in Notes mode. */
    #mode_notes:checked {
        background-image: none;
        background-color: rgba(208, 92, 28, 0.97);
        color: rgba(255, 248, 240, 1);
        font-weight: bold;
    }
    #mode_notes:checked:hover {
        background-color: rgba(224, 106, 38, 1);
    }
)CSS";

}  // namespace sudoku
