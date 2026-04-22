/*
 * Runtime Mathematical Expression Parser
 * Implements a Recursive Descent Parser (LL(1)) for evaluating strings 
 * into mathematical functions at runtime. 
 *
 * Grammer:
 *   Expression -> Term { ('+'|'-') Term }
 *   Term       -> Factor { ('*'|'/') Factor }
 *   Factor     -> Power [ '^' Factor ]
 *   Power      -> [ '-' ] ( '(' Expression ')' | Function | Number | Variable )
 */

#pragma once
#include <string>
...
#include <vector>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <iostream>

namespace math {

class Parser {
public:
    using Func = std::function<float(float, float, float)>;

    static Func parse(const std::string& expression) {
        Parser p(expression);
        try {
            return p.parse_expression();
        } catch (const std::exception& e) {
            std::cerr << "Parser Error: " << e.what() << std::endl;
            return [](float, float, float) { return 0.0f; };
        }
    }

private:
    Parser(const std::string& expr) : m_expr(expr), m_pos(0) {}

    char peek() { return m_pos < m_expr.length() ? m_expr[m_pos] : 0; }
    char get() { return m_pos < m_expr.length() ? m_expr[m_pos++] : 0; }
    void skip_ws() { while (std::isspace(peek())) m_pos++; }

    Func parse_expression() {
        Func f = parse_term();
        skip_ws();
        while (peek() == '+' || peek() == '-') {
            char op = get();
            Func next = parse_term();
            if (op == '+') f = [f, next](float x, float y, float t) { return f(x, y, t) + next(x, y, t); };
            else f = [f, next](float x, float y, float t) { return f(x, y, t) - next(x, y, t); };
            skip_ws();
        }
        return f;
    }

    Func parse_term() {
        Func f = parse_factor();
        skip_ws();
        while (peek() == '*' || peek() == '/') {
            char op = get();
            Func next = parse_factor();
            if (op == '*') f = [f, next](float x, float y, float t) { return f(x, y, t) * next(x, y, t); };
            else f = [f, next](float x, float y, float t) { 
                return next(x, y, t) != 0 ? f(x, y, t) / next(x, y, t) : 0; 
            };
            skip_ws();
        }
        return f;
    }

    Func parse_factor() {
        Func f = parse_power();
        skip_ws();
        if (peek() == '^') {
            get();
            Func next = parse_factor();
            f = [f, next](float x, float y, float t) { return std::pow(f(x, y, t), next(x, y, t)); };
        }
        return f;
    }

    Func parse_power() {
        skip_ws();
        char c = peek();
        if (c == '-') {
            get();
            Func next = parse_power();
            return [next](float x, float y, float t) { return -next(x, y, t); };
        }
        if (c == '(') {
            get();
            Func f = parse_expression();
            if (get() != ')') throw std::runtime_error("Expected ')'");
            return f;
        }
        if (std::isdigit(c)) {
            std::string s;
            while (std::isdigit(peek()) || peek() == '.') s += get();
            float val = std::stof(s);
            return [val](float, float, float) { return val; };
        }
        if (std::isalpha(c)) {
            std::string s;
            while (std::isalpha(peek())) s += get();
            if (s == "x") return [](float x, float, float) { return x; };
            if (s == "y") return [](float, float y, float) { return y; };
            if (s == "t") return [](float, float, float t) { return t; };
            
            // Functions
            skip_ws();
            if (peek() == '(') {
                get();
                Func next = parse_expression();
                if (get() != ')') throw std::runtime_error("Expected ')' after function");
                if (s == "sin") return [next](float x, float y, float t) { return std::sin(next(x, y, t)); };
                if (s == "cos") return [next](float x, float y, float t) { return std::cos(next(x, y, t)); };
                if (s == "tan") return [next](float x, float y, float t) { return std::tan(next(x, y, t)); };
                if (s == "sqrt") return [next](float x, float y, float t) { return std::sqrt(std::abs(next(x, y, t))); };
                if (s == "abs") return [next](float x, float y, float t) { return std::abs(next(x, y, t)); };
            }
            throw std::runtime_error("Unknown identifier: " + s);
        }
        throw std::runtime_error("Unexpected character: " + std::string(1, c));
    }

    std::string m_expr;
    size_t m_pos;
};

} // namespace math
