#ifndef REGEX_DFA_HPP
#define REGEX_DFA_HPP

#include <string>
#include <limits>
#include <set>
#include <utility>
#include <vector>
#include <map>

namespace regexs {
    class deterministic_automaton {
    public:
        using state = size_t;
        const state REJECT = std::numeric_limits<size_t>::max();

        deterministic_automaton();

        inline size_t state_count() const { return state_map.size(); }

        state add_state();
        state start_state() const;
        void set_jump(state from, char ch, state to);
        state next_state(state from, char ch) const;
        void set_stop_state(state s, bool stop = true);
        bool is_stop_state(state s) const;

        void add_state_mark(state s, int mark);
        void remove_state_mark(state s, int mark);
        const std::set<int>& state_mark(state s) const;

        std::pair<state, std::set<state>> import_automaton(const deterministic_automaton& atm);

        void simplify();

        std::string serialize() const;
    private:
        std::vector<std::map<char, state>> state_map;
        std::vector<std::set<int>> state_marks;
        state __start_state;
        std::set<state> __end_states;
    };
}

#endif