#pragma once // Use include guards to prevent multiple inclusions

// --- Enumerations ---

// Defines the possible directions for cursor movement or other directional actions.
enum class Direction {
    Up,
    Down,
    Left,
    Right,
    Nowhere // Represents no direction or a stationary state
};

// --- Type Aliases (Optional) ---
// You could define common type aliases here if needed, for example:
// using RowNum = int;
// using ColNum = int;
// using CursorPos = std::pair<RowNum, ColNum>;

// --- Constants (Optional) ---
// Define any global constants relevant across modules, e.g.:
// constexpr int DEFAULT_TAB_WIDTH = 4;


// --- Utility Functions (Use Sparingly) ---
// Generally avoid putting complex functions here. Keep it for simple,
// widely used definitions. If you need utility functions, consider a
// separate `Utils.hpp/.cpp`.