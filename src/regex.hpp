#ifndef REGEX_HPP
#define REGEX_HPP

#include <string_view>

#include "regex_nfa.hpp"

namespace regexs {
    class regex {
    public:
        regex(std::string_view sv);

        bool match(std::string_view sv) const;
    private:
        nonfinite_automaton atm;
    };
}

#endif