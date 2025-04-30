#pragma once

#include "IFrontend.hpp" // Inherits from the interface
#include "Input.hpp"     // Uses InputEvent
#include <string>
#include <vector>       // For highlight rules maybe

// Forward declaration
class Buffer;

// ncurses implementation of the Frontend interface
class Terminal : public IFrontend {
private:
    int m_totalRows = 0;       // Total rows reported by ncurses
    int m_totalCols = 0;       // Total columns reported by ncurses
    int m_editorRows = 0;      // Usable rows for buffer display (total - status bar)
    int m_displayStartRow = 0; // First buffer row index to display (for scrolling)
    std::string m_statusMessage;
    bool m_isInitialized = false;

    // --- Private Helper Methods ---
    void drawBuffer(const Buffer* buffer); // Draws the text buffer content
    void drawStatusBar();                  // Draws the bottom status line
    void drawLineNumber(int screenRow, int bufferRow, int width); // Draws line number
    // Draws a single line with syntax highlighting
    void printLineWithHighlighting(int screenRow, int startCol, const std::string& line);
    void handleResize();                   // Handle terminal resize events
    InputEvent mapNcursesKey(int ncurses_char); // Translate ncurses int to InputEvent

    // Syntax highlighting helpers (basic example)
    enum class SyntaxToken { Keyword, Type, Operator, Comment, String, Number, Normal };
    SyntaxToken getTokenType(const std::string& token);
    void applyColor(SyntaxToken tokenType);
    void resetColor();

public:
    Terminal() = default;
    ~Terminal() override; // Ensure shutdown is called via RAII

    // --- Lifecycle (from IFrontend) ---
    void initialize() override;
    void shutdown() override;

    // --- Core Interaction (from IFrontend) ---
    void render(const Editor& editor) override;
    InputEvent waitForInput(const Editor& editor) override;
    void setStatus(const std::string& status) override;

    // --- User Prompts (from IFrontend) ---
    std::optional<std::string> prompt(const std::string& promptText,
                                      const std::optional<std::string>& prefill = std::nullopt) override;
    std::optional<bool> ask(const std::string& promptText,
                             const std::string& yesOption = "y",
                             const std::string& noOption = "n") override;

    // --- Information (from IFrontend) ---
    int getHeight() const override; // Returns m_editorRows
    int getWidth() const override;  // Returns m_totalCols
};