
#include "regex.hpp"
#include "regex_nfa.hpp"
#include "regex_parse.hpp"

namespace regexs {
    regex::regex(std::string_view sv) : atm(build_nfa(regex_tokenize(sv))) {}

    bool regex::match(std::string_view sv) const {
        nonfinite_automaton::state s = atm.start_state();
        for (char c : sv) {
            s = atm.next_state(s, c);
        }
        
        return atm.is_stop_state(s);
    }
}