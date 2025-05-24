#include "Expr.hpp"
#include "Environment.hpp" // May need Environment definition for toString of Fn

#include <sstream>    // For toString implementation
#include <stdexcept>  // For errors in helper functions
#include <utility>    // For std::move

    // Use std::visit for type-safe comparison
    return std::visit(
        [&](const auto& lhs_val) {
            // We know rhs has the same type because indices match
            const auto& rhs_val = std::get<std::decay_t<decltype(lhs_val)>>(rhs.value);

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
        lhs.value);
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

struct ExprToStringVisitor {
    std::string operator()(const std::monostate&) const {
        return "nil";
    }
    std::string operator()(int v) const {
        return std::to_string(v);
    }
    std::string operator()(const std::string& v) const {
        return '"' + v + '"';
    }
    std::string operator()(const std::shared_ptr<Expr>& v) const {
        return v ? toString(*v) : "<null>";
    }
};

struct ExprToStringVisitor {
    std::string operator()(const std::monostate&) const {
        return "nil";
    }
    std::string operator()(int v) const {
        return std::to_string(v);
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
};

// Convert Expr to string
std::string toString(const Expr& expr) {
    return std::visit(ExprToStringVisitor{}, expr);
}

Expr make_nil() { return Expr(std::monostate{}); }

Expr make_number(double value) { return Expr(value); }

Expr make_string(const std::string& str) { return Expr(str); }

Expr make_symbol(const std::string& name) { return Expr(name); }

Expr make_list(const std::vector<Expr>& elements) { return Expr(elements); }

Expr make_vector(const std::vector<Expr>& elements) { return Expr(elements); }

Expr make_map(const std::map<Expr, Expr>& elements) { return Expr(elements); }

Expr make_function(const std::function<Expr(const std::vector<Expr>&)>& func) {
    return Expr(func);
}
// Comparison operators
bool operator==(const Expr& lhs, const Expr& rhs) {
    return std::visit([](const auto& l, const auto& r) {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;
        if constexpr (std::is_same_v<L, R>) {
            return l == r;
        }
        return false;
    }, lhs.value, rhs.value);
}

bool operator!=(const Expr& lhs, const Expr& rhs) {
    return !(lhs == rhs);
}

// Less than operator
bool operator<(const Expr& lhs, const Expr& rhs) {
    return std::visit([](const auto& l, const auto& r) {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;
        if constexpr (std::is_same_v<L, R>) {
            return l < r;
        }
        return false;
    }, lhs.value, rhs.value);
}

// Get value
std::optional<std::variant<std::monostate, double, std::string, std::vector<Expr>, std::map<Expr, Expr>, std::function<Expr(const std::vector<Expr>&)>>> getValue(const Expr& expr) {
    return expr.value;
}

bool isTruthy(const Expr& expr) {
    return std::visit(
        [](const auto& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return false; // None is false
            } else if constexpr (std::is_same_v<T, bool>) {
                return val;   // Boolean value itself
            } else if constexpr (std::is_same_v<T, double>) {
                return val != 0.0;
            } else if constexpr (std::is_same_v<T, std::string>) {
                return true;
            } else if constexpr (std::is_same_v<T, std::shared_ptr<ExprList>>) {
                return true;
            } else if constexpr (std::is_same_v<T, std::shared_ptr<ExprDict>>) {
                return true;
            } else {
                return true; // All other types are true
            }
        },
        expr.value);
}