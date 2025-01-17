#ifndef REGEX_HPP
#define REGEX_HPP

#include <memory>
#include <string_view>
#include <vector>

#include "regex_dfa.hpp"
#include "regex_nfa.hpp"
#include "regex_parse.hpp"

namespace regexs {
    class regex {
    public:
        regex(std::string_view sv);

        bool match(std::string_view sv) const;
        std::vector<std::string> tokens() const;
        nondeterministic_automaton& automaton();
        const nondeterministic_automaton& automaton() const;
        const deterministic_automaton& deter_automaton() const;
    private:
        std::vector<std::shared_ptr<token>> __tokens;
        nondeterministic_automaton __atm;
        mutable std::unique_ptr<deterministic_automaton> __dfa_ptr;

        void make_dfa() const;
    };

    namespace literal {
        regex operator"" _regex(const char* str, size_t len);
    }

    using namespace literal;
}

#endif