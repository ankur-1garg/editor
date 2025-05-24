#include "Builtin.hpp"
#include "Expr.hpp"
#include "Environment.hpp"
#include "../Editor.hpp" // Include Editor definition for context
#include "Interpreter.hpp" // Needed for evalArg helper

#include <iostream>   // For builtin_print example
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <numeric>    // For std::accumulate (example in add)

// --- Helper to Add Builtins ---

void addBuiltinFunction(
    Environment& env,
    const std::string& name,
    BuiltinFuncType func,
    const std::string& helpShort,
    const std::string& helpLong
) {
    auto info = std::make_shared<BuiltinFunctionInfo>(
        BuiltinFunctionInfo{name, std::move(func), helpShort, helpLong}
    );
    // Define the symbol 'name' in the environment to point to this builtin info
    env.define(name, info);
}

// --- Builtin Argument Helpers ---

// Forward declare the actual evaluator function (defined in Interpreter.cpp)
// We need this circular dependency resolved for evalArg
// Expr evaluateImpl(const Expr& expr, Editor& editor, Environment& env); // Or however Interpreter exposes it

Expr evalArg(const Expr& arg, Editor& editor, Environment& env) {
    // Builtins often receive unevaluated arguments if they are macros,
    // but typically receive evaluated arguments for normal functions.
    // This helper assumes the interpreter needs to evaluate args passed to builtins.
    // Note: The Interpreter's design determines if args are pre-evaluated or not.
    // If args are pre-evaluated, this function might just return 'arg'.
    // If not pre-evaluated (like Lisp), evaluation happens here.

    // Assuming Interpreter needs to be instantiated and called:
     Interpreter interpreter(std::make_shared<Environment>(env)); // Use current env as parent
     return interpreter.evaluate(arg); // Evaluate the argument expression

    // --- OR --- if interpreter evaluates args before calling builtin:
    // return arg;
}

void checkArgCount(const std::string& funcName, const std::vector<Expr>& args, size_t expected) {
    if (args.size() != expected) {
        throw std::runtime_error("Builtin '" + funcName + "' expected " +
                                 std::to_string(expected) + " arguments, but got " +
                                 std::to_string(args.size()));
    }
}

template <typename T>
const T& expectArgType(const std::string& funcName, const Expr& arg, size_t argIndex) {
    if (const T* val = std::get_if<T>(&arg)) {
        return *val;
    } else {
        throw std::runtime_error("Builtin '" + funcName + "' expected type " +
                                 typeid(T).name() + " for argument " + std::to_string(argIndex + 1) +
                                 ", but got " + toString(arg));
    }
}

template <typename T>
const std::shared_ptr<T>& expectArgSharedPtrType(const std::string& funcName, const Expr& arg, size_t argIndex) {
     using PtrT = std::shared_ptr<T>;
     if (const PtrT* ptr_val = std::get_if<PtrT>(&arg)) {
        if (*ptr_val) { // Ensure the pointer itself is not null
             return *ptr_val;
        } else {
             throw std::runtime_error("Builtin '" + funcName + "' received null pointer for argument " +
                                      std::to_string(argIndex + 1));
        }
     } else {
          throw std::runtime_error("Builtin '" + funcName + "' expected pointer type " +
                                 typeid(T).name() + "* for argument " + std::to_string(argIndex + 1) +
                                 ", but got " + toString(arg));
     }
}


// --- Builtin Function Implementations ---

Expr builtin_print(const std::vector<Expr>& args, Editor& editor, Environment& env) {
    // Example: print arguments to the status bar
    std::string output = "";
    for (size_t i = 0; i < args.size(); ++i) {
        Expr evaluatedArg = evalArg(args[i], editor, env); // Evaluate each arg
        output += toString(evaluatedArg);
        if (i < args.size() - 1) {
            output += " ";
        }
    }
    if (editor.getFrontend()) {
        editor.getFrontend()->setStatus(output);
    } else {
        std::cout << output << std::endl; // Fallback to console if no frontend
    }
    return make_nil(); // Print typically returns None/Nil
}

Expr builtin_add(const std::vector<Expr>& args, Editor& editor, Environment& env) {
    // Example: Add numbers or concatenate strings/lists
    if (args.empty()) {
        return make_int(0); // Adding nothing is 0? Or error?
    }

    // Evaluate the first argument to determine the type
    Expr result = evalArg(args[0], editor, env);

    for (size_t i = 1; i < args.size(); ++i) {
        Expr currentArg = evalArg(args[i], editor, env);

        // Use std::visit to handle different type combinations
        result = std::visit(
            [&](auto& res_val, const auto& arg_val) -> Expr {
                using ResT = std::decay_t<decltype(res_val)>;
                using ArgT = std::decay_t<decltype(arg_val)>;

                // --- Number addition ---
                if constexpr (std::is_same_v<ResT, long long> && std::is_same_v<ArgT, long long>) {
                    return make_int(res_val + arg_val);
                } else if constexpr (std::is_same_v<ResT, double> && std::is_same_v<ArgT, double>) {
                    return make_float(res_val + arg_val);
                } else if constexpr (std::is_same_v<ResT, long long> && std::is_same_v<ArgT, double>) {
                    return make_float(static_cast<double>(res_val) + arg_val);
                } else if constexpr (std::is_same_v<ResT, double> && std::is_same_v<ArgT, long long>) {
                    return make_float(res_val + static_cast<double>(arg_val));
                }
                // --- String concatenation ---
                else if constexpr (std::is_same_v<ResT, std::string> && std::is_same_v<ArgT, std::string>) {
                    return make_string(res_val + arg_val);
                }
                // --- List concatenation ---
                else if constexpr (std::is_same_v<ResT, std::shared_ptr<ExprList>> && std::is_same_v<ArgT, std::shared_ptr<ExprList>>) {
                    if (!res_val || !arg_val) throw std::runtime_error("Cannot add null lists");
                    std::vector<Expr> combined = res_val->items;
                    combined.insert(combined.end(), arg_val->items.begin(), arg_val->items.end());
                    return make_list(std::move(combined));
                }
                // --- Error ---
                else {
                     throw std::runtime_error("Builtin 'add' cannot add types " +
                                             toString(result) + " and " + toString(currentArg));
                }
            },
            result, // Pass current result (potentially modified in-place by variant access)
            currentArg);
    }
    return result;
}


// --- Editor Interaction Builtins (Placeholders - Require Editor methods) ---

Expr builtin_insert(const std::vector<Expr>& args, Editor& editor, Environment& env) {
    // Inserts evaluated string representation of all arguments at cursor
    std::string textToInsert = "";
     for (const auto& arg : args) {
          Expr evaluatedArg = evalArg(arg, editor, env);
          textToInsert += toString(evaluatedArg); // Simple string conversion
     }
     if (!textToInsert.empty()) {
          editor.insertString(textToInsert); // Assumes Editor::insertString exists
     }
     return make_nil();
}

Expr builtin_delete(const std::vector<Expr>& args, Editor& editor, Environment& env) {
     checkArgCount("delete", args, 1);
     Expr evaluatedArg = evalArg(args[0], editor, env);
     long long count = expectArgType<long long>("delete", evaluatedArg, 0);

     if (count > 0) {
          // Assumes Editor::deleteChars exists (or similar)
          // editor.deleteChars(count, Direction::Backward); // Or specify direction
          for(long long i = 0; i < count; ++i) editor.deleteBackward(); // Simple
     } else if (count < 0) {
          for(long long i = 0; i > count; --i) editor.deleteForward(); // Simple
     }
     return make_nil();
}

Expr builtin_move(const std::vector<Expr>& args, Editor& editor, Environment& env) {
    checkArgCount("move", args, 1);
    Expr evaluatedArg = evalArg(args[0], editor, env);

    // Handle either integer offset or direction symbol/string
    if (auto* offset_ptr = std::get_if<long long>(&evaluatedArg)) {
        long long offset = *offset_ptr;
        Direction dir = (offset >= 0) ? Direction::Right : Direction::Left;
        long long count = std::abs(offset);
        for (long long i = 0; i < count; ++i) {
             editor.moveCursor(dir);
        }
    } else if (auto* str_ptr = std::get_if<std::string>(&evaluatedArg)) {
         const std::string& dir_str = *str_ptr;
         if (dir_str == "up") editor.moveCursor(Direction::Up);
         else if (dir_str == "down") editor.moveCursor(Direction::Down);
         else if (dir_str == "left") editor.moveCursor(Direction::Left);
         else if (dir_str == "right") editor.moveCursor(Direction::Right);
         else throw std::runtime_error("Builtin 'move' invalid direction string: " + dir_str);
    } else if (const auto* sym_ptr = std::get_if<std::shared_ptr<ExprSymbol>>(&evaluatedArg)) {
        const std::string& dir_str = (*sym_ptr)->name;
         if (dir_str == "up") editor.moveCursor(Direction::Up);
         else if (dir_str == "down") editor.moveCursor(Direction::Down);
         else if (dir_str == "left") editor.moveCursor(Direction::Left);
         else if (dir_str == "right") editor.moveCursor(Direction::Right);
         else throw std::runtime_error("Builtin 'move' invalid direction symbol: " + dir_str);
    }
    else {
        throw std::runtime_error("Builtin 'move' expects an integer offset or direction symbol/string.");
    }
    return make_nil();
}

Expr builtin_goto(const std::vector<Expr>& args, Editor& editor, Environment& env) {
     checkArgCount("goto", args, 2);
     Expr evalRow = evalArg(args[0], editor, env);
     Expr evalCol = evalArg(args[1], editor, env);
     long long row = expectArgType<long long>("goto", evalRow, 0);
     long long col = expectArgType<long long>("goto", evalCol, 1);

     editor.gotoPosition(row, col); // Assumes Editor::gotoPosition exists
     return make_nil();
}

Expr builtin_get_select(const std::vector<Expr>& args, Editor& editor, Environment& env) {
     if (!args.empty()) throw std::runtime_error("Builtin 'get-select' expects 0 arguments");
     // Assumes Editor::getSelectedText exists and returns std::optional<string>
     auto selectedOpt = editor.getSelectedText();
     return selectedOpt ? make_string(*selectedOpt) : make_nil();
}

Expr builtin_select(const std::vector<Expr>& args, Editor& editor, Environment& env) {
    if (!args.empty()) throw std::runtime_error("Builtin 'select' expects 0 arguments");
     editor.startSelection(); // Assumes Editor::startSelection exists
     return make_nil();
}

Expr builtin_unselect(const std::vector<Expr>& args, Editor& editor, Environment& env) {
    if (!args.empty()) throw std::runtime_error("Builtin 'unselect' expects 0 arguments");
     editor.clearSelection(); // Assumes Editor::clearSelection exists
     return make_nil();
}

Expr builtin_new_buf(const std::vector<Expr>& args, Editor& editor, Environment& env) {
     if (!args.empty()) throw std::runtime_error("Builtin 'new-buf' expects 0 arguments");
     int index = editor.createNewBuffer();
     return make_int(index);
}

Expr builtin_set_buf(const std::vector<Expr>& args, Editor& editor, Environment& env) {
     checkArgCount("set-buf", args, 1);
     Expr evalIndex = evalArg(args[0], editor, env);
     long long index = expectArgType<long long>("set-buf", evalIndex, 0);
     if (!editor.switchToBuffer(static_cast<int>(index))) {
          throw std::runtime_error("Builtin 'set-buf' invalid buffer index: " + std::to_string(index));
     }
     return make_int(index); // Return the index set
}

Expr builtin_get_cur_buf(const std::vector<Expr>& args, Editor& editor, Environment& env) {
    if (!args.empty()) throw std::runtime_error("Builtin 'get-cur-buf' expects 0 arguments");
    return make_int(editor.getCurrentBufferIndex());
}


// ... Implement other builtins ...