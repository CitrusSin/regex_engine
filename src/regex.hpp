#ifndef REGEX_HPP
#define REGEX_HPP

#include <initializer_list>
#include <vector>
#include <string_view>
#include <map>
#include <set>
#include <cstddef>


namespace regexs {
    class automaton {
    public:
        using single_state = size_t;
        
        class state : private std::set<single_state> {
        private:
            friend class automaton;

            state() : std::set<single_state>() {}
            state(std::initializer_list<single_state> il) : std::set<single_state>(std::move(il)) {}
        };

        automaton() : nodes{{.next={}, .eps_next={}}}, start_sstate(0) {}

        single_state add_state();
        void add_jump(single_state from, char ch, single_state to);
        void add_epsilon_jump(single_state from, single_state to);
        bool contains_epsilon_jump(single_state from, single_state to) const;
        state epsilon_closure(single_state s) const;
        state epsilon_closure(state states) const;
        state next_state(single_state prev, char ch) const;
        state next_state(const state& prev, char ch) const;
        state start_state() const;
        single_state start_single_state() const;
        void set_stop_state(single_state s, bool stop = true);
        bool is_stop_state(single_state s) const;
        bool is_stop_state(const state& s) const;
        void add_automaton(single_state from, const automaton& atm);
        void refactor_to_repetitive();
        void refactor_to_skippable();

        void connect(const automaton& atm);

        automaton& operator|=(const automaton& m2);

        static automaton string_automaton(std::string_view s);
    private:
        struct state_node {
            std::multimap<char, single_state> next;
            std::set<single_state> eps_next;
        };

        std::vector<state_node> nodes;
        single_state start_sstate;
        std::set<single_state> stop_sstates;

        std::pair<single_state, std::set<single_state>> import_automaton(const automaton& atm);
        void unify_stop_sstates();
    };

    class regex {
    public:
        regex(std::string_view sv);

        bool match(std::string_view sv) const;
    private:
        automaton atm;
    };
}


#endif