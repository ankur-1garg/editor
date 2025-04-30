#include "Parser.hpp"
#include "Expr.hpp" // Needs factory functions like make_symbol, make_int etc.

#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>    // For isdigit, isalpha, isspace
#include <map>       // For operator precedence


// --- Parser Implementation ---

// Error reporting helper
void Parser::error(const std::string& message) {
    // Include position information for better errors
    // e.g., calculate line/column from m_position
    throw std::runtime_error("Parse Error: " + message + " (at position " + std::to_string(m_position) + ")");
}

// --- Lexer-like Helpers ---

void Parser::advance() {
    m_position++;
    if (!isAtEnd()) {
        m_currentChar = m_source[m_position];
    } else {
        m_currentChar = '\0'; // End of input marker
    }
}

bool Parser::isAtEnd() const {
    return m_position >= m_source.length();
}

char Parser::peek() const {
     if (m_position + 1 >= m_source.length()) {
         return '\0';
     }
     return m_source[m_position + 1];
}


void Parser::skipWhitespace() {
    while (!isAtEnd() && std::isspace(m_currentChar)) {
        advance();
    }
}

void Parser::skipComment() {
    if (m_currentChar == '#') {
        while (!isAtEnd() && m_currentChar != '\n') {
            advance();
        }
         // Skip the newline itself if we found one
         // if (!isAtEnd() && m_currentChar == '\n') advance();
    }
}

long long Parser::parseInteger() {
    std::string numStr;
    bool negative = false;
    if (m_currentChar == '-') {
        negative = true;
        advance();
    }
    if (!std::isdigit(m_currentChar)) {
        error("Expected digit after potential '-' for number");
    }
    while (!isAtEnd() && std::isdigit(m_currentChar)) {
        numStr += m_currentChar;
        advance();
    }
    try {
        long long val = std::stoll(numStr);
        return negative ? -val : val;
    } catch (const std::out_of_range&) {
        error("Integer literal out of range: " + numStr);
    } catch (...) {
         error("Invalid integer literal: " + numStr);
    }
    return 0; // Should not reach here
}

double Parser::parseFloat(long long intPart) {
     // Assumes '.' has been consumed before calling
     std::string fracStr;
     while (!isAtEnd() && std::isdigit(m_currentChar)) {
         fracStr += m_currentChar;
         advance();
     }
     // Combine integer and fractional parts
     std::string fullNumStr = std::to_string(intPart) + "." + fracStr;
     // TODO: Handle potential exponent part 'e'/'E'
     try {
         return std::stod(fullNumStr);
     } catch(...) {
          error("Invalid float literal: " + fullNumStr);
     }
     return 0.0; // Should not reach here
}


std::string Parser::parseStringLiteral() {
     // Assumes opening '"' has been consumed
     std::string value;
     while (!isAtEnd() && m_currentChar != '"') {
         if (m_currentChar == '\\') {
             advance(); // Consume backslash
             if (isAtEnd()) error("Unterminated escape sequence in string");
             switch (m_currentChar) {
                 case '"': value += '"'; break;
                 case '\\': value += '\\'; break;
                 case 'n': value += '\n'; break;
                 case 't': value += '\t'; break;
                 // Add other escapes (\r, \b, \f, \/, \uXXXX) if needed
                 default:
                     value += '\\'; // Keep unrecognized escapes literally? Or error?
                     value += m_currentChar;
                     // error("Invalid escape sequence: \\" + std::string(1, m_currentChar));
                     break;
             }
         } else {
             value += m_currentChar;
         }
         advance();
     }
     if (isAtEnd()) {
         error("Unterminated string literal");
     }
     advance(); // Consume closing '"'
     return value;
}

std::string Parser::parseSymbolOrKeyword() {
    std::string name;
    // Define valid symbol characters (more permissive than typical identifiers)
    auto isValidSymbolChar = [](char c) {
        return std::isalnum(c) || c == '-' || c == '_' || c == '?' || c == '!' || c == '+' || c == '*' || c == '/'; // Extend as needed
    };

    if (!isValidSymbolChar(m_currentChar) || std::isdigit(m_currentChar)) { // Symbols cannot start with a digit usually
        error("Invalid character for start of symbol or keyword");
    }

    while (!isAtEnd() && isValidSymbolChar(m_currentChar)) {
        name += m_currentChar;
        advance();
    }
    return name;
}


// --- Main Parse Function ---

Expr Parser::parse(const std::string& sourceCode) {
    m_source = sourceCode;
    m_position = 0;
    if (m_source.empty()) {
        return make_do({}); // Empty input parses to empty 'do' block
    }
    m_currentChar = m_source[0];

    std::vector<Expr> expressions;
    skipWhitespace();
    while (!isAtEnd()) {
         skipComment(); // Skip comments between expressions
         skipWhitespace();
         if (isAtEnd()) break; // Handle trailing whitespace/comments

        expressions.push_back(parseExpression());

        skipWhitespace();
        if (!isAtEnd()) {
            if (m_currentChar == ';') {
                advance(); // Consume separator
                skipWhitespace();
                 // Allow optional trailing semicolon
                if (isAtEnd()) break;
            } else {
                // If it's not EOF and not ';', it might be an implicit function call
                // or just the end of the input without a semicolon.
                // Depending on language rules, might error or assume end.
                // Let's assume separate expressions need ';'.
                 // error("Expected ';' or end of input after expression");
                 // Or treat as end if no more non-whitespace
                 bool onlyWhitespaceRemains = true;
                 size_t tempPos = m_position;
                 while(tempPos < m_source.length()) {
                      if (!std::isspace(m_source[tempPos])) {
                           if (m_source[tempPos] == '#') { // Allow comments at end
                                while(tempPos < m_source.length() && m_source[tempPos] != '\n') tempPos++;
                                if(tempPos < m_source.length()) tempPos++; // skip newline
                                continue;
                           }
                           onlyWhitespaceRemains = false;
                           break;
                      }
                      tempPos++;
                 }
                 if (!onlyWhitespaceRemains) {
                       error("Expected ';' or end of input after expression");
                 } else {
                     break; // Reached end effectively
                 }
            }
        }
    }

    // Return single expression directly, or wrap multiple in a 'do' block
    if (expressions.size() == 1) {
        return std::move(expressions[0]);
    } else {
        return make_do(std::move(expressions));
    }
}

// --- Grammar Rule Parsers (Placeholders/Simplified) ---
// A full implementation is complex and requires careful handling
// of operator precedence, associativity, and language grammar.

Expr Parser::parseExpression() {
    // Placeholder: Parses a primary expression (term) and handles potential
    // function calls or binary operators following it.
    // Uses Pratt parsing or similar precedence climbing for binary ops.
    Expr lhs = parseFactor(); // Start with highest precedence (unary ops, terms)

    // Loop to handle function calls and binary operators
    while (true) {
        skipWhitespace();
        // Check for function call syntax (e.g., term arg1 arg2)
        // This simple check assumes any subsequent term starts a call.
        // A real parser needs more robust lookahead or grammar rules.
         if (!isAtEnd() && (std::isalnum(m_currentChar) || m_currentChar == '(' || m_currentChar == '[' || m_currentChar == '{' || m_currentChar == '\'' || m_currentChar == '"')) {
              // Potential argument starting -> parse as application
               std::vector<Expr> args;
               // Parse arguments until a non-argument token is found (e.g., ';', ')', '}', EOF, operator)
               // This is highly simplified. A real parser would use precedence.
               while(!isAtEnd() && (std::isalnum(m_currentChar) || m_currentChar == '(' || m_currentChar == '[' || m_currentChar == '{' || m_currentChar == '\'' || m_currentChar == '"')) {
                     args.push_back(parseFactor()); // Parse arguments at high precedence
                     skipWhitespace();
               }
               if (!args.empty()) {
                   lhs = make_apply(std::move(lhs), std::move(args));
               } else {
                   break; // No more arguments, stop application parsing
               }
         }
        // Check for binary operators (placeholder)
        // else if (!isAtEnd() && isBinaryOperator(m_currentChar)) {
        //    lhs = parseBinaryOpRHS(0, std::move(lhs)); // Start precedence climbing
        // }
        else {
            break; // No call or binary operator found
        }
    }

    return lhs;
}


// Parses atoms (literals, symbols) and potentially parenthesized expressions
Expr Parser::parseAtom() {
    skipWhitespace();
    if (std::isdigit(m_currentChar) || (m_currentChar == '-' && std::isdigit(peek()))) {
        long long intPart = parseInteger();
        if (m_currentChar == '.') {
            advance(); // Consume '.'
            return make_float(parseFloat(intPart));
        } else {
            return make_int(intPart);
        }
    } else if (m_currentChar == '"') {
        advance(); // Consume opening '"'
        return make_string(parseStringLiteral());
    } else if (m_currentChar == '[') {
        return parseList();
    } else if (m_currentChar == '{') {
         // Could be dict or do block based on grammar
         return parseBlock(); // Assuming '{' always starts a do block for now
         // return parseDict();
    } else if (m_currentChar == '(') {
        return parseGroup();
    } else if (m_currentChar == '\'') {
         return parseQuote();
    } else if (std::isalpha(m_currentChar) || m_currentChar == '_') { // Start of symbol/keyword
        std::string name = parseSymbolOrKeyword();
        if (name == "True") return make_bool(true);
        if (name == "False") return make_bool(false);
        if (name == "None") return make_nil();
        // Check for keywords like 'if', 'let', 'fn' etc. here
        // if (name == "if") return parseIf(); // Better handled higher up
        // if (name == "let") return parseLet(); // Better handled higher up
        return make_symbol(std::move(name));
    }
    // Handle other potential atom starts (like operators treated as symbols?)
    else {
        error("Unexpected character encountered: '" + std::string(1, m_currentChar) + "'");
    }
    return make_nil(); // Should not be reached
}

Expr Parser::parseTerm() {
    // Placeholder: In a full grammar, this handles atoms and calls
    return parseAtom();
}

Expr Parser::parseFactor() {
     // Placeholder: Handles unary operators before terms
     skipWhitespace();
     if (m_currentChar == '-') {
         advance(); // Consume '-'
         // return make_neg(parseFactor()); // Recursive call
         return Expr{ std::make_shared<ExprNeg>(ExprNeg{parseFactor()}) }; // Simplified
     } else if (m_currentChar == '!') {
          advance();
          return Expr{ std::make_shared<ExprNot>(ExprNot{parseFactor()}) }; // Simplified
     }
     // If no unary operator, parse the term directly
     return parseTerm();
}


Expr Parser::parseList() {
    // Assumes opening '[' consumed
    advance();
    std::vector<Expr> items;
    skipWhitespace();
    if (m_currentChar == ']') { // Empty list
        advance();
        return make_list({});
    }
    while (true) {
        items.push_back(parseExpression());
        skipWhitespace();
        if (m_currentChar == ']') {
            advance();
            break;
        } else if (m_currentChar == ',') {
            advance();
            skipWhitespace();
            if (m_currentChar == ']') { // Allow trailing comma
                 advance();
                 break;
            }
        } else {
            error("Expected ',' or ']' in list literal");
        }
    }
    return make_list(std::move(items));
}


Expr Parser::parseGroup() {
     // Assumes opening '(' consumed
     advance();
     Expr expr = parseExpression();
     skipWhitespace();
     if (m_currentChar != ')') {
          error("Expected ')' to close group expression");
     }
     advance(); // Consume ')'
     return expr; // Groups don't usually create a node, just control precedence
}

Expr Parser::parseBlock() {
     // Assumes opening '{' consumed
     advance();
     std::vector<Expr> expressions;
     skipWhitespace();
     while (!isAtEnd() && m_currentChar != '}') {
          skipComment();
          skipWhitespace();
          if(m_currentChar == '}') break; // Handle empty or trailing whitespace

          expressions.push_back(parseExpression());
          skipWhitespace();
          if (m_currentChar == ';') {
               advance();
               skipWhitespace();
               if(m_currentChar == '}') break; // Allow trailing semicolon
          } else if (m_currentChar != '}') {
               // Require semicolon unless it's the last expression before '}'
               // Need lookahead or adjust loop condition
               bool onlyWhitespaceRemains = true;
               size_t tempPos = m_position;
               while(tempPos < m_source.length() && m_source[tempPos] != '}') {
                    if (!std::isspace(m_source[tempPos])) {
                         if (m_source[tempPos] == '#') {
                              while(tempPos < m_source.length() && m_source[tempPos] != '\n') tempPos++;
                              if(tempPos < m_source.length()) tempPos++;
                              continue;
                         }
                         onlyWhitespaceRemains = false;
                         break;
                    }
                    tempPos++;
               }
               if (!onlyWhitespaceRemains) {
                    error("Expected ';' or '}' after expression in block");
               }
               // If only whitespace/comments remain before '}', allow missing semicolon
          }
     }
     if (isAtEnd()) {
          error("Unterminated block, missing '}'");
     }
     advance(); // Consume '}'
     return make_do(std::move(expressions));
}

Expr Parser::parseQuote() {
     // Assumes opening '\'' consumed
     advance();
     Expr quoted = parseExpression();
     return Expr{ std::make_shared<ExprQuote>(ExprQuote{std::move(quoted)}) };
}


// --- Placeholder for other parse functions ---
// Expr Parser::parseDict() { /* ... */ }
// Expr Parser::parseIf() { /* ... */ }
// Expr Parser::parseLet() { /* ... */ }
// Expr Parser::parseFnProcMacro() { /* ... */ }
// Expr Parser::parseBinaryOpRHS(int exprPrec, Expr lhs) { /* ... Pratt parser logic ... */ }
// int Parser::getTokenPrecedence() { /* ... return precedence for current token ... */ }