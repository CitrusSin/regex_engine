#include <sstream>
#include <stack>
#include "regex_nfa.hpp"

using namespace regexs;

nondeterministic_automaton::nondeterministic_automaton() : nodes{{.next={}, .eps_next={}}}, start_sstate(0) {}

nondeterministic_automaton::state nondeterministic_automaton::state::next_state(char next) const {
    return atm->next_state(*this, next);
}

nondeterministic_automaton::state& nondeterministic_automaton::state::next(char next) {
    return *this = atm->next_state(*this, next);
}

nondeterministic_automaton::state& nondeterministic_automaton::state::operator+=(const state& s2) {
    insert(s2.begin(), s2.end());
    return *this;
}

std::set<char> nondeterministic_automaton::state::character_transitions() const {
    return atm->character_transitions(*this);
}

nondeterministic_automaton::single_state nondeterministic_automaton::add_state() {
    nodes.push_back({.next={}, .eps_next={}});
    return nodes.size() - 1;
}

void nondeterministic_automaton::add_jump(single_state from, char ch, single_state to) {
    nodes[from].next.insert(std::make_pair(ch, to));
}

void nondeterministic_automaton::add_epsilon_jump(single_state from, single_state to) {
    nodes[from].eps_next.insert(to);
}

bool nondeterministic_automaton::contains_epsilon_jump(single_state from, single_state to) const {
    return nodes[from].eps_next.count(to) > 0;
}

nondeterministic_automaton::state nondeterministic_automaton::epsilon_closure(single_state s) const {
    return epsilon_closure(state_of({s}));
}

nondeterministic_automaton::state nondeterministic_automaton::epsilon_closure(state states) const {
    std::stack<single_state> search_stack;
    for (single_state s : states) {
        search_stack.push(s);
    }

    while (!search_stack.empty()) {
        single_state st = search_stack.top();
        search_stack.pop();

        for (single_state next : nodes[st].eps_next) {
            if (!states.count(next)) {
                states.insert(next);
                search_stack.push(next);
            }
        }
    }

    return states;
}

nondeterministic_automaton::state nondeterministic_automaton::next_state(single_state prev, char ch) const {
    auto iterator_begin = nodes[prev].next.find(ch);
    auto iterator_end = iterator_begin;

    state st = state_of({});
    while (iterator_end != nodes[prev].next.end() && iterator_end->first == ch) {
        st.insert(iterator_end->second);
        iterator_end++;
    }

    return epsilon_closure(st);
}

nondeterministic_automaton::state nondeterministic_automaton::next_state(const state& prev, char ch) const {
    state s = state_of({});
    for (single_state ss : prev) {
        auto it = nodes[ss].next.find(ch);
        while (it != nodes[ss].next.end() && it->first == ch) {
            s.insert((it++)->second);
        }
    }
    return epsilon_closure(s);
}

std::set<char> nondeterministic_automaton::character_transitions(single_state sstate) const {
    std::set<char> transitions;

    for (auto& [ch, next] : nodes[sstate].next) {
        transitions.insert(ch);
    }

    return transitions;
}

std::set<char> nondeterministic_automaton::character_transitions(const state& state) const {
    std::set<char> transitions;

    for (single_state sstate : state) {
        for (auto& [ch, next] : nodes[sstate].next) {
            transitions.insert(ch);
        }
    }

    return transitions;
}

nondeterministic_automaton::state nondeterministic_automaton::start_state() const {
    return epsilon_closure(start_sstate);
}

nondeterministic_automaton::single_state nondeterministic_automaton::start_single_state() const {
    return start_sstate;
}

void nondeterministic_automaton::set_stop_state(single_state s, bool stop) {
    if (stop) {
        stop_sstates.insert(s);
    } else {
        stop_sstates.erase(s);
    }
}

bool nondeterministic_automaton::is_stop_state(single_state s) const {
    return stop_sstates.count(s);
}

bool nondeterministic_automaton::is_stop_state(const state& s) const {
    for (auto ss : s) {
        if (is_stop_state(ss)) return true;
    }
    return false;
}

void nondeterministic_automaton::add_automaton(single_state from, const nondeterministic_automaton& atm) {
    auto [start, stop] = import_automaton(atm);
    
    add_epsilon_jump(from, start);
    stop_sstates.insert(stop.begin(), stop.end());
}

void nondeterministic_automaton::refactor_to_repetitive() {
    unify_stop_sstates();

    if (stop_sstates.size() == 0) {
        return;
    }

    if (contains_epsilon_jump(*stop_sstates.begin(), start_sstate)) {
        return;
    }

    add_epsilon_jump(*stop_sstates.begin(), start_sstate);
}

void nondeterministic_automaton::refactor_to_skippable() {
    unify_stop_sstates();

    if (stop_sstates.size() == 0) {
        return;
    }

    if (contains_epsilon_jump(start_sstate, *stop_sstates.begin())) {
        return;
    }

    add_epsilon_jump(start_sstate, *stop_sstates.begin());
}

void nondeterministic_automaton::connect(const nondeterministic_automaton& atm) {
    unify_stop_sstates();

    single_state sstate = *stop_sstates.begin();
    stop_sstates.clear();

    add_automaton(sstate, atm);
}

void nondeterministic_automaton::make_origin_branch(const nondeterministic_automaton& m2) {
    add_automaton(start_sstate, m2);
}

template <typename T>
static std::string serialize_set(const std::set<T>& val) {
    if (val.size() == 0) {
        return "{}";
    }

    std::stringstream seri_stream;
    if (val.size() == 1) {
        seri_stream << *val.begin();
        return seri_stream.str();
    }

    seri_stream << '{';

    bool mark = false;
    for (auto v : val) {
        if (mark) seri_stream << ',';
        seri_stream << v;
        mark = true;
    }

    seri_stream << '}';

    return seri_stream.str();
}

std::string nondeterministic_automaton::serialize() const {
    std::stringstream seri_stream;
    for (single_state ss = 0; ss < state_count(); ss++) {
        seri_stream << "STATE" << ss << ": {";

        bool mark1 = false;
        if (!nodes[ss].eps_next.empty()) {
            seri_stream << "EPS -> " << serialize_set(nodes[ss].eps_next);
            mark1 = true;
        }

        auto& nextmap = nodes[ss].next;

        char last_ch = '\0';
        std::set<single_state> last_set;
        for (auto it = nextmap.begin(); it != nextmap.end(); it++) {
            if (last_ch == '\0') last_ch = it->first;
            if (last_ch != it->first) {
                if (mark1) seri_stream << ',';
                mark1 = true;
                seri_stream << last_ch << " -> " << serialize_set(last_set);
                last_set.clear();
                last_ch = it->first;
            }
            last_set.insert(it->second);
        }
        if (!last_set.empty()) {
            if (mark1) seri_stream << ',';
            mark1 = true;
            seri_stream << last_ch << " -> " << serialize_set(last_set);
            last_set.clear();
        }

        seri_stream << "}\n";
    }

    seri_stream << "FINISH_STATES = " << serialize_set(stop_sstates) << "\n";
    return seri_stream.str();
}

// PRIVATE FUNCTIONS
std::pair<nondeterministic_automaton::single_state, std::set<nondeterministic_automaton::single_state>>
nondeterministic_automaton::import_automaton(const nondeterministic_automaton& atm) {
    single_state bias = nodes.size();
    for (single_state src = 0; src < atm.nodes.size(); src++) {
        state_node next_node;
        for (auto [ch, st] : atm.nodes[src].next) {
            next_node.next.emplace(ch, st + bias);
        }
        for (auto st : atm.nodes[src].eps_next) {
            next_node.eps_next.emplace(st + bias);
        }

        nodes.push_back(std::move(next_node));
    }

    single_state start_sstate = atm.start_sstate + bias;
    std::set<single_state> stop_sstates;
    for (auto s : atm.stop_sstates) {
        stop_sstates.insert(s + bias);
    }

    return make_pair(start_sstate, std::move(stop_sstates));
}

nondeterministic_automaton::state nondeterministic_automaton::state_of(std::initializer_list<single_state> sstates) const {
    return state(this, sstates);
}

void nondeterministic_automaton::unify_stop_sstates() {
    if (stop_sstates.size() <= 1) return;

    single_state new_stop = add_state();
    for (single_state sstate : stop_sstates) {
        add_epsilon_jump(sstate, new_stop);
    }

    stop_sstates = {new_stop};
}
