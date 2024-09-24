
#include "regex_dfa.hpp"
#include "regex_nfa.hpp"
#include <deque>
#include <sstream>
#include <vector>

using namespace regexs;

deterministic_automaton::deterministic_automaton() : state_map(1), __start_state(0), __end_states() {}

deterministic_automaton::state deterministic_automaton::add_state() {
    state_map.push_back({});
    return state_map.size() - 1;
}

deterministic_automaton::state deterministic_automaton::start_state() const {
    return __start_state;
}

void deterministic_automaton::set_jump(state from, char ch, state to) {
    state_map[from][ch] = to;
}

deterministic_automaton::state deterministic_automaton::next_state(state from, char ch) const {
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

    dsu equi(state_count());
    state first_end = REJECT;
    for (state s = 0; s < state_count(); s++) {
        if (is_stop_state(s)) {
            if (first_end == REJECT) {
                first_end = s;
                equi.reset(s);
            } else {
                equi.reset(s);
                equi.link(s, first_end);
            }
        }
    }

    auto skiptable_equals = [&](const std::map<char, state>& table1, const std::map<char, state>& table2) {
        if (table1.size() != table2.size()) return false;

        for (auto [k, v] : table1) {
            if (!table2.count(k)) return false;
            if (equi.root(v) != equi.root(table2.at(k))) {
                return false;
            }
        }
        
        return true;
    };

    bool has_changes = true;
    while (has_changes) {
        has_changes = false;
        for (state s0 = 0; s0 < state_count(); s0++) {
            state s1 = equi.root(s0);
            if (s0 != s1 && !skiptable_equals(state_map[s0], state_map[s1])) {
                equi.reset(s0);
                has_changes = true;
            }
        }
    }

    std::vector<state> state_mappings(state_count(), REJECT);
    for (state s = 0, sc = 0; s < state_count(); s++) {
        if (equi.root(s) == s) {
            state_mappings[s] = sc++;
        }
    }

    std::vector<state> remove_states;
    for (state s = 0; s < state_count(); s++) {
        if (equi.root(s) != s) {
            remove_states.push_back(s);
            continue;
        }

        auto& smap = state_map[s];
        for (auto& [ch, st] : smap) {
            smap[ch] = state_mappings[equi.root(st)];
        }
    }

    size_t bias = 0;
    for (state s : remove_states) {
        state_map.erase(state_map.begin() + s - (bias++));
    }

    std::set<state> new_stop_states;
    for (state s : __end_states) {
        new_stop_states.insert(state_mappings[equi.root(s)]);
    }
    __end_states = new_stop_states;
    __start_state = state_mappings[equi.root(__start_state)];
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

deterministic_automaton deterministic_automaton::from_nonfinite(const nondeterministic_automaton& nfa) {
    deterministic_automaton atm;

    nondeterministic_automaton::state nfa_state = nfa.start_state();

    std::map<nondeterministic_automaton::state, deterministic_automaton::state> state_translate;
    state_translate[nfa_state] = atm.start_state();

    std::deque<nondeterministic_automaton::state> state_queue;
    state_queue.push_back(nfa_state);

    while (!state_queue.empty()) {
        nondeterministic_automaton::state& st = state_queue.front();
        deterministic_automaton::state fst = state_translate[st];

        for (char ch : st.character_transitions()) {
            nondeterministic_automaton::state next_st = st.next_state(ch);
            deterministic_automaton::state next_fst;
            if (!state_translate.count(next_st)) {
                next_fst = state_translate[next_st] = atm.add_state();
                atm.set_stop_state(next_fst, nfa.is_stop_state(next_st));
                state_queue.push_back(next_st);
            } else {
                next_fst = state_translate[next_st];
            }
            atm.set_jump(fst, ch, next_fst);
        }

        state_queue.pop_front();
    }

    atm.simplify();

    return atm;
}
