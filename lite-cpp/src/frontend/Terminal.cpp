#include "Terminal.hpp"
#include "../Editor.hpp"   // Needed for render/waitForInput access
#include "../Buffer.hpp"   // Needed for buffer data access
#include "../Common.hpp"   // Maybe for Direction if needed here

#include <ncurses.h>       // The core ncurses library header
#include <string>
#include <vector>
#include <cmath>           // For log10 in line number width calculation
#include <algorithm>       // For std::min/max
#include <stdexcept>       // For potential errors
#include <locale>          // For std::tolower
#include <cwchar>          // Potentially for wide char input later

// Color Pair definitions (adjust as needed)
#define COLOR_PAIR_NORMAL    1
#define COLOR_PAIR_STATUS    2
#define COLOR_PAIR_KEYWORD   3
#define COLOR_PAIR_TYPE      4
#define COLOR_PAIR_OPERATOR  5
#define COLOR_PAIR_COMMENT   6
#define COLOR_PAIR_STRING    7
#define COLOR_PAIR_NUMBER    8
#define COLOR_PAIR_SELECTION 9
#define COLOR_PAIR_LINENUM   10


// --- Destructor ---
Terminal::~Terminal() {
    // Ensure shutdown happens even if not explicitly called
    shutdown();
}

// --- Lifecycle ---
void Terminal::initialize() {
    if (m_isInitialized) return;

    // Set locale for potentially better character handling
    // setlocale(LC_ALL, ""); // Uncomment if needed, requires <locale>

    if (initscr() == NULL) { // Initialize ncurses screen
         throw std::runtime_error("Failed to initialize ncurses screen.");
    }

    if (has_colors() == FALSE) {
        endwin(); // Clean up if colors not supported but needed
        throw std::runtime_error("Terminal does not support colors.");
        // Or fallback to non-color mode?
    }
    start_color(); // Enable color functionality

    // Define color pairs (Background: -1 for default terminal background)
    // Pairs: ID, Foreground, Background
    init_pair(COLOR_PAIR_NORMAL, COLOR_WHITE, -1);
    init_pair(COLOR_PAIR_STATUS, COLOR_BLACK, COLOR_WHITE); // Status: Black on White
    init_pair(COLOR_PAIR_KEYWORD, COLOR_MAGENTA, -1);
    init_pair(COLOR_PAIR_TYPE, COLOR_BLUE, -1);
    init_pair(COLOR_PAIR_OPERATOR, COLOR_YELLOW, -1);
    init_pair(COLOR_PAIR_COMMENT, COLOR_GREEN, -1);
    init_pair(COLOR_PAIR_STRING, COLOR_CYAN, -1);
    init_pair(COLOR_PAIR_NUMBER, COLOR_RED, -1);
    init_pair(COLOR_PAIR_SELECTION, COLOR_BLACK, COLOR_YELLOW); // Selection: Black on Yellow
    init_pair(COLOR_PAIR_LINENUM, COLOR_YELLOW, -1); // Line numbers: Yellow

    raw();                // Disable line buffering
    keypad(stdscr, TRUE); // Enable function keys (F1, arrows, etc.)
    noecho();             // Don't echo typed characters back to screen
    curs_set(1);          // Make cursor visible (1 = normal)

    getmaxyx(stdscr, m_totalRows, m_totalCols); // Get screen dimensions
    m_editorRows = m_totalRows - 1; // Reserve 1 row for status bar

    m_isInitialized = true;
}

void Terminal::shutdown() {
    if (!m_isInitialized || isendwin()) return; // Prevent double shutdown

    // `endwin` restores the terminal to its normal state
    endwin();
    m_isInitialized = false;
}

// --- Core Interaction ---

void Terminal::render(const Editor& editor) {
    if (!m_isInitialized) return;

    erase(); // Clear the virtual screen buffer

    const Buffer* buffer = editor.getCurrentBuffer();

    // --- Scrolling Logic ---
    if (buffer) {
        int cursorRow = buffer->getCursorRow();
        // Scroll down if cursor is below the visible area
        while (cursorRow >= m_displayStartRow + m_editorRows) {
            m_displayStartRow++;
        }
        // Scroll up if cursor is above the visible area
        while (cursorRow < m_displayStartRow) {
            m_displayStartRow--;
        }
         // Ensure start row is not negative
         m_displayStartRow = std::max(0, m_displayStartRow);
    } else {
        m_displayStartRow = 0; // Reset scroll for empty view
    }

    // --- Draw Components ---
    drawBuffer(buffer);   // Draw buffer content or placeholder
    drawStatusBar();      // Draw the status line

    // --- Position Cursor ---
    if (buffer) {
        int lineNumWidth = (buffer->getLineCount() == 0) ? 1 : (int)log10(buffer->getLineCount()) + 1;
        int termRow = buffer->getCursorRow() - m_displayStartRow;
        int termCol = buffer->getCursorCol() + lineNumWidth + 1; // +1 for space
        // Ensure cursor stays within visible bounds (important after buffer clearing)
        termRow = std::max(0, std::min(termRow, m_editorRows - 1));
        termCol = std::max(0, std::min(termCol, m_totalCols - 1));
        move(termRow, termCol); // Position the physical cursor
    } else {
        move(0, 0); // Move to top-left if no buffer
    }

    refresh(); // Update the physical terminal screen
}

InputEvent Terminal::waitForInput(const Editor& editor) {
    if (!m_isInitialized) return InputEvent{}; // Return default if not ready

    int ncurses_char = getch(); // Blocking call to get input character/code

    // Handle resize event specifically
    if (ncurses_char == KEY_RESIZE) {
        handleResize();
        // Return a "None" event or recursively call waitForInput
        // after potentially re-rendering due to resize?
        // For simplicity, return None and let the next loop iteration re-render.
        return InputEvent{};
    }

    return mapNcursesKey(ncurses_char); // Translate to our InputEvent format
}

void Terminal::setStatus(const std::string& status) {
    m_statusMessage = status;
    // Optimization: could redraw just status bar immediately,
    // but render() will pick it up on the next loop.
    // drawStatusBar(); refresh(); // Uncomment for immediate update
}

// --- User Prompts ---

std::optional<std::string> Terminal::prompt(const std::string& promptText, const std::optional<std::string>& prefill) {
    if (!m_isInitialized) return std::nullopt;

    std::string input = prefill.value_or("");
    int statusRow = m_totalRows - 1; // Last row
    curs_set(1); // Ensure cursor is visible for input

    // Clear status bar temporarily
    attron(COLOR_PAIR(COLOR_PAIR_STATUS));
    mvhline(statusRow, 0, ' ', m_totalCols); // Clear with background color

    while (true) {
        // Draw prompt and current input
        mvprintw(statusRow, 0, "%s", promptText.c_str());
        // Print input, ensuring it doesn't overflow status bar width
        int maxInputLen = m_totalCols - static_cast<int>(promptText.length()) - 1; // -1 for cursor space
        if (maxInputLen < 0) maxInputLen = 0;
        std::string displayInput = input.substr(0, maxInputLen);
        printw("%s", displayInput.c_str());

        // Clear remaining part of the status line
        int currentLen = promptText.length() + displayInput.length();
        for(int i = currentLen; i < m_totalCols; ++i) {
             mvaddch(statusRow, i, ' ');
        }

        attroff(COLOR_PAIR(COLOR_PAIR_STATUS));
        move(statusRow, promptText.length() + displayInput.length()); // Move cursor to end
        refresh();

        int ch = getch();
        InputEvent event = mapNcursesKey(ch); // Use mapping for consistency

        switch (event.code) {
            case KeyCode::Enter:
                curs_set(1); // Ensure cursor is visible after prompt
                return input; // Return the collected input
            case KeyCode::Escape:
                curs_set(1);
                return std::nullopt; // User cancelled
            case KeyCode::Backspace:
                if (!input.empty()) {
                    input.pop_back();
                }
                break;
            case KeyCode::Char:
                input += event.character;
                break;
            case KeyCode::None: // Ignore unhandled keys during prompt
            default:
                 // Maybe beep()?
                 break;
        }
    }
}

std::optional<bool> Terminal::ask(const std::string& promptText, const std::string& yesOption, const std::string& noOption) {
     if (!m_isInitialized) return std::nullopt;

     int statusRow = m_totalRows - 1;
     // Decide on yes/no characters (case-insensitive check later)
     char yesChar = (yesOption.empty() ? 'y' : std::tolower(yesOption[0]));
     char noChar = (noOption.empty() ? 'n' : std::tolower(noOption[0]));

     std::string fullPrompt = promptText + " (" + yesOption + "/" + noOption + ")? ";

     curs_set(0); // Hide cursor for simple Y/N prompt

     attron(COLOR_PAIR(COLOR_PAIR_STATUS));
     mvhline(statusRow, 0, ' ', m_totalCols); // Clear line
     mvprintw(statusRow, 0, "%-*s", m_totalCols, fullPrompt.c_str()); // Print padded
     attroff(COLOR_PAIR(COLOR_PAIR_STATUS));
     refresh();

     while(true) {
         int ch_int = getch();
         char ch = std::tolower(ch_int); // Check lowercase

         if (ch == yesChar) {
             curs_set(1); // Restore cursor visibility
             return true;
         }
         if (ch == noChar) {
              curs_set(1);
              return false;
         }
         if (ch_int == 27) { // Escape key ASCII
              curs_set(1);
              return std::nullopt; // Cancelled
         }
         // Ignore other keys, potentially beep
         // beep();
     }
}

// --- Information ---
int Terminal::getHeight() const {
    // Return usable rows for editor area
    return m_editorRows > 0 ? m_editorRows : 0;
}

int Terminal::getWidth() const {
    return m_totalCols > 0 ? m_totalCols : 0;
}

// --- Private Helper Methods ---

void Terminal::drawBuffer(const Buffer* buffer) {
    if (!buffer) {
        // Optional: Draw welcome message or placeholder if no buffer
        // mvprintw(m_editorRows / 2, (m_totalCols - 10) / 2, "lite-cpp");
        return;
    }

    int lineNumWidth = (buffer->getLineCount() == 0) ? 1 : (int)log10(buffer->getLineCount()) + 1;
    int textStartCol = lineNumWidth + 1; // Col where text actually starts

    // Get selection range once
    auto selectionRange = buffer->getSelectionRange();

    for (int i = 0; i < m_editorRows; ++i) {
        int bufferRow = m_displayStartRow + i;

        if (bufferRow >= buffer->getLineCount()) {
            // Optional: Draw tildes for lines not part of the file
            // attron(COLOR_PAIR(COLOR_PAIR_LINENUM)); // Use a distinct color
            // mvaddch(i, 0, '~');
            // attroff(COLOR_PAIR(COLOR_PAIR_LINENUM));
            break; // Don't draw past the end of the buffer
        }

        // Draw line number
        drawLineNumber(i, bufferRow, lineNumWidth);

        // Draw line content with highlighting and selection overlay
        const std::string& line = buffer->getLine(bufferRow);
        int lineLength = static_cast<int>(line.length());

        // --- Render Line Content with Selection ---
        int currentScreenCol = textStartCol;
        for (int currentCol = 0; currentCol < lineLength; ++currentCol) {
            if (currentScreenCol >= m_totalCols) break; // Stop if exceeding screen width

            bool isSelected = false;
            if (selectionRange) {
                auto [startPos, endPos] = *selectionRange;
                 // Check if current char position (bufferRow, currentCol) is within selection
                 if (bufferRow > startPos.first && bufferRow < endPos.first) {
                     isSelected = true; // Whole line selected (excluding start/end lines)
                 } else if (bufferRow == startPos.first && bufferRow == endPos.first) {
                     // Selection on a single line
                     isSelected = (currentCol >= startPos.second && currentCol < endPos.second);
                 } else if (bufferRow == startPos.first) {
                     // Selection starts on this line
                     isSelected = (currentCol >= startPos.second);
                 } else if (bufferRow == endPos.first) {
                     // Selection ends on this line
                     isSelected = (currentCol < endPos.second);
                 }
            }

            // Apply selection attribute if needed
            if (isSelected) {
                attron(COLOR_PAIR(COLOR_PAIR_SELECTION));
            }

            // Print character (basic, no syntax highlighting yet)
            mvaddch(i, currentScreenCol, line[currentCol]);

            // Turn off selection attribute if applied
            if (isSelected) {
                attroff(COLOR_PAIR(COLOR_PAIR_SELECTION));
            }

            currentScreenCol++;
        }
         // Clear rest of the line on screen
         // clrtoeol(); // Careful: This clears based on current cursor pos
         // Manual clear is safer:
         int clearStartCol = textStartCol + lineLength;
         if (clearStartCol < m_totalCols) {
              mvhline(i, clearStartCol, ' ', m_totalCols - clearStartCol);
         }


        // TODO: Integrate printLineWithHighlighting instead of basic mvaddch loop
        // printLineWithHighlighting(i, textStartCol, buffer->getLine(bufferRow));
    }
}

void Terminal::drawLineNumber(int screenRow, int bufferRow, int width) {
     attron(COLOR_PAIR(COLOR_PAIR_LINENUM));
     // Use snprintf for safe formatting
     char lineNumStr[16]; // Should be large enough
     snprintf(lineNumStr, sizeof(lineNumStr), "%*d", width, bufferRow + 1);
     mvprintw(screenRow, 0, "%s ", lineNumStr); // Add space after number
     attroff(COLOR_PAIR(COLOR_PAIR_LINENUM));
}


void Terminal::printLineWithHighlighting(int screenRow, int startCol, const std::string& line) {
    // Placeholder - implement simple keyword/token based highlighting later
    // For now, just print the line normally, truncated.
    int maxLen = m_totalCols - startCol;
    if (maxLen <= 0) return;
    std::string truncatedLine = line.substr(0, maxLen);
    mvprintw(screenRow, startCol, "%s", truncatedLine.c_str());
}


void Terminal::drawStatusBar() {
    int statusRow = m_totalRows - 1;
    attron(COLOR_PAIR(COLOR_PAIR_STATUS));
    // Clear the entire status bar line with the status bar color pair
    mvhline(statusRow, 0, ' ', m_totalCols);
    // Print the status message, truncated if necessary
    std::string truncatedStatus = m_statusMessage.substr(0, m_totalCols);
    mvprintw(statusRow, 0, "%s", truncatedStatus.c_str());
    attroff(COLOR_PAIR(COLOR_PAIR_STATUS));
}

void Terminal::handleResize() {
    // Update dimensions
    getmaxyx(stdscr, m_totalRows, m_totalCols);
    m_editorRows = m_totalRows - 1;
    if (m_editorRows < 0) m_editorRows = 0; // Ensure non-negative

    // Clear screen and tell ncurses about the resize
    // Optional: `resizeterm(m_totalRows, m_totalCols)` if available and needed
    clear();
    // The main loop's render call will handle redrawing everything correctly.
}

// --- Key Mapping ---
InputEvent Terminal::mapNcursesKey(int ncurses_char) {
    InputEvent event;
    // Modifiers are hard to detect reliably with basic getch()
    // Assume none for now, unless specific sequences are parsed
    event.modifiers = KeyModifier::None;

    switch (ncurses_char) {
        // Special Keys
        case KEY_UP:        event.code = KeyCode::Up; break;
        case KEY_DOWN:      event.code = KeyCode::Down; break;
        case KEY_LEFT:      event.code = KeyCode::Left; break;
        case KEY_RIGHT:     event.code = KeyCode::Right; break;
        case KEY_HOME:      event.code = KeyCode::Home; break;
        case KEY_END:       event.code = KeyCode::End; break;
        case KEY_PPAGE:     event.code = KeyCode::PageUp; break;
        case KEY_NPAGE:     event.code = KeyCode::PageDown; break;
        case KEY_DC:        event.code = KeyCode::Delete; break; // Delete Character
        case KEY_ENTER:     // Fallthrough
        case '\n':          // Enter/Return
        case '\r':          // Carriage Return (sometimes)
            event.code = KeyCode::Enter; break;
        case '\t':          // Tab key
            event.code = KeyCode::Tab; break;
        case 27:            // Escape key
            // TODO: Implement non-blocking check for Alt sequences (e.g., Esc + char)
            // For now, just treat it as plain Escape
            event.code = KeyCode::Escape;
            break;
        case KEY_BACKSPACE: // Explicit backspace key code
        case 127:           // Often DEL ASCII or Backspace on some terms
        case 8:             // ASCII Backspace
            event.code = KeyCode::Backspace; break;

        // Handle Ctrl + Alpha keys (ASCII 1-26 map to Ctrl+A to Ctrl+Z)
        default:
            if (ncurses_char >= 1 && ncurses_char <= 26) {
                event.code = KeyCode::Char;
                event.character = static_cast<char>('a' + ncurses_char - 1);
                event.modifiers = KeyModifier::Control;
            }
            // Handle printable characters
            // Need to be careful about multi-byte UTF-8 chars if locale is set
            // Basic ASCII handling:
            else if (ncurses_char >= 32 && ncurses_char <= 126) {
                event.code = KeyCode::Char;
                event.character = static_cast<char>(ncurses_char);
                // TODO: Detect Shift modifier if possible (complex)
            }
            // else: event remains KeyCode::None for unhandled codes
            break;
    }
    return event;
}

// --- Syntax Highlighting (Basic Placeholder) ---
Terminal::SyntaxToken Terminal::getTokenType(const std::string& token) {
    // Simple placeholder logic
    // Add actual lists of keywords, types, operators
    if (token == "int" || token == "void" || token == "class" || token == "struct") return SyntaxToken::Type;
    if (token == "if" || token == "else" || token == "for" || token == "while" || token == "return") return SyntaxToken::Keyword;
    if (token == "+" || token == "-" || token == "*" || token == "/" || token == "=" || token == "==") return SyntaxToken::Operator;
    // Add checks for numbers, strings, comments
    return SyntaxToken::Normal;
}

void Terminal::applyColor(SyntaxToken tokenType) {
    int colorPair = COLOR_PAIR_NORMAL;
    switch(tokenType) {
        case SyntaxToken::Keyword: colorPair = COLOR_PAIR_KEYWORD; break;
        case SyntaxToken::Type:    colorPair = COLOR_PAIR_TYPE; break;
        case SyntaxToken::Operator:colorPair = COLOR_PAIR_OPERATOR; break;
        // Add others
        default: break;
    }
    attron(COLOR_PAIR(colorPair));
}

void Terminal::resetColor() {
    attroff(COLOR_PAIR(COLOR_PAIR_KEYWORD));
    attroff(COLOR_PAIR(COLOR_PAIR_TYPE));
    attroff(COLOR_PAIR(COLOR_PAIR_OPERATOR));
    // Add others
    attron(COLOR_PAIR(COLOR_PAIR_NORMAL)); // Explicitly set back to normal
}