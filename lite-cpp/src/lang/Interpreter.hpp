#pragma once

#include "Expr.hpp"
#include "Environment.hpp"

class Interpreter {
public:
    Interpreter(std::shared_ptr<Environment> env) :
        m_env(env) {}

    Expr evaluate(const Expr& expr);

private:
    std::shared_ptr<Environment> m_env;
};