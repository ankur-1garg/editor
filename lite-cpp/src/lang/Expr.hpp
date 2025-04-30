#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>     // For std::shared_ptr
#include <functional> // For std::function
#include <optional>   // Potentially for error types or results

// Forward declarations to break cycles and allow pointers in variant
class Environment;
class Editor; // Needed for BuiltinFuncType context

// Forward declarations for recursive Expr types stored in shared_ptr
struct ExprList;
struct ExprDict;
struct ExprSymbol;    // Explicit Symbol type might be clearer than just string
struct ExprQuote;
struct ExprNeg;
struct ExprNot;
struct ExprAdd;       // Binary operations
struct ExprSub;
struct ExprMul;
struct ExprDiv;
struct ExprRem;
// ... other binary ops like And, Or, Pow, Get, To if implementing ...
struct ExprIf;
struct ExprLet;       // Let binding
struct ExprAssign;    // Assignment to existing var
struct ExprDo;        // Block scope / sequence
struct ExprFn;        // Function definition (captures env)
struct ExprProc;      // Procedure definition (no env capture)
struct ExprMacro;     // Macro definition
struct ExprApply;     // Function/Proc/Macro Application
struct ExprTry;       // Try/Catch
struct ExprRaise;     // Raise exception
// ... other language constructs like For, While ...

// Type alias for Builtin functions
// Takes arguments, editor context, current environment -> returns result or throws
using BuiltinFuncType = std::function<struct Expr(
    const std::vector<struct Expr>& /* args */,
    Editor& /* editor context */,
    Environment& /* current env */
)>;

// Structure to hold information about a built-in function
struct BuiltinFunctionInfo {
    std::string name;
    BuiltinFuncType function;
    std::string helpShort; // Optional help text
    std::string helpLong;  // Optional detailed help

    // Need a way to compare/hash Builtins if used in maps/sets based on name?
    // For simplicity, comparison might rely on pointer identity or name.
    bool operator<(const BuiltinFunctionInfo& other) const { return name < other.name; }
};


// --- The main Expression Variant ---
// Holds all possible types of values and AST nodes in the language.
using Expr = std::variant<
    // Primitive/Simple Types
    std::monostate,                                 // Represents 'None' or 'Nil'
    long long,                                      // Integer type
    double,                                         // Floating point type
    bool,                                           // Boolean type
    std::string,                                    // String literal type

    // Boxed Pointers to specific AST node structures or builtins
    std::shared_ptr<ExprSymbol>,                    // Explicit Symbol type
    std::shared_ptr<BuiltinFunctionInfo>,           // Reference to a Builtin
    std::shared_ptr<ExprList>,
    std::shared_ptr<ExprDict>,
    std::shared_ptr<ExprQuote>,
    std::shared_ptr<ExprNeg>,
    std::shared_ptr<ExprNot>,
    std::shared_ptr<ExprAdd>,
    std::shared_ptr<ExprSub>,
    std::shared_ptr<ExprMul>,
    std::shared_ptr<ExprDiv>,
    std::shared_ptr<ExprRem>,
    std::shared_ptr<ExprIf>,
    std::shared_ptr<ExprLet>,
    std::shared_ptr<ExprAssign>,
    std::shared_ptr<ExprDo>,
    std::shared_ptr<ExprFn>,      // Closure (captures Environment)
    std::shared_ptr<ExprProc>,    // Simple Procedure (no capture)
    std::shared_ptr<ExprMacro>,   // Macro
    std::shared_ptr<ExprApply>,
    std::shared_ptr<ExprTry>,
    std::shared_ptr<ExprRaise>
    // Add pointers for other operation/construct types (And, Or, Get, To, For, While...)
>;

// --- Structure Definitions for Boxed Types ---
// These hold the actual data for complex/recursive expression types.

struct ExprSymbol {
    std::string name;
    bool operator<(const ExprSymbol& other) const { return name < other.name; }
};

struct ExprList {
    std::vector<Expr> items;
};

struct ExprDict {
    // Requires Expr to be comparable (implement operator<)
    std::map<Expr, Expr> items;
};

struct ExprQuote {
    Expr quotedExpr;
};

// Unary Ops
struct ExprNeg { Expr operand; };
struct ExprNot { Expr operand; };

// Binary Ops
struct ExprAdd { Expr left; Expr right; };
struct ExprSub { Expr left; Expr right; };
struct ExprMul { Expr left; Expr right; };
struct ExprDiv { Expr left; Expr right; };
struct ExprRem { Expr left; Expr right; };
// ... Other binary ops ...

// Control Flow / Scoping
struct ExprIf { Expr condition; Expr thenBranch; Expr elseBranch; };
struct ExprLet { std::shared_ptr<ExprSymbol> var; Expr value; Expr body; }; // In body
struct ExprAssign { std::shared_ptr<ExprSymbol> var; Expr value; }; // Assign existing
struct ExprDo { std::vector<Expr> expressions; }; // Sequence/Block

// Function Types
struct ExprFnParams { // Helper for param lists if complex args needed later
    std::vector<std::shared_ptr<ExprSymbol>> params;
    // Optional: support for varargs, default values etc.
};

struct ExprFn {
    ExprFnParams params;
    Expr body;
    std::shared_ptr<Environment> capturedEnv; // Closure: captures definition environment
};

struct ExprProc { // No captured environment
    ExprFnParams params;
    Expr body;
};

struct ExprMacro { // Evaluated differently
    ExprFnParams params;
    Expr body;
};

// Application
struct ExprApply {
    Expr function; // The Expr being called (Symbol, Fn, Proc, Macro, Builtin)
    std::vector<Expr> args;
};

// Error Handling
struct ExprTry { Expr tryBody; Expr catchBody; /* Maybe var for error? */ };
struct ExprRaise { Expr errorValue; };


// --- Helper Functions ---

// Convenience factory functions (implement in Expr.cpp)
Expr make_nil();
Expr make_int(long long val);
Expr make_float(double val);
Expr make_bool(bool val);
Expr make_string(std::string val);
Expr make_symbol(std::string name);
// ... factories for list, dict, add, if, let, fn, apply etc. ...
Expr make_list(std::vector<Expr> items);
Expr make_do(std::vector<Expr> expressions);
Expr make_apply(Expr fn, std::vector<Expr> args);


// For using Expr as a std::map key (needed for ExprDict)
bool operator<(const Expr& lhs, const Expr& rhs);

// Convert Expr to a printable string representation (implement in Expr.cpp)
std::string toString(const Expr& expr);

// Check the type of an Expr (implement in Expr.cpp)
bool isTruthy(const Expr& expr); // For use in 'if' conditions etc.