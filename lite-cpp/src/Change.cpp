#include "Change.hpp"
#include "Buffer.hpp" // Include Buffer definition
#include <stdexcept> // For potential errors

// --- InsertChange ---

void InsertChange::apply(Buffer& buffer) {
    // Apply assumes the buffer state is *before* the insertion occurred
    // during the original action. It re-does the insertion.
    // Cursor placement is handled by insertTextInternal.
    buffer.setCursorPosition(m_row, m_col); // Ensure cursor is where insert started
    buffer.insertTextInternal(m_text);      // Perform the insertion
}

void InsertChange::undo(Buffer& buffer) {
    // Undo assumes the buffer state is *after* the insertion occurred.
    // It deletes the text that was inserted.
    // Cursor placement is handled by deleteTextInternal and setCursorPosition.
    buffer.deleteTextInternal(m_row, m_col, m_text.length()); // Delete the inserted text
    buffer.setCursorPosition(m_row, m_col); // Restore cursor to *before* insertion
}

// --- DeleteChange ---

void DeleteChange::apply(Buffer& buffer) {
    // Apply assumes the buffer state is *before* the deletion occurred.
    // It re-does the deletion.
    buffer.deleteTextInternal(m_row, m_col, m_deletedText.length()); // Perform deletion
    buffer.setCursorPosition(m_row, m_col); // Cursor stays at deletion start
}

void DeleteChange::undo(Buffer& buffer) {
    // Undo assumes the buffer state is *after* the deletion occurred.
    // It re-inserts the text that was deleted.
    buffer.setCursorPosition(m_row, m_col); // Ensure cursor is where deletion started
    buffer.insertTextInternal(m_deletedText); // Re-insert the text
    // Cursor will naturally end up after the re-inserted text.
}

// --- MoveChange ---

void MoveChange::apply(Buffer& buffer) {
    // Apply assumes state is *before* the move. Re-does the move.
    buffer.setCursorPosition(m_toRow, m_toCol);
    // TODO: Apply selection changes if move affected selection state
}

void MoveChange::undo(Buffer& buffer) {
    // Undo assumes state is *after* the move. Moves cursor back.
    buffer.setCursorPosition(m_fromRow, m_fromCol);
    // TODO: Restore previous selection state if move affected it
}

// --- SelectChange ---

void SelectChange::apply(Buffer& buffer) {
    // Apply assumes state *before* selection started/changed. Re-sets anchor.
    buffer.setSelectionStartInternal(m_newSelectionStart);
    // Assumes cursor is already at the other end (handled by MoveChange)
}

void SelectChange::undo(Buffer& buffer) {
    // Undo assumes state *after* selection started/changed. Restores old anchor.
    if (m_oldSelectionStart) {
        buffer.setSelectionStartInternal(*m_oldSelectionStart);
    } else {
        buffer.clearSelectionInternal();
    }
    // Cursor position restored by associated MoveChange undo
}

// --- UnselectChange ---

void UnselectChange::apply(Buffer& buffer) {
    // Apply assumes state *before* unselect. Re-clears selection.
    buffer.clearSelectionInternal();
}

void UnselectChange::undo(Buffer& buffer) {
    // Undo assumes state *after* unselect. Re-instates old anchor.
    buffer.setSelectionStartInternal(m_oldSelectionStart);
    // Cursor position should be handled by MoveChange undo if relevant
}