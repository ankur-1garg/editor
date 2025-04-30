#include "Buffer.hpp"
#include "Change.hpp" // Include derived Change types too

#include <fstream>
#include <stdexcept> // For exceptions
#include <algorithm> // For std::min/max
#include <utility>   // For std::move
#include <iostream>  // For debug (optional)

// --- Constructor Implementation ---

Buffer::Buffer() : m_lines(1, "") { // Start with one empty line
    // std::cout << "Buffer default constructor\n"; // Debug
}

Buffer::Buffer(const std::filesystem::path& filePath) : m_filePath(filePath) {
    // std::cout << "Buffer file constructor: " << filePath << "\n"; // Debug
    std::ifstream file(filePath);
    if (file) {
        std::string line;
        while (std::getline(file, line)) {
            // Handle potential Windows CRLF -> LF conversion
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            m_lines.push_back(std::move(line));
        }
        file.close(); // Good practice
    } else {
        // File doesn't exist or couldn't be opened, treat as new empty file
        m_filePath = std::nullopt; // Clear path if loading failed? Or keep it? Decide policy.
                                   // Let's keep it for potential saving later.
        std::cerr << "Warning: Could not open file " << filePath << ". Starting empty buffer.\n";
    }

    // Ensure buffer is never truly empty (at least one line, even if empty)
    if (m_lines.empty()) {
        m_lines.emplace_back("");
    }
    fixCursor(); // Ensure cursor is valid initially
}

// --- Getters Implementation ---

std::optional<std::filesystem::path> Buffer::getFilePath() const { return m_filePath; }
bool Buffer::isEdited() const { return m_isEdited; }
int Buffer::getCursorRow() const { return m_cursorRow; }
int Buffer::getCursorCol() const { return m_cursorCol; }
std::pair<int, int> Buffer::getCursorPosition() const { return {m_cursorRow, m_cursorCol}; }
int Buffer::getLineCount() const { return static_cast<int>(m_lines.size()); }
bool Buffer::hasSelection() const { return m_selectionStart.has_value(); }

const std::string& Buffer::getLine(int row) const {
    // Provide read-only access, with bounds checking
    if (row >= 0 && row < m_lines.size()) {
        return m_lines[row];
    } else {
        // Return reference to a static empty string to avoid dangling references
        // or throwing an exception. Throwing might be safer.
        static const std::string empty_line = "";
        // Consider: throw std::out_of_range("Buffer::getLine - Invalid row index");
        return empty_line; // Use with caution
    }
}

std::vector<std::string> Buffer::getLines(int startRow, int endRow) const {
    std::vector<std::string> result;
    int clampedStart = std::max(0, startRow);
    int clampedEnd = std::min(getLineCount(), endRow); // endRow is exclusive

    if (clampedStart < clampedEnd) {
        result.reserve(clampedEnd - clampedStart);
        for (int i = clampedStart; i < clampedEnd; ++i) {
            result.push_back(m_lines[i]); // Copy lines
        }
    }
    return result;
}

std::optional<std::pair<std::pair<int, int>, std::pair<int, int>>> Buffer::calculateSelectionRange() const {
    if (!m_selectionStart) {
        return std::nullopt;
    }

    std::pair<int, int> anchor = *m_selectionStart;
    std::pair<int, int> cursor = {m_cursorRow, m_cursorCol};

    // Determine which point comes first textually
    if (anchor.first < cursor.first || (anchor.first == cursor.first && anchor.second < cursor.second)) {
        // Anchor is start, cursor is end
        return std::make_pair(anchor, cursor);
    } else {
        // Cursor is start, anchor is end
        return std::make_pair(cursor, anchor);
    }
}

std::optional<std::pair<std::pair<int, int>, std::pair<int, int>>> Buffer::getSelectionRange() const {
    return calculateSelectionRange();
}

std::optional<std::string> Buffer::getSelectedText() const {
    auto rangeOpt = calculateSelectionRange();
    if (!rangeOpt) {
        return std::nullopt;
    }

    auto [startPos, endPos] = *rangeOpt;
    int startRow = startPos.first;
    int startCol = startPos.second;
    int endRow = endPos.first;
    int endCol = endPos.second;

    std::string selected;

    if (startRow == endRow) {
        // Single line selection
        if (startRow >= 0 && startRow < m_lines.size() && startCol < endCol) {
             size_t len = static_cast<size_t>(m_lines[startRow].length());
             int clampedStartCol = std::min(startCol, (int)len);
             int clampedEndCol = std::min(endCol, (int)len);
             if (clampedStartCol < clampedEndCol) {
                  selected = m_lines[startRow].substr(clampedStartCol, clampedEndCol - clampedStartCol);
             }
        }
    } else {
        // Multi-line selection
        if (startRow >= 0 && startRow < m_lines.size()) {
            // First line part
             size_t startLineLen = static_cast<size_t>(m_lines[startRow].length());
             int clampedStartCol = std::min(startCol, (int)startLineLen);
            selected += m_lines[startRow].substr(clampedStartCol);
        }
        // Intermediate full lines
        for (int i = startRow + 1; i < endRow; ++i) {
            if (i >= 0 && i < m_lines.size()) {
                selected += '\n';
                selected += m_lines[i];
            }
        }
        // Last line part
        if (endRow >= 0 && endRow < m_lines.size() && endCol > 0) {
             size_t endLineLen = static_cast<size_t>(m_lines[endRow].length());
             int clampedEndCol = std::min(endCol, (int)endLineLen);
             selected += '\n';
             selected += m_lines[endRow].substr(0, clampedEndCol);
        }
    }

    return selected;
}


// --- Setters / Modifiers Implementation ---

void Buffer::setFilePath(const std::filesystem::path& path) {
    // TODO: Should this create an undo action? Probably not.
    m_filePath = path;
}

bool Buffer::saveAs(const std::filesystem::path& path) {
    std::ofstream file(path);
    if (!file) {
        std::cerr << "Error: Could not open file for writing: " << path << std::endl;
        return false;
    }
    for (size_t i = 0; i < m_lines.size(); ++i) {
        file << m_lines[i];
        // Write newline for all lines except the very last one IF it's empty
        // Or always write newline except after the absolute last line?
        // Standard behaviour is usually newline after every line written.
        if (i < m_lines.size() - 1) {
             file << '\n';
        }
        // Decide if the last line should have a newline if it's not empty.
        // Most editors add one implicitly if missing on save. Let's do that.
        // This logic needs refinement based on desired POSIX compliance etc.
        // Simplified: Add newline if it's not the last line.
    }

    if (file.good()) {
        file.close();
        m_filePath = path;
        m_isEdited = false; // Saved successfully
        // Clear undo/redo stack on save? Some editors do, some don't. Let's not clear it.
        return true;
    } else {
         std::cerr << "Error: Failed to write to file: " << path << std::endl;
         file.close(); // Close even on failure
         return false;
    }
}

bool Buffer::save() {
    if (!m_filePath) {
        // Cannot save if no path is associated
        return false;
    }
    return saveAs(*m_filePath);
}

void Buffer::setCursorPosition(int row, int col) {
    // Public setter - might create an undo entry if position changes
    int oldRow = m_cursorRow;
    int oldCol = m_cursorCol;

    m_cursorRow = row;
    m_cursorCol = col;
    fixCursor(); // Ensure the requested position is valid

    // Only create move change if position actually changed
    if (m_cursorRow != oldRow || m_cursorCol != oldCol) {
        // Don't record move if selection is active? Or make Move handle selection?
        // Simple approach: always record the move.
        pushUndo(std::make_unique<MoveChange>(oldRow, oldCol, m_cursorRow, m_cursorCol));
    }
}

// --- Internal State Modifiers (No Undo) ---

void Buffer::insertTextInternal(const std::string& text) {
    if (text.empty()) return;

    size_t start = 0;
    size_t newlinePos;

    while ((newlinePos = text.find('\n', start)) != std::string::npos) {
        // Insert part before newline
        std::string part = text.substr(start, newlinePos - start);
        if (!part.empty()) {
            if (m_cursorRow >= 0 && m_cursorRow < m_lines.size() && m_cursorCol >=0) {
                 size_t lineLen = m_lines[m_cursorRow].length();
                 int clampedCol = std::min(m_cursorCol, (int)lineLen);
                 m_lines[m_cursorRow].insert(clampedCol, part);
                 m_cursorCol = clampedCol + part.length(); // Update cursor column
            }
        }

        // Handle the newline itself (split line)
        if (m_cursorRow >= 0 && m_cursorRow < m_lines.size()) {
             std::string& currentLine = m_lines[m_cursorRow];
             size_t lineLen = currentLine.length();
             int clampedCol = std::min(m_cursorCol, (int)lineLen);
             std::string nextLineContent = "";
             if (clampedCol < lineLen) {
                 nextLineContent = currentLine.substr(clampedCol);
                 currentLine.erase(clampedCol);
             }
             // Insert new line *after* current row
             m_lines.insert(m_lines.begin() + m_cursorRow + 1, std::move(nextLineContent));
             m_cursorRow++;
             m_cursorCol = 0;
        }
        start = newlinePos + 1;
    }

    // Insert remaining part after the last newline (or the whole string if no newlines)
    std::string remainingPart = text.substr(start);
    if (!remainingPart.empty()) {
        if (m_cursorRow >= 0 && m_cursorRow < m_lines.size() && m_cursorCol >= 0) {
            size_t lineLen = m_lines[m_cursorRow].length();
            int clampedCol = std::min(m_cursorCol, (int)lineLen);
            m_lines[m_cursorRow].insert(clampedCol, remainingPart);
            m_cursorCol = clampedCol + remainingPart.length();
        }
    }
    fixCursor(); // Ensure cursor validity after all insertions
}

void Buffer::deleteTextInternal(int row, int col, size_t length) {
    // Internal deletion used by undo/redo - assumes valid input somewhat
    // Needs to handle multi-line deletion correctly.
    if (length == 0) return;

    // Simple approach: Call internal delete-forward 'length' times
    // This is INEFFICIENT for large lengths but easier to implement correctly
    // regarding line joining than complex range calculation.
    setCursorPosition(row, col); // Move cursor to start WITHOUT recording undo move
    int startRow = m_cursorRow;
    int startCol = m_cursorCol;

    for (size_t i = 0; i < length; ++i) {
        // Check if we are at the end of the buffer
        if (m_cursorRow >= m_lines.size() - 1 && m_cursorCol >= m_lines[m_cursorRow].length()) {
             break; // Cannot delete further
        }

        if (m_cursorCol < m_lines[m_cursorRow].length()) {
            // Delete character within the line
            m_lines[m_cursorRow].erase(m_cursorCol, 1);
        } else if (m_cursorRow < m_lines.size() - 1) {
            // Join with the next line
            std::string nextLine = std::move(m_lines[m_cursorRow + 1]);
            m_lines.erase(m_lines.begin() + m_cursorRow + 1);
            m_lines[m_cursorRow] += nextLine;
        }
        // Don't increment cursor - deleting forward keeps cursor at the start
         fixCursor(); // Ensure cursor is valid after potential line join
    }
     // Leave cursor at the start position after deletion
    m_cursorRow = startRow;
    m_cursorCol = startCol;
    fixCursor();
}

void Buffer::setSelectionStartInternal(std::pair<int, int> startPos) {
     m_selectionStart = startPos;
     // fix selection position? Ensure it's valid?
}

void Buffer::clearSelectionInternal() {
     m_selectionStart.reset();
}

void Buffer::pushUndo(std::unique_ptr<Change> change) {
    // TODO: Implement merging consecutive changes (e.g., typing chars)
    m_undoStack.push_back(std::move(change));
    m_redoStack.clear(); // Any new distinct action clears the redo stack
}

void Buffer::fixCursor() {
    // Clamp row
    m_cursorRow = std::max(0, std::min(m_cursorRow, getLineCount() - 1));
    // Clamp column (allow 1 position past end of line)
    if (m_cursorRow >= 0 && m_cursorRow < m_lines.size()) {
        m_cursorCol = std::max(0, std::min(m_cursorCol, (int)m_lines[m_cursorRow].length()));
    } else {
        // Should not happen if row is clamped correctly
        m_cursorCol = 0;
    }
}


// --- Public Editing Operations (Create Undo Entries) ---

void Buffer::insertChar(char c) {
    // If selection exists, delete it first
    if (hasSelection()) {
         deleteSelection(); // This will create its own undo entry
    }

    int oldRow = m_cursorRow;
    int oldCol = m_cursorCol;
    std::string text(1, c);

    // Perform insertion (modifies cursor internally)
    insertTextInternal(text);
    m_isEdited = true;

    // Record undo
    pushUndo(std::make_unique<InsertChange>(oldRow, oldCol, std::move(text)));
}

void Buffer::insertNewline() {
    if (hasSelection()) {
         deleteSelection();
    }

    int oldRow = m_cursorRow;
    int oldCol = m_cursorCol;

    // Perform insertion (modifies cursor internally)
    insertTextInternal("\n");
    m_isEdited = true;

    // Record undo
    pushUndo(std::make_unique<InsertChange>(oldRow, oldCol, "\n"));
}

void Buffer::insertString(const std::string& text) {
     // For undo purposes, it's often better to treat this as one operation
     // if possible, rather than multiple char/newline inserts.
     if (text.empty()) return;

     if (hasSelection()) {
          deleteSelection();
     }

     int oldRow = m_cursorRow;
     int oldCol = m_cursorCol;

     insertTextInternal(text); // Perform the insertion
     m_isEdited = true;

     // Single undo entry for the whole string
     pushUndo(std::make_unique<InsertChange>(oldRow, oldCol, text));
}

void Buffer::deleteCharForward() { // Delete key
    if (hasSelection()) {
        deleteSelection();
        return;
    }

    // If no selection, delete character or join line
    int oldRow = m_cursorRow;
    int oldCol = m_cursorCol;
    std::string deletedText = "";

    if (m_cursorCol < m_lines[m_cursorRow].length()) {
        // Get char to delete
        deletedText = m_lines[m_cursorRow].substr(m_cursorCol, 1);
        // Perform deletion internally
        deleteTextInternal(m_cursorRow, m_cursorCol, 1);
    } else if (m_cursorRow < m_lines.size() - 1) {
        // Get newline to delete
        deletedText = "\n";
        // Perform deletion (join lines) internally
        deleteTextInternal(m_cursorRow, m_cursorCol, 1);
    } else {
        return; // Nothing to delete at end of buffer
    }

    m_isEdited = true;
    pushUndo(std::make_unique<DeleteChange>(oldRow, oldCol, std::move(deletedText)));
    // Cursor stays at oldRow, oldCol after forward delete
    setCursorPosition(oldRow, oldCol); // Explicitly set without creating MoveChange
    fixCursor();
}

void Buffer::deleteCharBackward() { // Backspace key
    if (hasSelection()) {
        deleteSelection();
        return;
    }

    // If no selection, delete character or join line
    int oldRow = m_cursorRow;
    int oldCol = m_cursorCol;
    std::string deletedText = "";

    if (m_cursorCol > 0) {
        // Delete char within line
        deletedText = m_lines[m_cursorRow].substr(m_cursorCol - 1, 1);
        // Perform deletion internally, cursor moves back automatically
        deleteTextInternal(m_cursorRow, m_cursorCol - 1, 1);

    } else if (m_cursorRow > 0) {
        // Delete newline (join lines)
        deletedText = "\n";
        int prevLineLen = m_lines[m_cursorRow - 1].length();
        // Perform deletion internally
        deleteTextInternal(m_cursorRow - 1, prevLineLen, 1); // Delete the '\n' at end of prev line
        // Cursor should end up at end of previous line after internal call
    } else {
        return; // Nothing to delete at start of buffer
    }

    m_isEdited = true;
    // The position for DeleteChange should be where the deletion *started*
    // For backspace within a line, it's (oldRow, oldCol - 1)
    // For backspace joining lines, it's (oldRow - 1, end of that line)
    int deleteStartRow = (m_cursorCol > 0 || oldRow == 0) ? oldRow : oldRow - 1;
    int deleteStartCol = (m_cursorCol > 0) ? oldCol - 1 : (oldRow > 0 ? (int)m_lines[oldRow - 1].length() : 0);

    pushUndo(std::make_unique<DeleteChange>(deleteStartRow, deleteStartCol, std::move(deletedText)));
     // Cursor position is already updated by deleteTextInternal
}

void Buffer::deleteSelection() {
    auto rangeOpt = calculateSelectionRange();
    if (!rangeOpt) return; // No selection to delete

    auto [startPos, endPos] = *rangeOpt;
    std::string deletedText = getSelectedText().value_or(""); // Get text *before* deleting

    if (deletedText.empty()) {
        unselect(); // Clear selection state if range was invalid/empty
        return;
    }

    // Perform deletion internally
    // Calculate length carefully, considering newlines
    size_t lengthToDelete = deletedText.length(); // Approximation, might differ slightly with internal delete logic

    deleteTextInternal(startPos.first, startPos.second, lengthToDelete);
    m_isEdited = true;
    unselect(); // Deleting selection clears selection state

    // Record undo for the whole deleted block
    pushUndo(std::make_unique<DeleteChange>(startPos.first, startPos.second, std::move(deletedText)));
    // Cursor is left at the start of the deleted range by deleteTextInternal
}


void Buffer::moveCursor(Direction dir, bool selecting) {
    int oldRow = m_cursorRow;
    int oldCol = m_cursorCol;
    std::optional<std::pair<int, int>> oldSelectionStart = m_selectionStart;

    // Start selection if Shift is pressed and no selection exists
    if (selecting && !m_selectionStart.has_value()) {
         selectStart(); // This sets the anchor and pushes SelectChange
    } else if (!selecting && m_selectionStart.has_value()) {
         // Moving without Shift clears existing selection
         unselect(); // This clears the anchor and pushes UnselectChange
    }

    // --- Actual cursor movement ---
    switch (dir) {
        case Direction::Up:
            if (m_cursorRow > 0) m_cursorRow--;
            break;
        case Direction::Down:
            if (m_cursorRow < getLineCount() - 1) m_cursorRow++;
            break;
        case Direction::Left:
            if (m_cursorCol > 0) m_cursorCol--;
            else if (m_cursorRow > 0) { // Move to end of previous line
                m_cursorRow--;
                m_cursorCol = static_cast<int>(m_lines[m_cursorRow].length());
            }
            break;
        case Direction::Right:
            if (m_cursorCol < m_lines[m_cursorRow].length()) {
                m_cursorCol++;
            } else if (m_cursorRow < getLineCount() - 1) { // Move to start of next line
                m_cursorRow++;
                m_cursorCol = 0;
            }
            break;
        case Direction::Nowhere:
            break; // No movement
    }
    fixCursor(); // Adjust col based on new line length, clamp row/col

    // --- Record Undo ---
    // Record move change *only if* position actually changed
    if (m_cursorRow != oldRow || m_cursorCol != oldCol) {
        pushUndo(std::make_unique<MoveChange>(oldRow, oldCol, m_cursorRow, m_cursorCol));
        // Note: The undo for MoveChange currently doesn't restore selection state.
        // A more complex Change system might be needed for perfect selection undo.
    }
}

void Buffer::selectStart() {
     // Public action to set selection anchor
     if (!m_selectionStart.has_value()) { // Only set if not already selecting
          std::pair<int, int> anchor = {m_cursorRow, m_cursorCol};
          auto oldSelection = m_selectionStart; // Will be nullopt here
          setSelectionStartInternal(anchor);
          pushUndo(std::make_unique<SelectChange>(oldSelection, anchor));
     }
}

void Buffer::unselect() {
      // Public action to clear selection anchor
      if (m_selectionStart.has_value()) {
           auto oldSelection = m_selectionStart; // Capture old value
           clearSelectionInternal();
           pushUndo(std::make_unique<UnselectChange>(*oldSelection));
      }
}

// --- Undo/Redo Implementation ---

void Buffer::undo() {
    if (m_undoStack.empty()) {
        // std::cout << "Undo stack empty\n"; // Debug
        return;
    }

    std::unique_ptr<Change> changeToUndo = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    // std::cout << "Undoing change...\n"; // Debug
    changeToUndo->undo(*this); // Perform the undo action

    m_redoStack.push_back(std::move(changeToUndo)); // Move it to redo stack

    // Determine edited status (simple check: edited if undo possible)
    m_isEdited = !m_undoStack.empty();
    fixCursor(); // Ensure cursor is valid after undo
    // std::cout << "Undo complete. Redo stack size: " << m_redoStack.size() << "\n"; // Debug
}

void Buffer::redo() {
    if (m_redoStack.empty()) {
        // std::cout << "Redo stack empty\n"; // Debug
        return;
    }

    std::unique_ptr<Change> changeToRedo = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    // std::cout << "Redoing change...\n"; // Debug
    changeToRedo->apply(*this); // Re-apply the action

    m_undoStack.push_back(std::move(changeToRedo)); // Move it back to undo stack

    m_isEdited = true; // Any redo makes the buffer edited
    fixCursor();       // Ensure cursor is valid after redo
    // std::cout << "Redo complete. Undo stack size: " << m_undoStack.size() << "\n"; // Debug
}