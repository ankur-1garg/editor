#include "Expr.hpp"
#include "Environment.hpp" // May need Environment definition for toString of Fn

#include <sstream>    // For toString implementation
#include <stdexcept>  // For errors in helper functions
#include <utility>    // For std::move
#include <vector>
#include <map>

// --- Factory Functions ---

Expr make_nil() { return std::monostate{}; }
Expr make_int(long long val) { return val; }
Expr make_float(double val) { return val; }
Expr make_bool(bool val) { return val; }
Expr make_string(std::string val) { return val; }
Expr make_symbol(std::string name) { return std::make_shared<ExprSymbol>(ExprSymbol{std::move(name)}); }

Expr make_list(std::vector<Expr> items) {
    return std::make_shared<ExprList>(ExprList{std::move(items)});
}
Expr make_dict(std::map<Expr, Expr> items) {
     return std::make_shared<ExprDict>(ExprDict{std::move(items)});
}
Expr make_do(std::vector<Expr> expressions) {
    return std::make_shared<ExprDo>(ExprDo{std::move(expressions)});
}
Expr make_apply(Expr fn, std::vector<Expr> args) {
     return std::make_shared<ExprApply>(ExprApply{std::move(fn), std::move(args)});
}
// ... Implement other factory functions as needed ...


// --- Operator< for Map Keys ---

bool operator<(const Expr& lhs, const Expr& rhs) {
    // Compare by index first (efficient)
    if (lhs.index() != rhs.index()) {
        return lhs.index() < rhs.index();
    }

    // If indices are the same, compare the actual values
    // Use std::visit for type-safe comparison
    return std::visit(
        [&](const auto& lhs_val) {
            // We know rhs has the same type because indices match
            const auto& rhs_val = std::get<std::decay_t<decltype(lhs_val)>>(rhs);

            // Handle comparisons for specific types
            using T = std::decay_t<decltype(lhs_val)>;

            if constexpr (std::is_same_v<T, std::monostate>) {
                return false; // None == None
            } else if constexpr (std::is_same_v<T, std::shared_ptr<ExprSymbol>>) {
                return *lhs_val < *rhs_val; // Compare symbols by name via ExprSymbol::operator<
            } else if constexpr (std::is_same_v<T, std::shared_ptr<BuiltinFunctionInfo>>) {
                // Compare builtins by name? Or pointer address? Let's use name.
                return lhs_val->name < rhs_val->name;
            } else if constexpr (std::is_base_of_v<std::shared_ptr<void>, T>) {
                // Generic shared_ptr comparison (by pointer address) - might not be stable
                // return lhs_val < rhs_val;
                // It's better to define operator< for the structs (List, Dict etc.) if deep comparison is needed.
                // For map keys, pointer comparison (identity) is often sufficient and simpler.
                 return lhs_val.get() < rhs_val.get(); // Compare raw pointers
            }
            else {
                // Default comparison for arithmetic types, bool, string
                // Assumes operator< is defined for these types.
                return lhs_val < rhs_val;
            }
        },
        lhs);
}

// --- toString Implementation ---

// Helper struct (visitor) for toString
struct ExprToStringVisitor {
    std::string operator()(std::monostate) const { return "None"; }
    std::string operator()(long long v) const { return std::to_string(v); }
    std::string operator()(double v) const {
        std::ostringstream oss;
        oss << v;
        return oss.str();
    }
    std::string operator()(bool v) const { return v ? "True" : "False"; }
    std::string operator()(const std::string& v) const {
        // Basic string escaping for display
        std::ostringstream oss;
        oss << '"';
        for (char c : v) {
            switch (c) {
                case '"': oss << "\\\""; break;
                case '\\': oss << "\\\\"; break;
                case '\n': oss << "\\n"; break;
                case '\t': oss << "\\t"; break;
                // Add other escapes if needed
                default: oss << c; break;
            }
        }
        oss << '"';
        return oss.str();
    }
    std::string operator()(const std::shared_ptr<ExprSymbol>& v) const {
        return v ? v->name : "<null_symbol>";
    }
     std::string operator()(const std::shared_ptr<BuiltinFunctionInfo>& v) const {
        return v ? "<builtin:" + v->name + ">" : "<null_builtin>";
    }
    std::string operator()(const std::shared_ptr<ExprList>& v) const {
        if (!v) return "<null_list>";
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < v->items.size(); ++i) {
            oss << toString(v->items[i]); // Recursive call
            if (i < v->items.size() - 1) {
                oss << ", ";
            }
        }
        oss << "]";
        return oss.str();
    }
    std::string operator()(const std::shared_ptr<ExprDict>& v) const {
         if (!v) return "<null_dict>";
        std::ostringstream oss;
        oss << "{";
        bool first = true;
        for (const auto& pair : v->items) {
            if (!first) {
                oss << ", ";
            }
            oss << toString(pair.first) << ": " << toString(pair.second);
            first = false;
        }
        oss << "}";
        return oss.str();
    }
     std::string operator()(const std::shared_ptr<ExprQuote>& v) const {
        return v ? "'" + toString(v->quotedExpr) : "<null_quote>";
    }
     std::string operator()(const std::shared_ptr<ExprNeg>& v) const {
         return v ? "-" + toString(v->operand) : "<null_neg>";
     }
      std::string operator()(const std::shared_ptr<ExprNot>& v) const {
         return v ? "!" + toString(v->operand) : "<null_not>";
     }
     std::string operator()(const std::shared_ptr<ExprAdd>& v) const {
         return v ? "(" + toString(v->left) + " + " + toString(v->right) + ")" : "<null_add>";
     }
     std::string operator()(const std::shared_ptr<ExprSub>& v) const {
          return v ? "(" + toString(v->left) + " - " + toString(v->right) + ")" : "<null_sub>";
     }
     std::string operator()(const std::shared_ptr<ExprMul>& v) const {
         return v ? "(" + toString(v->left) + " * " + toString(v->right) + ")" : "<null_mul>";
     }
     std::string operator()(const std::shared_ptr<ExprDiv>& v) const {
          return v ? "(" + toString(v->left) + " / " + toString(v->right) + ")" : "<null_div>";
     }
     std::string operator()(const std::shared_ptr<ExprRem>& v) const {
          return v ? "(" + toString(v->left) + " % " + toString(v->right) + ")" : "<null_rem>";
     }
     std::string operator()(const std::shared_ptr<ExprIf>& v) const {
         return v ? "(if " + toString(v->condition) + " then " + toString(v->thenBranch) + " else " + toString(v->elseBranch) + ")" : "<null_if>";
     }
      std::string operator()(const std::shared_ptr<ExprLet>& v) const {
         return v ? "(let " + (v->var ? v->var->name : "<null_var>") + " = " + toString(v->value) + " in " + toString(v->body) + ")" : "<null_let>";
     }
      std::string operator()(const std::shared_ptr<ExprAssign>& v) const {
          return v ? "(" + (v->var ? v->var->name : "<null_var>") + " = " + toString(v->value) + ")" : "<null_assign>";
      }
       std::string operator()(const std::shared_ptr<ExprDo>& v) const {
           if (!v) return "<null_do>";
           std::ostringstream oss;
           oss << "{";
           for (size_t i = 0; i < v->expressions.size(); ++i) {
               oss << toString(v->expressions[i]);
               if (i < v->expressions.size() - 1) oss << "; ";
           }
           oss << "}";
           return oss.str();
       }
       std::string operator()(const std::shared_ptr<ExprFn>& v) const {
            if (!v) return "<null_fn>";
             std::ostringstream oss;
             oss << "<fn (";
              for (size_t i = 0; i < v->params.params.size(); ++i) {
                   oss << (v->params.params[i] ? v->params.params[i]->name : "?");
                   if (i < v->params.params.size() - 1) oss << ", ";
              }
             oss << ")>"; // Don't print body or env usually
             return oss.str();
       }
        std::string operator()(const std::shared_ptr<ExprProc>& v) const {
             if (!v) return "<null_proc>";
              std::ostringstream oss;
             oss << "<proc (";
              for (size_t i = 0; i < v->params.params.size(); ++i) {
                   oss << (v->params.params[i] ? v->params.params[i]->name : "?");
                   if (i < v->params.params.size() - 1) oss << ", ";
              }
             oss << ")>";
             return oss.str();
        }
        std::string operator()(const std::shared_ptr<ExprMacro>& v) const {
             if (!v) return "<null_macro>";
              std::ostringstream oss;
             oss << "<macro (";
              for (size_t i = 0; i < v->params.params.size(); ++i) {
                   oss << (v->params.params[i] ? v->params.params[i]->name : "?");
                   if (i < v->params.params.size() - 1) oss << ", ";
              }
             oss << ")>";
             return oss.str();
        }
        std::string operator()(const std::shared_ptr<ExprApply>& v) const {
             if (!v) return "<null_apply>";
             std::ostringstream oss;
             oss << "(" << toString(v->function);
             for(const auto& arg : v->args) {
                 oss << " " << toString(arg);
             }
             oss << ")";
             return oss.str();
        }
        std::string operator()(const std::shared_ptr<ExprTry>& v) const {
             return v ? "(try " + toString(v->tryBody) + " catch " + toString(v->catchBody) + ")" : "<null_try>";
        }
        std::string operator()(const std::shared_ptr<ExprRaise>& v) const {
            return v ? "(raise " + toString(v->errorValue) + ")" : "<null_raise>";
        }
        // Add cases for other variant types...

};

std::string toString(const Expr& expr) {
    return std::visit(ExprToStringVisitor{}, expr);
}

// --- isTruthy Implementation ---

bool isTruthy(const Expr& expr) {
    // Define language's truthiness rules.
    // Typically: False and None are false, everything else is true.
    // Could also consider 0 and empty strings/lists/dicts as false.
    return std::visit(
        [](const auto& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return false; // None is false
            } else if constexpr (std::is_same_v<T, bool>) {
                return val;   // Boolean value itself
            } else if constexpr (std::is_same_v<T, long long>) {
                return val != 0; // 0 is false, others true? (Or only bool false?) Let's stick to None/False being false.
                // return true; // Simpler: only None/False are false.
            } else if constexpr (std::is_same_v<T, double>) {
                 return val != 0.0; // Or just true?
                 // return true; // Simpler: only None/False are false.
            } else if constexpr (std::is_same_v<T, std::string>) {
                 // return !val.empty(); // Empty string false?
                 return true; // Simpler: only None/False are false.
            } else if constexpr (std::is_same_v<T, std::shared_ptr<ExprList>>) {
                 // return val && !val->items.empty(); // Empty list false?
                 return true; // Simpler: only None/False are false.
            } else if constexpr (std::is_same_v<T, std::shared_ptr<ExprDict>>) {
                  // return val && !val->items.empty(); // Empty dict false?
                  return true; // Simpler: only None/False are false.
            }
             else {
                return true; // All other types (Symbols, Builtins, Functions, etc.) are true
            }
        },
        expr);
}