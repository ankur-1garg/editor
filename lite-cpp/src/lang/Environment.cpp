#include "Environment.hpp"
#include <optional> // Ensure included

Environment::Environment(std::shared_ptr<Environment> parent)
    : m_parent(std::move(parent)) {}

// Define or update ONLY in the current scope
void Environment::define(const std::string& name, Expr value) {
    // This will insert if new, or update if it exists in the current scope.
    m_scope[name] = std::move(value);
}

// Assign to existing variable in current or parent scope
bool Environment::assign(const std::string& name, Expr value) {
    auto it = m_scope.find(name);
    if (it != m_scope.end()) {
        // Found in the current scope, update it.
        it->second = std::move(value);
        return true;
    } else if (m_parent) {
        // Not in current scope, try assigning in the parent scope.
        return m_parent->assign(name, std::move(value)); // Recursive call
    } else {
        // Not found in current scope and no parent exists. Assignment fails.
        return false;
    }
}

// Lookup variable in current or parent scope
std::optional<Expr> Environment::lookup(const std::string& name) {
    auto it = m_scope.find(name);
    if (it != m_scope.end()) {
        // Found in the current scope.
        return it->second;
    } else if (m_parent) {
        // Not found here, try the parent scope.
        return m_parent->lookup(name); // Recursive call
    } else {
        // Not found in current scope and no parent exists.
        return std::nullopt;
    }
}