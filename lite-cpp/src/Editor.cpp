#include "Editor.hpp"
#include "Buffer.hpp"             // Need Buffer implementation details
#include "Change.hpp"             // For creating Change objects (if actions do it directly)
#include "Common.hpp"
#include "frontend/IFrontend.hpp"
#include "lang/Parser.hpp"        // Need parser for config/eval
#include "lang/Interpreter.hpp"   // Need interpreter
#include "lang/Builtin.hpp"       // For setting up builtins
#include "lang/Expr.hpp"          // For Expr types and helpers (e.g., toString)

#include <fstream>
#include <stdexcept>
#include <algorithm>    // For std::min/max
#include <iostream>     // For potential debug output
#include <cstdlib>      // For std::system (basic shell command execution)

// --- Constructor ---
Editor::Editor(std::unique_ptr<IFrontend> frontend)
    : m_frontend(std::move(frontend)),
      m_scriptEnv(std::make_shared<Environment>()) // Create the root environment
{
    if (!m_frontend) {
        throw std::runtime_error("Frontend cannot be null.");
    }
    setupBuiltins();
    // Create an initial empty buffer after builtins are ready (config might use them)
    createNewBuffer();
    switchToBuffer(0); // Start with the first buffer active
    updateStatus();    // Set initial status message
}

// --- Private Helpers ---

Buffer* Editor::getCurrentBufferInternal() {
    if (m_currentBufferIndex >= 0 && m_currentBufferIndex < m_buffers.size()) {
        return m_buffers[m_currentBufferIndex].get();
    }
    return nullptr;
}

void Editor::updateStatus() {
    if (!m_frontend) return;

    const Buffer* buf = getCurrentBuffer();
    std::string status;
    if (buf) {
        std::string filename = "[No Name]";
        if (auto pathOpt = buf->getFilePath()) {
            filename = pathOpt->filename().string(); // Display only filename
        }
        status = "Editing: " + filename + (buf->isEdited() ? "*" : "") +
                 " | Buffer " + std::to_string(m_currentBufferIndex) + "/" + std::to_string(m_buffers.size() - 1) +
                 " | Ln " + std::to_string(buf->getCursorRow() + 1) +
                 ", Col " + std::to_string(buf->getCursorCol() + 1);
    } else {
        status = "lite-cpp | No buffer open.";
    }
    m_frontend->setStatus(status);
}

// --- Initialization & Setup ---

bool Editor::loadConfig(const std::filesystem::path& configPath) {
    std::ifstream configFile(configPath);
    if (!configFile) {
        if (m_frontend) m_frontend->setStatus("Config file not found: " + configPath.string());
        return false;
    }

    std::string scriptContent((std::istreambuf_iterator<char>(configFile)),
                              std::istreambuf_iterator<char>());

    if (scriptContent.empty()) {
        if (m_frontend) m_frontend->setStatus("Config file is empty: " + configPath.string());
        return true; // Empty config is not an error
    }

    try {
        // Note: Config is evaluated *before* opening command-line files
        evaluateScript(scriptContent);
        if (m_frontend) m_frontend->setStatus("Config loaded: " + configPath.string());
        updateStatus(); // Update status based on any changes from config
        return true;
    } catch (const std::runtime_error& e) {
        if (m_frontend) m_frontend->setStatus("Config Error: " + std::string(e.what()));
        return false;
    }
}

bool Editor::openFile(const std::filesystem::path& filePath) {
    try {
        // If the initial empty buffer is unmodified, replace it.
        bool replacedInitial = false;
        if (m_buffers.size() == 1 && m_currentBufferIndex == 0 && !m_buffers[0]->isEdited()) {
            m_buffers[0] = std::make_unique<Buffer>(filePath);
            replacedInitial = true;
        } else {
            m_buffers.push_back(std::make_unique<Buffer>(filePath));
        }

        if (!replacedInitial) {
             return switchToBuffer(static_cast<int>(m_buffers.size() - 1));
        } else {
             // Already switched (index 0), just update status
             updateStatus();
             return true;
        }

    } catch (const std::exception& e) {
         if (m_frontend) {
             m_frontend->setStatus("Error opening file: " + std::string(e.what()));
         }
        return false;
    }
}

// --- Core Loop ---

void Editor::run() {
    if (!m_frontend) return;

    m_frontend->initialize(); // Setup terminal

    while (!m_shouldExit) {
        updateStatus(); // Refresh status line info
        m_frontend->render(*this);
        InputEvent input = m_frontend->waitForInput(*this);
        handleInput(input);
    }

    m_frontend->shutdown(); // Restore terminal
}

// --- Buffer Management ---

int Editor::createNewBuffer() {
    m_buffers.push_back(std::make_unique<Buffer>());
    int newIndex = static_cast<int>(m_buffers.size() - 1);
    // Optionally switch to it immediately
    // switchToBuffer(newIndex);
    return newIndex;
}

bool Editor::switchToBuffer(int index) {
    if (index >= 0 && index < m_buffers.size()) {
        m_currentBufferIndex = index;
        updateStatus();
        return true;
    }
    return false;
}

bool Editor::closeCurrentBuffer(bool force) {
    if (m_currentBufferIndex < 0 || m_currentBufferIndex >= m_buffers.size()) {
        return false; // No buffer selected or invalid index
    }

    Buffer* buf = getCurrentBufferInternal();
    if (!buf) return false; // Should not happen if index is valid

    if (!force && buf->isEdited()) {
        if (!m_frontend) return false; // Cannot prompt without frontend
        // Ask user to save
        auto confirm = m_frontend->ask("Buffer modified. Save?", "y", "n");
        if (!confirm.has_value()) {
             m_frontend->setStatus("Close cancelled.");
             return false; // User cancelled prompt
        }
        if (*confirm) { // User chose 'yes'
            if (!saveCurrentBuffer()) {
                // Save failed (e.g., no filename, IO error, user cancelled Save As)
                m_frontend->setStatus("Save failed or cancelled. Buffer not closed.");
                return false;
            }
        }
        // If user chose 'no', proceed to close without saving
    }

    // --- Proceed with closing ---
    m_buffers.erase(m_buffers.begin() + m_currentBufferIndex);

    // Adjust current index and ensure at least one buffer exists
    if (m_buffers.empty()) {
        m_currentBufferIndex = -1; // No buffer selected now
        requestExit(); // Exit if the last buffer was closed
        // Alternative: createNewBuffer(); switchToBuffer(0);
    } else {
        // If we deleted buffer 0, index 0 is still valid (points to the new buffer 0)
        // If we deleted buffer N > 0, the new buffer N-1 is at the old index N-1.
        // If we deleted the *last* buffer (index > 0), the index should point to the new last buffer.
        m_currentBufferIndex = std::min(m_currentBufferIndex, static_cast<int>(m_buffers.size() - 1));
        // Ensure index is not negative if deletion logic somehow caused it
        if (m_currentBufferIndex < 0) m_currentBufferIndex = 0;
        switchToBuffer(m_currentBufferIndex); // Activate the new current buffer
    }
    updateStatus();
    return true;
}

bool Editor::nextBuffer() {
    if (m_buffers.size() <= 1) return false;
    int nextIndex = (m_currentBufferIndex + 1) % static_cast<int>(m_buffers.size());
    return switchToBuffer(nextIndex);
}

bool Editor::prevBuffer() {
    if (m_buffers.size() <= 1) return false;
    int prevIndex = (m_currentBufferIndex - 1 + static_cast<int>(m_buffers.size())) % static_cast<int>(m_buffers.size());
    return switchToBuffer(prevIndex);
}

int Editor::getCurrentBufferIndex() const { return m_currentBufferIndex; }
int Editor::getBufferCount() const { return static_cast<int>(m_buffers.size()); }

const Buffer* Editor::getCurrentBuffer() const {
    // Const version of the internal helper
    if (m_currentBufferIndex >= 0 && m_currentBufferIndex < m_buffers.size()) {
        return m_buffers[m_currentBufferIndex].get();
    }
    return nullptr;
}

// --- Editor Actions ---

void Editor::insertCharacter(char c) {
    if (Buffer* buf = getCurrentBufferInternal()) {
        // TODO: Delete selection if one exists before inserting
        buf->insertChar(c);
    }
}
void Editor::deleteBackward() {
    if (Buffer* buf = getCurrentBufferInternal()) {
        // TODO: Handle deleting selection first
        buf->deleteCharBackward();
    }
}
void Editor::deleteForward() {
    if (Buffer* buf = getCurrentBufferInternal()) {
        // TODO: Handle deleting selection first
        buf->deleteCharForward();
    }
}
void Editor::insertNewline() {
    if (Buffer* buf = getCurrentBufferInternal()) {
        // TODO: Delete selection if one exists before inserting
        buf->insertNewline();
    }
}
void Editor::moveCursor(Direction dir, bool selecting) {
    if (Buffer* buf = getCurrentBufferInternal()) {
        buf->moveCursor(dir, selecting);
    }
}

void Editor::moveCursorPageUp(bool selecting) {
     if (Buffer* buf = getCurrentBufferInternal() && m_frontend) {
         int linesToMove = m_frontend->getHeight(); // Move by screen height
         for (int i = 0; i < linesToMove; ++i) {
              buf->moveCursor(Direction::Up, selecting);
         }
     }
}
void Editor::moveCursorPageDown(bool selecting) {
      if (Buffer* buf = getCurrentBufferInternal() && m_frontend) {
         int linesToMove = m_frontend->getHeight(); // Move by screen height
         for (int i = 0; i < linesToMove; ++i) {
              buf->moveCursor(Direction::Down, selecting);
         }
     }
}
void Editor::moveCursorStartOfLine(bool selecting) {
     if (Buffer* buf = getCurrentBufferInternal()) {
         buf->setCursorPosition(buf->getCursorRow(), 0); // Simple version
         // TODO: handle selection update
     }
}
void Editor::moveCursorEndOfLine(bool selecting) {
      if (Buffer* buf = getCurrentBufferInternal()) {
          buf->setCursorPosition(buf->getCursorRow(), buf->getLine(buf->getCursorRow()).length());
          // TODO: handle selection update
     }
}


void Editor::undo() {
    if (Buffer* buf = getCurrentBufferInternal()) {
        buf->undo();
    }
}
void Editor::redo() {
    if (Buffer* buf = getCurrentBufferInternal()) {
        buf->redo();
    }
}

bool Editor::saveCurrentBuffer() {
    Buffer* buf = getCurrentBufferInternal();
    if (!buf) return false;

    if (buf->getFilePath()) {
        if (buf->save()) {
            if (m_frontend) m_frontend->setStatus("Saved: " + buf->getFilePath()->filename().string());
            return true;
        } else {
            if (m_frontend) m_frontend->setStatus("Save Failed!");
            return false;
        }
    } else {
        // No file path, trigger Save As logic
        return saveCurrentBufferAs();
    }
}

bool Editor::saveCurrentBufferAs() {
    Buffer* buf = getCurrentBufferInternal();
    if (!buf || !m_frontend) return false;

    auto maybePathStr = m_frontend->prompt("Save As:");
    if (!maybePathStr || maybePathStr->empty()) {
        m_frontend->setStatus("Save cancelled.");
        return false; // User cancelled
    }

    try {
        std::filesystem::path path(*maybePathStr);
        if (buf->saveAs(path)) {
            m_frontend->setStatus("Saved: " + path.filename().string());
            return true;
        } else {
            m_frontend->setStatus("Save Failed!");
            return false;
        }
    } catch (const std::exception& e) {
         m_frontend->setStatus("Invalid path for save: " + std::string(e.what()));
         return false;
    }
}

bool Editor::findInCurrentBuffer() {
     if (!m_frontend) return false;
     Buffer* buf = getCurrentBufferInternal();
     if (!buf) return false;

     auto queryOpt = m_frontend->prompt("Find: ", m_lastSearchQuery);
     if (!queryOpt) {
         m_frontend->setStatus("Find cancelled.");
         return false;
     }
     m_lastSearchQuery = *queryOpt;
     if (m_lastSearchQuery.empty()) return false;

     // TODO: Implement find logic in Buffer class
     // e.g., auto result = buf->findNext(m_lastSearchQuery);
     // if (result) {
     //    buf->setCursorPosition(result->row, result->col);
     //    // Optionally select the found text
     //    m_frontend->setStatus("Found at Ln " + ...);
     //    return true;
     // } else {
          m_frontend->setStatus("Text not found: " + m_lastSearchQuery);
          return false;
     // }
     return false; // Placeholder
}

void Editor::selectAll() {
     if (Buffer* buf = getCurrentBufferInternal()) {
         // TODO: Implement selectAll in Buffer class
         // buf->setSelection(0, 0, buf->getLineCount() - 1, buf->getLine(buf->getLineCount() - 1).length());
         // buf->setCursorPosition(buf->getLineCount() - 1, buf->getLine(buf->getLineCount() - 1).length());
     }
}
void Editor::copySelection() {
     if (const Buffer* buf = getCurrentBuffer()) {
          auto selected = buf->getSelectedText();
          if (selected) {
              m_clipboard = *selected;
              if(m_frontend) m_frontend->setStatus("Copied selection.");
              // Optionally clear selection after copy?
              // buf->unselect(); // Requires non-const buf
          }
     }
}
void Editor::cutSelection() {
     if (Buffer* buf = getCurrentBufferInternal()) {
          auto selected = buf->getSelectedText();
          if (selected) {
               m_clipboard = *selected;
               buf->deleteSelection(); // Requires deleteSelection in Buffer
               if(m_frontend) m_frontend->setStatus("Cut selection.");
          }
     }
}
void Editor::paste() {
      if (Buffer* buf = getCurrentBufferInternal()) {
           if (!m_clipboard.empty()) {
                // TODO: Delete selection if one exists before pasting
                buf->insertString(m_clipboard); // Requires insertString in Buffer
           }
      }
}

bool Editor::runShellCommand() {
     if (!m_frontend) return false;
     auto cmdOpt = m_frontend->prompt("Shell Command: ");
     if (!cmdOpt || cmdOpt->empty()) {
          m_frontend->setStatus("Shell command cancelled.");
          return false;
     }
     std::string command = *cmdOpt;
     m_frontend->setStatus("Running: " + command + " ...");
     // Crude, insecure, non-portable way:
     // int result = std::system(command.c_str());
     // Need a better way to capture output (platform-specific pipes or libraries)
     // Placeholder:
     std::string output = "Output capture for '" + command + "' not implemented.";
     int newBufIdx = createNewBuffer();
     if (switchToBuffer(newBufIdx)) {
         if (Buffer* outputBuf = getCurrentBufferInternal()) {
             outputBuf->insertString(output);
             outputBuf->setCursorPosition(0,0); // Go to start of output
             // Mark buffer as unmodified if desired
         }
     }
     m_frontend->setStatus("Shell command finished (output in new buffer).");
     return true; // Indicate command was attempted
}

bool Editor::evaluateScriptPrompt() {
      if (!m_frontend) return false;
      auto scriptOpt = m_frontend->prompt("Eval: ", m_lastEvalCommand);
      if (!scriptOpt) {
           m_frontend->setStatus("Eval cancelled.");
           return false;
      }
      m_lastEvalCommand = *scriptOpt;
      if (m_lastEvalCommand.empty()) return true; // Empty input is ok

      try {
           Expr result = evaluateScript(m_lastEvalCommand);
           m_frontend->setStatus("Result: " + toString(result)); // Requires toString for Expr
           return true;
      } catch (const std::runtime_error& e) {
           m_frontend->setStatus("Script Error: " + std::string(e.what()));
           return false;
      }
}

// --- Scripting Interaction ---

Expr Editor::evaluateScript(const std::string& scriptText) {
    // TODO: Instantiate Parser and Interpreter correctly
     Parser parser; // Assuming default constructor works
     Expr parsedExpr = parser.parse(scriptText); // Can throw
     return evaluateExpression(parsedExpr);
}

Expr Editor::evaluateExpression(const Expr& expression) {
    // TODO: Instantiate Interpreter correctly
     Interpreter interpreter(*this, m_scriptEnv); // Pass editor ref and current *root* env
     return interpreter.evaluate(expression); // Can throw
}

// --- Builtin Setup ---
void Editor::setupBuiltins() {
    // Add C++ functions to the script environment
    // Uses the functions declared in lang/Builtin.hpp and implemented in lang/Builtin.cpp
    addBuiltinFunction(*m_scriptEnv, "insert", builtin_insert);
    addBuiltinFunction(*m_scriptEnv, "delete", builtin_delete);
    addBuiltinFunction(*m_scriptEnv, "move", builtin_move);
    addBuiltinFunction(*m_scriptEnv, "goto", builtin_goto);
    addBuiltinFunction(*m_scriptEnv, "get-select", builtin_get_select);
    addBuiltinFunction(*m_scriptEnv, "select", builtin_select);
    addBuiltinFunction(*m_scriptEnv, "unselect", builtin_unselect);
    addBuiltinFunction(*m_scriptEnv, "new-buf", builtin_new_buf);
    addBuiltinFunction(*m_scriptEnv, "set-buf", builtin_set_buf);
    addBuiltinFunction(*m_scriptEnv, "get-cur-buf", builtin_get_cur_buf);
    // Add arithmetic, etc.
    addBuiltinFunction(*m_scriptEnv, "add", builtin_add);
    addBuiltinFunction(*m_scriptEnv, "print", builtin_print);
    // ... Add all other necessary builtins ...
}


// --- Input Dispatch ---
void Editor::handleInput(const InputEvent& event) {
    // More detailed input mapping than the simple example in main.cpp previously
    bool selectionHandled = false; // Flag if selection logic should override default move

    // --- Modifier Key Logic ---
    if (event.modifiers == KeyModifier::Control) {
        switch (event.character) {
            case 's': saveCurrentBuffer(); return;
            case 'q': { // Special handling for closing/quitting
                 Buffer* buf = getCurrentBufferInternal();
                 if (getBufferCount() <= 1 && buf && !buf->isEdited()) {
                     requestExit(); // Quit editor if last buffer and not edited
                 } else {
                     closeCurrentBuffer(); // Attempt to close buffer (might fail/prompt)
                 }
                 return;
            }
            case 'o': {
                 auto path = m_frontend->prompt("Open file:");
                 if (path && !path->empty()) openFile(*path);
                 return;
            }
            case 'n': {
                int newIdx = createNewBuffer();
                switchToBuffer(newIdx);
                return;
            }
            case 'z': undo(); return;
            case 'y': redo(); return;
            case 'c': copySelection(); return;
            case 'x': cutSelection(); return;
            case 'v': paste(); return;
            case 'f': findInCurrentBuffer(); return;
            case 'a': selectAll(); return;
            case 'd': { // Ctrl+D often deletes forward or selection
                 // TODO: If selection exists, delete selection, else delete forward
                 deleteForward();
                 return;
            }
            default: break; // Unhandled Ctrl combo
        }
    } else if (event.modifiers == KeyModifier::Alt) {
         // NOTE: Alt handling is basic here, real terminals often use Esc prefixes
         switch (event.character) {
             case 'q': requestExit(); return; // Alt+Q quits editor
             case 'n': nextBuffer(); return;
             case 'p': prevBuffer(); return;
             case 'e': evaluateScriptPrompt(); return;
             case '!': runShellCommand(); return;
             // Handle Alt+<number> for buffer switching
             default:
                if (event.character >= '0' && event.character <= '9') {
                    int bufferIndex = event.character - '0';
                    switchToBuffer(bufferIndex);
                }
                break;
         }
         return;
    } else if (event.modifiers == KeyModifier::Shift) {
        // Handle Shift + Arrows/Home/End/PgUp/PgDn for selection
        switch (event.code) {
            case KeyCode::Left:    moveCursor(Direction::Left, true); selectionHandled = true; break;
            case KeyCode::Right:   moveCursor(Direction::Right, true); selectionHandled = true; break;
            case KeyCode::Up:      moveCursor(Direction::Up, true); selectionHandled = true; break;
            case KeyCode::Down:    moveCursor(Direction::Down, true); selectionHandled = true; break;
            case KeyCode::Home:    moveCursorStartOfLine(true); selectionHandled = true; break;
            case KeyCode::End:     moveCursorEndOfLine(true); selectionHandled = true; break;
            case KeyCode::PageUp:  moveCursorPageUp(true); selectionHandled = true; break;
            case KeyCode::PageDown:moveCursorPageDown(true); selectionHandled = true; break;
            case KeyCode::Char:
                 // Simple uppercase insert for shifted chars
                 insertCharacter(std::toupper(event.character));
                 return; // Don't process default char insert
            default: break; // Unhandled Shift combo
        }
    }

    // If selection movement happened, don't do default move
    if (selectionHandled) return;

    // --- Default Key Handling (No/Other Modifiers) ---
    switch (event.code) {
        case KeyCode::Char:      insertCharacter(event.character); break;
        case KeyCode::Enter:     insertNewline(); break;
        case KeyCode::Backspace: deleteBackward(); break;
        case KeyCode::Delete:    deleteForward(); break;
        case KeyCode::Tab:       insertString("    "); break; // Simple tab
        case KeyCode::Left:      moveCursor(Direction::Left); break;
        case KeyCode::Right:     moveCursor(Direction::Right); break;
        case KeyCode::Up:        moveCursor(Direction::Up); break;
        case KeyCode::Down:      moveCursor(Direction::Down); break;
        case KeyCode::Home:      moveCursorStartOfLine(); break;
        case KeyCode::End:       moveCursorEndOfLine(); break;
        case KeyCode::PageUp:    moveCursorPageUp(); break;
        case KeyCode::PageDown:  moveCursorPageDown(); break;
        case KeyCode::Esc:
             // Default Esc action: clear selection? Cancel multi-key command?
             // For now, do nothing specific unless part of Alt sequence (handled above)
             if (Buffer* buf = getCurrentBufferInternal()) {
                 // Example: buf->unselect();
             }
             break;
        case KeyCode::None:      // Ignore unknown/invalid keys
        default:
             break;
    }
}