#pragma once

// sudoku::util — home for pumps (CANON: "Pumps at conceptual seams").
//
// A pump is a small abstraction at a seam where a concept lives, usually a
// bidirectional pair (encode/decode, parse/format) kept in one place so a
// change to one half forces seeing the other. This namespace is EMPTY on day
// one by design — it fills as the "same 5–10 lines in three places" signal
// fires. The first expected pump is the save/load codec, held to round-trip
// fidelity; it lands with persistence, not before.
namespace sudoku::util {}  // intentionally empty for now
