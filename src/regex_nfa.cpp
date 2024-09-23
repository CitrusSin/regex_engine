#include <stack>
#include "regex_nfa.hpp"

using namespace regexs;

nonfinite_automaton::state nonfinite_automaton::state::next_state(char next) const {
    return atm->next_state(*this, next);
}

nonfinite_automaton::state& nonfinite_automaton::state::next(char next) {
    return *this = atm->next_state(*this, next);
}

nonfinite_automaton::state& nonfinite_automaton::state::operator+=(const state& s2) {
    insert(s2.begin(), s2.end());
    return *this;
}

std::set<char> nonfinite_automaton::state::character_transitions() const {
    return atm->character_transitions(*this);
}

nonfinite_automaton::single_state nonfinite_automaton::add_state() {
    nodes.push_back({.next={}, .eps_next={}});
    return nodes.size() - 1;
}

void nonfinite_automaton::add_jump(single_state from, char ch, single_state to) {
    nodes[from].next.insert(std::make_pair(ch, to));
}

void nonfinite_automaton::add_epsilon_jump(single_state from, single_state to) {
    nodes[from].eps_next.insert(to);
}

bool nonfinite_automaton::contains_epsilon_jump(single_state from, single_state to) const {
    return nodes[from].eps_next.count(to) > 0;
}

nonfinite_automaton::state nonfinite_automaton::epsilon_closure(single_state s) const {
    return epsilon_closure(state_of({s}));
}

nonfinite_automaton::state nonfinite_automaton::epsilon_closure(state states) const {
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

nonfinite_automaton::state nonfinite_automaton::next_state(single_state prev, char ch) const {
    auto iterator_begin = nodes[prev].next.find(ch);
    auto iterator_end = iterator_begin;

    state st = state_of({});
    while (iterator_end != nodes[prev].next.end() && iterator_end->first == ch) {
        st.insert(iterator_end->second);
        iterator_end++;
    }

    return epsilon_closure(st);
}

nonfinite_automaton::state nonfinite_automaton::next_state(const state& prev, char ch) const {
    state s = state_of({});
    for (single_state ss : prev) {
        auto it = nodes[ss].next.find(ch);
        while (it != nodes[ss].next.end() && it->first == ch) {
            s.insert((it++)->second);
        }
    }
    return epsilon_closure(s);
}

std::set<char> nonfinite_automaton::character_transitions(single_state sstate) const {
    std::set<char> transitions;

    for (auto& [ch, next] : nodes[sstate].next) {
        transitions.insert(ch);
    }

    return transitions;
}

std::set<char> nonfinite_automaton::character_transitions(const state& state) const {
    std::set<char> transitions;

    for (single_state sstate : state) {
        for (auto& [ch, next] : nodes[sstate].next) {
            transitions.insert(ch);
        }
    }

    return transitions;
}

nonfinite_automaton::state nonfinite_automaton::start_state() const {
    return epsilon_closure(start_sstate);
}

nonfinite_automaton::single_state nonfinite_automaton::start_single_state() const {
    return start_sstate;
}

void nonfinite_automaton::set_stop_state(single_state s, bool stop) {
    if (stop) {
        stop_sstates.insert(s);
    } else {
        stop_sstates.erase(s);
    }
}

bool nonfinite_automaton::is_stop_state(single_state s) const {
    return stop_sstates.count(s);
}

bool nonfinite_automaton::is_stop_state(const state& s) const {
    for (auto ss : s) {
        if (is_stop_state(ss)) return true;
    }
    return false;
}

void nonfinite_automaton::add_automaton(single_state from, const nonfinite_automaton& atm) {
    auto [start, stop] = import_automaton(atm);
    
    add_epsilon_jump(from, start);
    stop_sstates.insert(stop.begin(), stop.end());
}

void nonfinite_automaton::refactor_to_repetitive() {
    unify_stop_sstates();

    if (stop_sstates.size() == 0) {
        return;
    }

    if (contains_epsilon_jump(*stop_sstates.begin(), start_sstate)) {
        return;
    }

    add_epsilon_jump(*stop_sstates.begin(), start_sstate);
}

void nonfinite_automaton::refactor_to_skippable() {
    unify_stop_sstates();

    if (stop_sstates.size() == 0) {
        return;
    }

    if (contains_epsilon_jump(start_sstate, *stop_sstates.begin())) {
        return;
    }

    add_epsilon_jump(start_sstate, *stop_sstates.begin());
}

void nonfinite_automaton::connect(const nonfinite_automaton& atm) {
    unify_stop_sstates();

    single_state sstate = *stop_sstates.begin();
    stop_sstates.clear();

    add_automaton(sstate, atm);
}

nonfinite_automaton& nonfinite_automaton::operator|=(const nonfinite_automaton& m2) {
    add_automaton(start_sstate, m2);
    return *this;
}


std::pair<nonfinite_automaton::single_state, std::set<nonfinite_automaton::single_state>>
nonfinite_automaton::import_automaton(const nonfinite_automaton& atm) {
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

nonfinite_automaton::state nonfinite_automaton::state_of(std::initializer_list<single_state> sstates) const {
    return state(this, sstates);
}

void nonfinite_automaton::unify_stop_sstates() {
    if (stop_sstates.size() <= 1) return;

    single_state new_stop = add_state();
    for (single_state sstate : stop_sstates) {
        add_epsilon_jump(sstate, new_stop);
    }

    stop_sstates = {new_stop};
}

nonfinite_automaton nonfinite_automaton::string_automaton(std::string_view s) {
    nonfinite_automaton atm;

    auto state = atm.start_single_state();
    for (char c : s) {
        auto next_state = atm.add_state();
        atm.add_jump(state, c, next_state);
        state = next_state;
    }
    atm.set_stop_state(state);

    return atm;
}
