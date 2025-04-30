#pragma once

#include "Expr.hpp" // Requires Expr definition

#include <map>
#include <string>
#include <memory>   // For std::shared_ptr
#include <optional>

// Manages scopes for variables and functions during script evaluation.
// Supports nested scopes via parent pointers.
class Environment : public std::enable_shared_from_this<Environment> {
private:
    // Bindings in the current scope (Variable/Function Name -> Value/Definition)
    // Using string for symbol name is simpler than ExprSymbol here.
    std::map<std::string, Expr> m_scope;

    // Pointer to the enclosing (parent) scope, null for the global scope
    std::shared_ptr<Environment> m_parent;

public:
    // Constructor for creating a new scope, optionally nested within a parent.
    Environment(std::shared_ptr<Environment> parent = nullptr);

    // --- Scope Operations ---

    // Define or update a variable/function ONLY in the *current* scope.
    // Used for 'let' bindings or defining new functions.
    void define(const std::string& name, Expr value);

    // Assign a value to an *existing* variable.
    // Searches the current scope and then parent scopes recursively.
    // Returns true if assignment was successful (variable was found), false otherwise.
    bool assign(const std::string& name, Expr value);

    // Look up the value of a variable/function.
    // Searches the current scope and then parent scopes recursively.
    // Returns the found Expr, or nullopt if not found in any scope.
    std::optional<Expr> lookup(const std::string& name);

    // --- Accessors ---
    std::shared_ptr<Environment> getParent() const { return m_parent; }

    // Get a reference to the internal map (e.g., for inspection or merging)
    const std::map<std::string, Expr>& getScopeMap() const { return m_scope; }
    // Non-const version (use with caution)
    std::map<std::string, Expr>& getScopeMapMutable() { return m_scope; }

};