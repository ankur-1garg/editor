#pragma once

#include "Expr.hpp" // Needs Expr definition and BuiltinFuncType
#include "Environment.hpp" // Needs Environment

#include <string>
#include <vector>
#include <functional> // For std::function
#include <memory>     // For std::shared_ptr

// Forward declaration
class Editor; // Editor context needed by builtins

// --- Helper to Add Builtins ---

// Registers a C++ function as a callable builtin in the given environment.
// Creates a BuiltinFunctionInfo and wraps it in an Expr variant.
void addBuiltinFunction(
    Environment& env,                  // Environment to define the builtin in
    const std::string& name,           // Name accessible from the script
    BuiltinFuncType func,              // The C++ function to call
    const std::string& helpShort = "", // Optional short help text
    const std::string& helpLong = ""   // Optional long help text
);

// --- Declare the actual C++ builtin functions ---
// These functions match the BuiltinFuncType signature.
// They are declared here and implemented in Builtin.cpp.

// Example Builtin Function Declarations:
Expr builtin_print(const std::vector<Expr>& args, Editor& editor, Environment& env);
Expr builtin_add(const std::vector<Expr>& args, Editor& editor, Environment& env);
Expr builtin_insert(const std::vector<Expr>& args, Editor& editor, Environment& env);
Expr builtin_delete(const std::vector<Expr>& args, Editor& editor, Environment& env);
Expr builtin_move(const std::vector<Expr>& args, Editor& editor, Environment& env);
Expr builtin_goto(const std::vector<Expr>& args, Editor& editor, Environment& env);
Expr builtin_get_select(const std::vector<Expr>& args, Editor& editor, Environment& env);
Expr builtin_select(const std::vector<Expr>& args, Editor& editor, Environment& env);
Expr builtin_unselect(const std::vector<Expr>& args, Editor& editor, Environment& env);
Expr builtin_new_buf(const std::vector<Expr>& args, Editor& editor, Environment& env);
Expr builtin_set_buf(const std::vector<Expr>& args, Editor& editor, Environment& env);
Expr builtin_get_cur_buf(const std::vector<Expr>& args, Editor& editor, Environment& env);
// ... Declare other builtins (sub, mul, div, buffer ops, etc.) ...

// Helper within builtins to evaluate arguments (calls Interpreter)
Expr evalArg(const Expr& arg, Editor& editor, Environment& env);

// Helper within builtins to check arg count and types (basic example)
// Throws std::runtime_error on failure
void checkArgCount(const std::string& funcName, const std::vector<Expr>& args, size_t expected);
template <typename T> // T is the std::variant alternative type (e.g., long long, std::string)
const T& expectArgType(const std::string& funcName, const Expr& arg, size_t argIndex);
// Overload for shared_ptr types
template <typename T> // T is the struct type (e.g., ExprSymbol, ExprList)
const std::shared_ptr<T>& expectArgSharedPtrType(const std::string& funcName, const Expr& arg, size_t argIndex);