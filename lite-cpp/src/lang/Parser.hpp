#pragma once

#include "Expr.hpp" // Requires Expr definition
#include <string>
#include <vector>
#include <stdexcept> // For parse errors

// Basic Recursive Descent Parser (Placeholder Structure)
class Parser {
private:
    // Internal state for the parser
    std::string m_source;
    size_t m_position = 0; // Current position in m_source
    char m_currentChar = 0; // Lookahead character

    // --- Lexer-like Helpers ---
    void advance();            // Move to the next character
    void skipWhitespace();     // Skip spaces, tabs, newlines
    void skipComment();        // Skip '#' style comments
    char peek() const;         // Look at the next character without advancing
    bool isAtEnd() const;
    // Read specific tokens
    long long parseInteger();
    double parseFloat(long long intPart); // Needs integer part from parseInteger
    std::string parseStringLiteral();
    std::string parseSymbolOrKeyword(); // Reads symbols/keywords like 'let', 'if', '+', 'my-var'

    // --- Grammar Rule Parsers (Recursive Descent) ---
    Expr parseExpression();    // Main entry point for parsing an expression
    Expr parseAtom();          // Parses literals (num, string, bool, none) and symbols
    Expr parseList();          // Parses [...]
    Expr parseDict();          // Parses {...}
    Expr parseGroup();         // Parses (...)
    Expr parseBlock();         // Parses {...} as a 'do' block
    Expr parseQuote();         // Parses 'expr
    Expr parseCall(Expr function); // Parses function application (fn arg1 arg2)
    Expr parseTerm();          // Parses atoms, lists, groups, blocks, quotes
    Expr parseFactor();        // Handles unary ops like '-' or '!'
    Expr parseBinaryOpRHS(int exprPrec, Expr lhs); // Handles binary operators based on precedence
    Expr parseIf();            // Parses 'if condition then true_branch else false_branch'
    Expr parseLet();           // Parses 'let var = val in body' or 'let var = val' (assignment)
    Expr parseFnProcMacro();   // Parses 'params -> body', 'params => body', 'params ~> body'
    Expr parseDoBlock();       // Parses '{ expr1; expr2 }'


    // Helper to get operator precedence
    int getTokenPrecedence();

    // Error reporting
    void error(const std::string& message);

public:
    Parser() = default;

    // Parses the entire source code string.
    // Returns the top-level expression (often a 'do' block).
    // Throws std::runtime_error on syntax error.
    Expr parse(const std::string& sourceCode);
};