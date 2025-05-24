#pragma once

#include "Common.hpp" // For Direction (might be needed later)
#include <string>
#include <memory>    // For std::unique_ptr
#include <vector>
#include <utility>   // For std::pair
#include <optional>  // For std::optional

// Forward declaration to avoid circular includes
class Buffer;

// --- Base Change Class ---

// Abstract base class for undo/redo actions
class Change {
public:
    virtual ~Change() = default; // Essential virtual destructor

    // Apply the change to the buffer
    virtual void apply(Buffer& buffer) = 0;

    // Undo the change applied to the buffer
    virtual void undo(Buffer& buffer) = 0;

    // Optional: Method to potentially merge consecutive changes
    // virtual bool merge(const Change& nextChange) { return false; }
};

// --- Concrete Change Types ---

// Represents inserting text
class InsertChange : public Change {
private:
    int m_row, m_col;      // Position *before* insertion
    std::string m_text;    // The text that was inserted
    // Store if selection existed before insert? (For more complex undo)

public:
    InsertChange(int row, int col, std::string text)
        : m_row(row), m_col(col), m_text(std::move(text)) {}

    void apply(Buffer& buffer) override;
    void undo(Buffer& buffer) override;
    const std::string& getText() const { return m_text; } // Maybe useful
};

// Represents deleting text
class DeleteChange : public Change {
private:
    int m_row, m_col;          // Position *before* deletion started
    std::string m_deletedText; // The text that was deleted
    // Store selection state?

public:
    DeleteChange(int row, int col, std::string deletedText)
        : m_row(row), m_col(col), m_deletedText(std::move(deletedText)) {}

    void apply(Buffer& buffer) override;
    void undo(Buffer& buffer) override;
    const std::string& getDeletedText() const { return m_deletedText; }
};

// Represents cursor movement (could be combined with selection changes)
class MoveChange : public Change {
private:
    int m_fromRow, m_fromCol; // Position *before* move
    int m_toRow, m_toCol;     // Position *after* move
    // Store selection state before/after?

public:
    MoveChange(int fromRow, int fromCol, int toRow, int toCol)
        : m_fromRow(fromRow), m_fromCol(fromCol), m_toRow(toRow), m_toCol(toCol) {}

    void apply(Buffer& buffer) override;
    void undo(Buffer& buffer) override;
};

// Represents starting or modifying a selection anchor
// Note: Simple version - assumes cursor movement handles the other end.
class SelectChange : public Change {
private:
    std::optional<std::pair<int, int>> m_oldSelectionStart; // State *before* change
    std::pair<int, int> m_newSelectionStart;                // State *after* change (where anchor was set)

public:
    SelectChange(std::optional<std::pair<int, int>> oldStart, std::pair<int, int> newStart)
        : m_oldSelectionStart(oldStart), m_newSelectionStart(newStart) {}

    void apply(Buffer& buffer) override;
    void undo(Buffer& buffer) override;
};

// Represents clearing a selection
class UnselectChange : public Change {
private:
    std::pair<int, int> m_oldSelectionStart; // Anchor position *before* clearing
    // Store cursor position too? Maybe not needed if MoveChange handles it.

public:
    explicit UnselectChange(std::pair<int, int> oldStart)
        : m_oldSelectionStart(oldStart) {}

    void apply(Buffer& buffer) override;
    void undo(Buffer& buffer) override;
};