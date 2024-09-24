
#include "regex_dfa.hpp"
#include <map>
#include <sstream>
#include <utility>
#include <vector>

using namespace regexs;
using state = deterministic_automaton::state;

deterministic_automaton::deterministic_automaton() : state_map(1), state_marks(1), __start_state(0), __end_states() {}

state deterministic_automaton::add_state() {
    state_map.push_back({});
    state_marks.push_back({});
    return state_map.size() - 1;
}

state deterministic_automaton::start_state() const {
    return __start_state;
}

void deterministic_automaton::set_jump(state from, char ch, state to) {
    state_map[from][ch] = to;
}

state deterministic_automaton::next_state(state from, char ch) const {
    if (from == REJECT) return REJECT;
    if (state_map[from].count(ch)) return state_map[from].at(ch);
    return REJECT;
}

void deterministic_automaton::set_stop_state(state s, bool stop) {
    if (stop) {
        __end_states.insert(s);
    } else {
        __end_states.erase(s);
    }
}

bool deterministic_automaton::is_stop_state(state s) const {
    return __end_states.count(s) > 0;
}

void deterministic_automaton::add_state_mark(state s, int mark) {
    state_marks[s].insert(mark);
}

void deterministic_automaton::remove_state_mark(state s, int mark) {
    state_marks[s].erase(mark);
}

const std::set<int>& deterministic_automaton::state_mark(state s) const {
    return state_marks[s];
}

std::pair<state, std::set<state>> deterministic_automaton::import_automaton(const deterministic_automaton& atm) {
    size_t bias = state_count();
    state_map.insert(state_map.end(), atm.state_map.begin(), atm.state_map.end());
    state_marks.insert(state_marks.end(), atm.state_marks.begin(), atm.state_marks.end());

    for (size_t i = bias; i < state_map.size(); i++) {
        for (auto& [ch, target] : state_map[i]) {
            target += bias;
        }
    }

    state start_state = atm.__start_state + bias;
    std::set<state> stop_states;
    for (state s : atm.__end_states) {
        stop_states.insert(s + bias);
    }
    return std::make_pair(start_state, stop_states);
}



void deterministic_automaton::simplify() {
    class dsu {
    public:
        explicit dsu(size_t n) : values(n, 0) {}

        void reset(size_t x) {values[x] = x;}
        size_t root(size_t x) {
            if (values[x] == x) return x;
            return values[x] = root(values[x]);
        }
        void link(size_t a, size_t b) {values[root(a)] = root(b);}
    private:
        std::vector<size_t> values;
    };

    dsu equivalence(state_count());
    std::map<std::set<int>, state> mark_states;

    for (state s = 0; s < state_count(); s++) {
        if (is_stop_state(s)) {
            equivalence.reset(s);
            std::set<int> mark = state_mark(s);
            if (mark_states.count(mark)) {
                equivalence.link(s, mark_states[mark]);
            } else {
                mark_states[state_mark(s)] = s;
            }
        }
    }

    auto skiptable_equals = [&](const std::map<char, state>& table1, const std::map<char, state>& table2) {
        if (table1.size() != table2.size()) return false;

        for (auto [k, v] : table1) {
            if (!table2.count(k)) return false;
            if (equivalence.root(v) != equivalence.root(table2.at(k))) {
                return false;
            }
        }
        
        return true;
    };

    bool has_changes;
    do {
        has_changes = false;
        for (state s0 = 0; s0 < state_count(); s0++) {
            state s1 = equivalence.root(s0);
            if (s0 != s1 && !skiptable_equals(state_map[s0], state_map[s1])) {
                equivalence.reset(s0);
                has_changes = true;
            }
        }
    } while (has_changes);

    std::vector<state> state_mappings(state_count(), REJECT);
    for (state s = 0, sc = 0; s < state_count(); s++) {
        if (equivalence.root(s) == s) {
            state_mappings[s] = sc++;
        }
    }

    std::vector<state> remove_states;
    for (state s = 0; s < state_count(); s++) {
        if (equivalence.root(s) != s) {
            remove_states.push_back(s);
            continue;
        }

        auto& smap = state_map[s];
        for (auto& [ch, st] : smap) {
            smap[ch] = state_mappings[equivalence.root(st)];
        }
    }

    size_t bias = 0;
    for (state s : remove_states) {
        state_map.erase(state_map.begin() + s - bias);
        state_marks.erase(state_marks.begin() + s - bias);
        bias++;
    }

    std::set<state> new_stop_states;
    for (state s : __end_states) {
        new_stop_states.insert(state_mappings[equivalence.root(s)]);
    }
    __end_states = new_stop_states;
    __start_state = state_mappings[equivalence.root(__start_state)];
}

std::string deterministic_automaton::serialize() const {
    std::stringstream seri_stream;
    for (state s = 0; s < state_count(); s++) {
        seri_stream << "STATE" << s << ": {";
        bool mark = false;
        for (auto [ch, st] : state_map[s]) {
            if (mark) seri_stream << ", ";
            seri_stream << ch << " -> " << st;
            mark = true;
        }
        seri_stream << "}\n";
    }
    seri_stream << "STOP_STATES =";
    for (state s : __end_states) {
        seri_stream << ' ' << s;
    }
    seri_stream << '\n';

    return seri_stream.str();
}

