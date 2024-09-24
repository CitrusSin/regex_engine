
#include "regex.hpp"
#include "regex_dfa.hpp"
#include "regex_nfa.hpp"
#include "regex_parse.hpp"
#include <memory>
#include <string>

namespace regexs {
    regex::regex(std::string_view sv) : 
        __tokens(regex_tokenize(sv)),
        __atm(build_nfa(__tokens)),
        __dfa_ptr(nullptr)
    {}

    bool regex::match(std::string_view sv) const {
        make_dfa();

        deterministic_automaton::state s = __dfa_ptr->start_state();
        for (char c : sv) {
            s = __dfa_ptr->next_state(s, c);
        }
        
        return __dfa_ptr->is_stop_state(s);
    }

    std::vector<std::string> regex::tokens() const {
        std::vector<std::string> tks(__tokens.size());
        for (size_t i=0; i<__tokens.size(); i++) {
            tks[i] = __tokens[i]->serialize();
        }
        return tks;
    }

    nondeterministic_automaton& regex::automaton() {
        return __atm;
    }

    const nondeterministic_automaton& regex::automaton() const {
        return __atm;
    }

    const deterministic_automaton& regex::deter_automaton() const {
        make_dfa();
        return *__dfa_ptr;
    }

    void regex::make_dfa() const {
        if (__dfa_ptr == nullptr) {
            __dfa_ptr = std::make_unique<deterministic_automaton>(__atm.to_deterministic());
        }
    }

    regex literal::operator"" _regex(const char* str, size_t len) {
        return regex(std::string_view(str, len));
    }
}