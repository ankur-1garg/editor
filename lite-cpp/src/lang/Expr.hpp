#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>     // For std::shared_ptr
#include <functional> // For std::function
#include <optional>   // Potentially for error types or results

// Forward declaration for external classes
class Environment;
class Editor; // Needed for BuiltinFuncType context

// Forward declaration of Expr type
struct Expr;

// Forward declarations for all expression types to allow circular references
struct ExprSymbol;
struct ExprList;
struct ExprDict;
struct ExprQuote;
struct ExprNeg;
struct ExprNot;
struct ExprAdd;
struct ExprSub;
struct ExprMul;
struct ExprDiv;
struct ExprRem;
struct ExprIf;
struct ExprLet;
struct ExprAssign;
struct ExprDo;
struct ExprFn;
struct ExprProc;
struct ExprMacro;
struct ExprApply;
struct ExprTry;
struct ExprRaise;
struct BuiltinFunctionInfo;
struct ExprFnParams;

// Define comparison operators before they're used in containers
// Note: ExprSymbol already has operator< as a member function
bool operator<(const Expr& lhs, const Expr& rhs);

// --- Simple types that don't directly depend on Expr ---

// Symbol definition
struct ExprSymbol {
    std::string name;
    bool operator<(const ExprSymbol& other) const { return name < other.name; }
};

// Function parameter list definition - doesn't need full Expr
struct ExprFnParams {
    std::vector<std::shared_ptr<ExprSymbol>> params;
    // Optional: support for varargs, default values etc.
};

// Type alias for Builtin functions - using forward declared Expr
using BuiltinFuncType = std::function<Expr(
    const std::vector<Expr>& /* args */,
    Editor& /* editor context */,
    Environment& /* current env */
)>;

// Structure to hold information about a built-in function
struct BuiltinFunctionInfo {
    std::string name;
    BuiltinFuncType function;
    std::string helpShort; // Optional help text
    std::string helpLong;  // Optional detailed help
    
    bool operator<(const BuiltinFunctionInfo& other) const { return name < other.name; }
};

// Define the internal variant type used by Expr
// Note: this is a type alias, not a struct
using ExprVariant = std::variant<
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

// Now define the Expr class that wraps the variant
struct Expr {
private:
    ExprVariant value;

public:
    // Default constructor creates a nil value
    Expr() : value(std::monostate{}) {}

    // Constructors for each variant type
    Expr(std::monostate nil) : value(nil) {}
    Expr(long long i) : value(i) {}
    Expr(int i) : value(static_cast<long long>(i)) {} // Convenience constructor
    Expr(double d) : value(d) {}
    Expr(bool b) : value(b) {}
    Expr(const char* s) : value(std::string(s)) {} // Convenience constructor
    Expr(std::string s) : value(std::move(s)) {}
    
    // Constructors for boxed pointer types
    Expr(std::shared_ptr<ExprSymbol> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<BuiltinFunctionInfo> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprList> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprDict> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprQuote> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprNeg> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprNot> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprAdd> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprSub> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprMul> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprDiv> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprRem> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprIf> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprLet> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprAssign> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprDo> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprFn> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprProc> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprMacro> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprApply> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprTry> ptr) : value(std::move(ptr)) {}
    Expr(std::shared_ptr<ExprRaise> ptr) : value(std::move(ptr)) {}

    // Constructor from the internal variant type
    Expr(ExprVariant v) : value(std::move(v)) {}

    // Access the internal variant
    const ExprVariant& getVariant() const { return value; }
    ExprVariant& getVariant() { return value; }

    // Type checking
    template<typename T>
    bool is() const {
        return std::holds_alternative<T>(value);
    }

    // Value retrieval
    template<typename T>
    const T& as() const {
        return std::get<T>(value);
    }

    template<typename T>
    T& as() {
        return std::get<T>(value);
    }

    // Visitor pattern support
    template<typename Visitor>
    auto visit(Visitor&& visitor) const {
        return std::visit(std::forward<Visitor>(visitor), value);
    }

    template<typename Visitor>
    auto visit(Visitor&& visitor) {
        return std::visit(std::forward<Visitor>(visitor), value);
    }

    // Comparison operators
    bool operator==(const Expr& other) const {
        return value == other.value;
    }

    bool operator!=(const Expr& other) const {
        return value != other.value;
    }

    // Let the comparison operator be defined externally
    friend bool operator<(const Expr& lhs, const Expr& rhs);
};

// Implement the comparison operator for Expr
inline bool operator<(const Expr& lhs, const Expr& rhs) {
    return lhs.value < rhs.value;
}

// --- Now that Expr is defined, define the structures that use it ---

// Complex expression types that contain Expr members
struct ExprList {
    std::vector<Expr> items;
    
    // Constructor for convenience
    ExprList() = default;
    explicit ExprList(std::vector<Expr> i) : items(std::move(i)) {}
};

struct ExprDict {
    std::map<Expr, Expr> items;
    
    // Constructor for convenience
    ExprDict() = default;
    explicit ExprDict(std::map<Expr, Expr> i) : items(std::move(i)) {}
};

struct ExprQuote {
    Expr quotedExpr;
    
    // Constructor for convenience
    ExprQuote() = default;
    explicit ExprQuote(Expr e) : quotedExpr(std::move(e)) {}
};

// Unary Ops
struct ExprNeg { 
    Expr operand; 
    explicit ExprNeg(Expr e) : operand(std::move(e)) {}
};

struct ExprNot { 
    Expr operand; 
    explicit ExprNot(Expr e) : operand(std::move(e)) {}
};

// Binary Ops
struct ExprAdd { 
    Expr left; 
    Expr right; 
    ExprAdd(Expr l, Expr r) : left(std::move(l)), right(std::move(r)) {}
};

struct ExprSub { 
    Expr left; 
    Expr right; 
    ExprSub(Expr l, Expr r) : left(std::move(l)), right(std::move(r)) {}
};

struct ExprMul { 
    Expr left; 
    Expr right; 
    ExprMul(Expr l, Expr r) : left(std::move(l)), right(std::move(r)) {}
};

struct ExprDiv { 
    Expr left; 
    Expr right; 
    ExprDiv(Expr l, Expr r) : left(std::move(l)), right(std::move(r)) {}
};

struct ExprRem { 
    Expr left; 
    Expr right; 
    ExprRem(Expr l, Expr r) : left(std::move(l)), right(std::move(r)) {}
};

// Control Flow / Scoping
struct ExprIf { 
    Expr condition; 
    Expr thenBranch; 
    Expr elseBranch; 
    
    ExprIf(Expr c, Expr t, Expr e) : condition(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {}
};

struct ExprLet { 
    std::shared_ptr<ExprSymbol> var; 
    Expr value; 
    Expr body; 
    
    ExprLet(std::shared_ptr<ExprSymbol> v, Expr val, Expr b) : 
        var(std::move(v)), value(std::move(val)), body(std::move(b)) {}
}; // In body

struct ExprAssign { 
    std::shared_ptr<ExprSymbol> var; 
    Expr value; 
    
    ExprAssign(std::shared_ptr<ExprSymbol> v, Expr val) : 
        var(std::move(v)), value(std::move(val)) {}
}; // Assign existing

struct ExprDo { 
    std::vector<Expr> expressions; 
    
    ExprDo() = default;
    explicit ExprDo(std::vector<Expr> e) : expressions(std::move(e)) {}
}; // Sequence/Block

struct ExprFn {
    ExprFnParams params;
    Expr body;
    std::shared_ptr<Environment> capturedEnv; // Closure: captures definition environment
    
    ExprFn(ExprFnParams p, Expr b, std::shared_ptr<Environment> env = nullptr) : 
        params(std::move(p)), body(std::move(b)), capturedEnv(std::move(env)) {}
};

struct ExprProc { // No captured environment
    ExprFnParams params;
    Expr body;
    
    ExprProc(ExprFnParams p, Expr b) : 
        params(std::move(p)), body(std::move(b)) {}
};

struct ExprMacro { // Evaluated differently
    ExprFnParams params;
    Expr body;
    
    ExprMacro(ExprFnParams p, Expr b) : 
        params(std::move(p)), body(std::move(b)) {}
};

// Application
struct ExprApply {
    Expr function; // The Expr being called (Symbol, Fn, Proc, Macro, Builtin)
    std::vector<Expr> args;
    
    ExprApply(Expr f, std::vector<Expr> a) : 
        function(std::move(f)), args(std::move(a)) {}
};

// Error Handling
struct ExprTry { 
    Expr tryBody; 
    Expr catchBody; /* Maybe var for error? */ 
    
    ExprTry(Expr t, Expr c) : 
        tryBody(std::move(t)), catchBody(std::move(c)) {}
};

struct ExprRaise { 
    Expr errorValue; 
    
    explicit ExprRaise(Expr e) : errorValue(std::move(e)) {}
};

// --- Helper Functions ---

// Convenience factory functions (implement in Expr.cpp)
inline Expr make_nil() { return Expr(std::monostate{}); }
inline Expr make_int(long long val) { return Expr(val); }
inline Expr make_float(double val) { return Expr(val); }
inline Expr make_bool(bool val) { return Expr(val); }
inline Expr make_string(std::string val) { return Expr(std::move(val)); }
inline Expr make_symbol(std::string name) { 
    return Expr(std::make_shared<ExprSymbol>(ExprSymbol{std::move(name)})); 
}

// ... factories for list, dict, add, if, let, fn, apply etc. ...
inline Expr make_list(std::vector<Expr> items) {
    return Expr(std::make_shared<ExprList>(std::move(items)));
}

inline Expr make_do(std::vector<Expr> expressions) {
    return Expr(std::make_shared<ExprDo>(std::move(expressions)));
}

inline Expr make_apply(Expr fn, std::vector<Expr> args) {
    return Expr(std::make_shared<ExprApply>(std::move(fn), std::move(args)));
}

// Additional factory functions for other expression types
inline Expr make_add(Expr left, Expr right) {
    return Expr(std::make_shared<ExprAdd>(std::move(left), std::move(right)));
}

inline Expr make_sub(Expr left, Expr right) {
    return Expr(std::make_shared<ExprSub>(std::move(left), std::move(right)));
}

inline Expr make_mul(Expr left, Expr right) {
    return Expr(std::make_shared<ExprMul>(std::move(left), std::move(right)));
}

inline Expr make_div(Expr left, Expr right) {
    return Expr(std::make_shared<ExprDiv>(std::move(left), std::move(right)));
}

inline Expr make_if(Expr condition, Expr thenBranch, Expr elseBranch) {
    return Expr(std::make_shared<ExprIf>(std::move(condition), std::move(thenBranch), std::move(elseBranch)));
}

// Convert Expr to a printable string representation (implement in Expr.cpp)
std::string toString(const Expr& expr);

// Check the type of an Expr (implement in Expr.cpp)
bool isTruthy(const Expr& expr); // For use in 'if' conditions etc.
