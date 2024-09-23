#ifndef REGEX_TOKEN_HPP
#define REGEX_TOKEN_HPP

#include <string>
#include <deque>
#include <memory>

#include "regex_nfa.hpp"

namespace regexs {
    class token {
    public:
        virtual ~token() {}

        enum type {
            STRING, OPERATOR, LEFT_BRACKET, RIGHT_BRACKET
        };
        virtual type get_type() const = 0;
        virtual std::string serialize() const = 0;
    };

    class plain_string : public token {
    public:
        explicit plain_string(std::string_view content) : __content(content) {}

        constexpr const std::string& content() const {
            return __content;
        }

        type get_type() const override {
            return STRING;
        }

        std::string serialize() const override {
            return "PLAIN_STRING\"" + __content + "\"";
        }
    private:
        std::string __content;
    };

    class oper : public token {
    public:
        virtual ~oper() {}

        virtual int priority() const = 0;
        virtual int operand_count() const = 0;
        virtual char content() const = 0;
        virtual void apply_operator(std::deque<nondeterministic_automaton>& operands) = 0;

        virtual std::string serialize() const override {
            return std::string("OPERATOR\'") + content() + "\'";
        }

        virtual type get_type() const override {
            return OPERATOR;
        }

        static constexpr inline bool is_operator(char c);
        static std::unique_ptr<oper> operator_from(char op);
    };

    class oper_bracket : public oper {
    public:
        explicit oper_bracket(char ch = '(') : __content(ch) {}

        int priority() const override       { return -1; }
        int operand_count() const override  { return (__content == ')') ? 1 : 2; }
        char content() const override       { return __content; }

        type get_type() const override { return (__content == '(') ? LEFT_BRACKET : RIGHT_BRACKET; }

        std::string serialize() const override { return (__content == '(') ? "LEFT_BRACKET" : "RIGHT_BRACKET"; }

        void apply_operator(std::deque<nondeterministic_automaton>& operands) override {}
    private:
        char __content;
    };

    class oper_plus : public oper {
    public:
        int priority() const override       { return 2; }
        int operand_count() const override  { return 1; }
        char content() const override       { return '+'; }

        void apply_operator(std::deque<nondeterministic_automaton>& operands) override {
            operands.back().refactor_to_repetitive();
        }
    };

    class oper_optional : public oper {
    public:
        int priority() const override       { return 2; }
        int operand_count() const override  { return 1; }
        char content() const override       { return '?'; }

        void apply_operator(std::deque<nondeterministic_automaton>& operands) override {
            operands.back().refactor_to_skippable();
        }
    };

    class oper_asterisk : public oper {
    public:
        int priority() const override       { return 2; }
        int operand_count() const override  { return 1; }
        char content() const override       { return '*'; }

        void apply_operator(std::deque<nondeterministic_automaton>& operands) override {
            operands.back().refactor_to_repetitive();
            operands.back().refactor_to_skippable();
        }
    };

    class oper_concat : public oper {
    public:
        int priority() const override       { return 1; }
        int operand_count() const override  { return 2; }
        char content() const override       { return 'C'; }

        std::string serialize() const override { return "CONNECT"; }

        void apply_operator(std::deque<nondeterministic_automaton>& operands) override {
            nondeterministic_automaton& r2 = operands[operands.size()-1];
            nondeterministic_automaton& r1 = operands[operands.size()-2];
            r1.connect(r2);
            operands.pop_back();
        }
    };

    class oper_or : public oper {
    public:
        int priority() const override       { return 0; }
        int operand_count() const override  { return 2; }
        char content() const override       { return '|'; }

        void apply_operator(std::deque<nondeterministic_automaton>& operands) override {
            nondeterministic_automaton& r2 = operands[operands.size()-1];
            nondeterministic_automaton& r1 = operands[operands.size()-2];
            r1.make_origin_branch(r2);
            operands.pop_back();
        }
    };

    constexpr inline bool oper::is_operator(char c) {
        switch (c) {
        case '(':
        case ')':
        case '+':
        case '?':
        case '*':
        case '|':
        case '\0':
            return true;
        default:
            return false;
        }
    }

    inline std::unique_ptr<oper> oper::operator_from(char op) {
        switch (op) {
        case '(':
            return std::make_unique<oper_bracket>('(');
        case ')':
            return std::make_unique<oper_bracket>(')');
        case '+':
            return std::make_unique<oper_plus>();
        case '?':
            return std::make_unique<oper_optional>();
        case '*':
            return std::make_unique<oper_asterisk>();
        case '|':
            return std::make_unique<oper_or>();
        case '\0':
            return std::make_unique<oper_concat>();
        default:
            return std::make_unique<oper_concat>();
        }
    }

    std::vector<std::shared_ptr<token>> regex_tokenize(std::string_view sv);
    nondeterministic_automaton build_nfa(const std::vector<std::shared_ptr<token>>& tokens);
}

#endif