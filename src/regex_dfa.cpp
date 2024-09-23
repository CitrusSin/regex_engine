
#include "regex_dfa.hpp"
#include "regex_nfa.hpp"
#include <deque>
#include <sstream>

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

    return atm;
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