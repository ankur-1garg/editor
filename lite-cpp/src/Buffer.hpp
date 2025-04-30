#pragma once

#include "Common.hpp"
#include "Change.hpp" // Include Change base class

#include <vector>
#include <string>
#include <optional>
#include <filesystem> // Requires C++17
#include <memory>     // For unique_ptr in undo/redo stack
#include <utility>    // For std::pair

class Buffer {
private:
    std::optional<std::filesystem::path> m_filePath;
    std::vector<std::string> m_lines;
    int m_cursorRow = 0;
    int m_cursorCol = 0;
    std::optional<std::pair<int, int>> m_selectionStart; // Anchor (Row, Col)
    bool m_isEdited = false;

    std::vector<std::unique_ptr<Change>> m_undoStack;
    std::vector<std::unique_ptr<Change>> m_redoStack;

    // --- Internal Helpers ---
    // Allow Change classes to call internal state modifiers directly
    friend class InsertChange;
    friend class DeleteChange;
    friend class MoveChange;
    friend class SelectChange;
    friend class UnselectChange;

    // Modifies text/cursor state WITHOUT creating undo entries
    void insertTextInternal(const std::string& text);
    void deleteTextInternal(int row, int col, size_t length);
    void setSelectionStartInternal(std::pair<int, int> startPos);
    void clearSelectionInternal();

    void pushUndo(std::unique_ptr<Change> change);
    void fixCursor(); // Ensure cursor row/col are within valid bounds
    // Calculates the actual (start_row, start_col), (end_row, end_col) range
    std::optional<std::pair<std::pair<int, int>, std::pair<int, int>>> calculateSelectionRange() const;

public:
    Buffer(); // Default constructor for empty buffer
    explicit Buffer(const std::filesystem::path& filePath); // Constructor from file
    ~Buffer() = default; // Default destructor is fine

    // --- Getters ---
    std::optional<std::filesystem::path> getFilePath() const;
    bool isEdited() const;
    int getCursorRow() const;
    int getCursorCol() const;
    std::pair<int, int> getCursorPosition() const; // Convenience
    int getLineCount() const;
    const std::string& getLine(int row) const; // Be careful with bounds
    std::vector<std::string> getLines(int startRow, int endRow) const; // Get range
    std::optional<std::string> getSelectedText() const;
    // Returns ((startRow, startCol), (endRow, endCol)) or nullopt
    std::optional<std::pair<std::pair<int, int>, std::pair<int, int>>> getSelectionRange() const;
    bool hasSelection() const;

    // --- Setters / Modifiers ---
    void setFilePath(const std::filesystem::path& path);
    // These return true on success, false on failure (e.g., IO error)
    bool save(); // Save to existing path, returns false if path not set
    bool saveAs(const std::filesystem::path& path); // Save to specific path
    void setCursorPosition(int row, int col); // Public setter, creates MoveChange if needed

    // --- Editing Operations (Public API - Create Undo Entries) ---
    void insertChar(char c);
    void insertNewline();
    void insertString(const std::string& text); // Handles newlines within text
    void deleteCharForward(); // Delete char under/after cursor (Delete key)
    void deleteCharBackward(); // Delete char before cursor (Backspace key)
    void deleteSelection();    // Deletes the currently selected text range

    // Moves cursor; if selecting is true, potentially starts/updates selection
    void moveCursor(Direction dir, bool selecting = false);

    // Explicit selection control
    void selectStart();   // Sets the selection anchor at the current cursor pos
    void unselect();      // Clears the selection anchor

    // --- Undo/Redo ---
    void undo();
    void redo();
};