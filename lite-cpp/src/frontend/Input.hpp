#pragma once

#include <variant> // For potential future complex input types
#include <cstdint> // For uint8_t

// Represents specific key codes, including non-printable ones
enum class KeyCode {
    None,      // Represents no key or unhandled input
    Char,      // Represents a printable character (stored separately)
    Enter,
    Tab,
    Backspace,
    Delete,
    Escape,
    Up,
    Down,
    Left,
    Right,
    Home,
    End,
    PageUp,
    PageDown
    // Add other special keys if needed (F1-F12, Insert, etc.)
};

// Bit flags for modifier keys
enum class KeyModifier : uint8_t {
    None    = 0,
    Shift   = 1 << 0, // 1
    Control = 1 << 1, // 2
    Alt     = 1 << 2  // 4
    // Meta/Super could be added if detectable
};

// --- Bitwise operators for KeyModifier flags ---
// Allows combining modifiers like: KeyModifier::Control | KeyModifier::Alt

inline KeyModifier operator|(KeyModifier lhs, KeyModifier rhs) {
    using T = std::underlying_type_t<KeyModifier>;
    return static_cast<KeyModifier>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline KeyModifier& operator|=(KeyModifier& lhs, KeyModifier rhs) {
    lhs = lhs | rhs;
    return lhs;
}

inline KeyModifier operator&(KeyModifier lhs, KeyModifier rhs) {
    using T = std::underlying_type_t<KeyModifier>;
    return static_cast<KeyModifier>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

// Check if a specific flag is set
inline bool hasModifier(KeyModifier flags, KeyModifier check) {
    // Check if all bits in 'check' are set in 'flags'
    // Note: 'check' should typically be a single flag (None, Shift, Control, Alt)
     using T = std::underlying_type_t<KeyModifier>;
     return (static_cast<T>(flags) & static_cast<T>(check)) != 0;
}


// --- Input Event Structure ---

// Holds the details of a single input event from the user
struct InputEvent {
    KeyCode code = KeyCode::None;
    char character = '\0'; // Valid only if code is KeyCode::Char
    KeyModifier modifiers = KeyModifier::None;

    // Default comparison operator
    bool operator==(const InputEvent& other) const = default;
};