#pragma once

#include "Buffer.hpp"             // Needs Buffer definition
#include "frontend/IFrontend.hpp" // Needs Frontend interface
#include "frontend/Input.hpp"     // Needs InputEvent definition
#include "lang/Environment.hpp"   // Needs Scripting Environment definition
#include "lang/Expr.hpp"          // Needs Scripting Expression definition
#include "Common.hpp"             // For Direction, etc.

#include <vector>
#include <memory>         // For std::unique_ptr, std::shared_ptr
#include <string>
#include <filesystem>     // Requires C++17
#include <optional>

// Forward Declarations (if needed, though included above)
// class Buffer;
// class IFrontend;
// class Environment;
// struct InputEvent;
// enum class Direction;
// using Expr = ... ; // Assuming Expr is defined in lang/Expr.hpp

class Editor {
private:
    std::vector<std::unique_ptr<Buffer>> m_buffers;
    int m_currentBufferIndex = -1;          // -1 indicates no buffer selected (initially)
    std::unique_ptr<IFrontend> m_frontend;  // Owns the frontend interface implementation
    std::shared_ptr<Environment> m_scriptEnv; // Global scripting environment (shared_ptr for capture)

    std::string m_clipboard;                // Simple internal clipboard
    std::string m_lastSearchQuery;          // Store last search term
    std::string m_lastEvalCommand;          // Store last script command evaluated
    bool m_shouldExit = false;              // Flag to signal exit from main loop

    // --- Private Helper Methods ---
    void setupBuiltins();                   // Initialize scripting built-in functions
    void updateStatus();                    // Update the status bar message based on current state
    Buffer* getCurrentBufferInternal();     // Non-const internal helper

public:
    // Constructor takes ownership of the frontend implementation
    Editor(std::unique_ptr<IFrontend> frontend);
    ~Editor() = default; // Default destructor is likely fine with unique_ptr

    // --- Initialization & Setup ---
    bool loadConfig(const std::filesystem::path& configPath);
    bool openFile(const std::filesystem::path& filePath); // Open initial file(s)

    // --- Core Loop Control ---
    void run();                             // Starts the main event loop

    // --- Selection Methods ---
    std::optional<std::string> getSelectedText() const;
    void startSelection();

    // --- Buffer Management ---
    int createNewBuffer();                  // Creates a new empty buffer, returns its index
    bool switchToBuffer(int index);         // Switch focus to a specific buffer index
    bool closeCurrentBuffer(bool force = false); // Close the active buffer (prompts if edited)
    bool nextBuffer();                      // Switch to the next buffer in the list
    bool prevBuffer();                      // Switch to the previous buffer in the list
    int getCurrentBufferIndex() const;
    int getBufferCount() const;
    const Buffer* getCurrentBuffer() const; // Read-only access to current buffer

    // --- Editor Actions (Public API called from main or builtins) ---
    void handleInput(const InputEvent& event); // Dispatches input to specific actions
    void insertCharacter(char c);
    void deleteBackward();                  // Backspace action
    void deleteForward();                   // Delete action
    void insertNewline();
    void moveCursor(Direction dir, bool selecting = false);
    void moveCursorPageUp(bool selecting = false);
    void moveCursorPageDown(bool selecting = false);
    void moveCursorStartOfLine(bool selecting = false);
    void moveCursorEndOfLine(bool selecting = false);
    void undo();
    void redo();
    bool saveCurrentBuffer();               // Save with current name or prompt if needed
    bool saveCurrentBufferAs();             // Force prompt for new name
    bool findInCurrentBuffer();             // Prompt for text and find next occurrence
    void selectAll();
    void copySelection();
    void cutSelection();
    void paste();
    bool runShellCommand();                 // Prompt for command, run, show output in new buffer
    bool evaluateScriptPrompt();            // Prompt for script, evaluate, show result
    void insertString(const std::string& text);      // Insert a string at current cursor position
    void gotoPosition(int line, int column);         // Move cursor to a specific position
    bool hasSelection() const;                       // Check if there is an active selection
    std::string getSelection() const;                // Get the currently selected text
    void clearSelection();                           // Clear the current selection

    // --- Scripting Interaction ---
    Expr evaluateScript(const std::string& scriptText); // Parse & evaluate text
    Expr evaluateExpression(const Expr& expression);   // Evaluate existing AST node
    // Provide access for builtins (non-const needed for editor manipulation)
    Environment& getScriptEnvironment() { return *m_scriptEnv; }

    // --- Frontend Access ---
    IFrontend* getFrontend() { return m_frontend.get(); } // Raw pointer access (non-owning)
};