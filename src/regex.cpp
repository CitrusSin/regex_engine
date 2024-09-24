
#include "regex.hpp"
#include "regex_dfa.hpp"
#include "regex_nfa.hpp"
#include "regex_parse.hpp"
#include <string>

namespace regexs {
    regex::regex(std::string_view sv) : 
        __tokens(regex_tokenize(sv)),
        __atm(build_nfa(__tokens)),
        __dfa(__atm.to_deterministic())
    {}

    bool regex::match(std::string_view sv) const {
        deterministic_automaton::state s = __dfa.start_state();
        for (char c : sv) {
            s = __dfa.next_state(s, c);
        }
        
        return __dfa.is_stop_state(s);
    }

    std::vector<std::string> regex::tokens() const {
        std::vector<std::string> tks(__tokens.size());
        for (size_t i=0; i<__tokens.size(); i++) {
            tks[i] = __tokens[i]->serialize();
        }
        return tks;
    }

    const nondeterministic_automaton& regex::automaton() const {
        return __atm;
    }

    const deterministic_automaton& regex::deter_automaton() const {
        return __dfa;
    }
}