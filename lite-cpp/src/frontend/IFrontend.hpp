#pragma once

#include "Input.hpp" // Requires InputEvent definition
#include <string>
#include <vector>
#include <optional>

// Forward declaration to avoid including the full Editor header here
class Editor;

class IFrontend {
public:
    // Virtual destructor is crucial for classes designed for inheritance
    virtual ~IFrontend() = default;

    // --- Lifecycle ---
    // Initialize the frontend (e.g., set up terminal)
    virtual void initialize() = 0;
    // Shutdown the frontend (e.g., restore terminal)
    virtual void shutdown() = 0;

    // --- Core Interaction ---
    // Render the current editor state to the screen
    virtual void render(const Editor& editor) = 0;
    // Block and wait for the next user input event
    virtual InputEvent waitForInput(const Editor& editor) = 0;
    // Set the text displayed in the status bar
    virtual void setStatus(const std::string& status) = 0;

    // --- User Prompts ---
    // Ask the user for text input, optionally with a prefilled value
    // Returns nullopt if the user cancels (e.g., presses Esc)
    virtual std::optional<std::string> prompt(const std::string& promptText,
                                              const std::optional<std::string>& prefill = std::nullopt) = 0;
    // Ask the user a yes/no question
    // Returns nullopt if the user cancels
    virtual std::optional<bool> ask(const std::string& promptText,
                                     const std::string& yesOption = "y",
                                     const std::string& noOption = "n") = 0;
    // Optional advanced prompts (implement later if needed):
    // virtual std::optional<std::string> choose(const std::string& prompt, const std::vector<std::string>& options) = 0;
    // virtual std::optional<long> getNum(const std::string& prompt) = 0;

    // --- Information ---
    // Get the usable height of the editor area (excluding status bar, etc.)
    virtual int getHeight() const = 0;
    // Get the usable width of the editor area
    virtual int getWidth() const = 0;
};