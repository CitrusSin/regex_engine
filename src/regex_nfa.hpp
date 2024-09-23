#ifndef REGEX_NFA_HPP
#define REGEX_NFA_HPP

#include <initializer_list>
#include <vector>
#include <string>
#include <string_view>
#include <map>
#include <set>


namespace regexs {
    class nondeterministic_automaton {
    public:
        using single_state = size_t;
        
        class state : private std::set<single_state> {
        public:
            state next_state(char next) const;
            state& next(char next);
            state& operator+=(const state& s2);

            // Expose compare functions
#define __EXT_CMP(cmp) \
            inline bool operator cmp (const state& s2) const {    \
                if (atm == s2.atm) { \
                    return  static_cast<const std::set<single_state>&>(*this)   \
                            cmp static_cast<const std::set<single_state>&>(s2); \
                } else { \
                    return atm cmp s2.atm;\
                } \
            }

            __EXT_CMP(<)  __EXT_CMP(==)  __EXT_CMP(>)
            __EXT_CMP(<=) __EXT_CMP(!=)  __EXT_CMP(>=)
#undef __EXT_CMP

            std::set<char> character_transitions() const;

        private:
            friend class nondeterministic_automaton;

            const nondeterministic_automaton* atm;

            state(const nondeterministic_automaton* atm) : std::set<single_state>(), atm(atm) {}
            state(const nondeterministic_automaton* atm, std::initializer_list<single_state> il) : std::set<single_state>(std::move(il)), atm(atm) {}
        };

        nondeterministic_automaton();

        inline size_t state_count() const { return nodes.size(); }

        single_state add_state();
        void add_jump(single_state from, char ch, single_state to);
        void add_epsilon_jump(single_state from, single_state to);
        bool contains_epsilon_jump(single_state from, single_state to) const;
        state epsilon_closure(single_state s) const;
        state epsilon_closure(state states) const;
        state next_state(single_state prev, char ch) const;
        state next_state(const state& prev, char ch) const;
        std::set<char> character_transitions(single_state sstate) const;
        std::set<char> character_transitions(const state& state) const;
        state start_state() const;
        single_state start_single_state() const;
        void set_stop_state(single_state s, bool stop = true);
        bool is_stop_state(single_state s) const;
        bool is_stop_state(const state& s) const;
        void add_automaton(single_state from, const nondeterministic_automaton& atm);
        void refactor_to_repetitive();
        void refactor_to_skippable();
        void connect(const nondeterministic_automaton& atm);
        void make_origin_branch(const nondeterministic_automaton& m2);

        std::string serialize() const;

        static nondeterministic_automaton string_automaton(std::string_view s);
    private:
        struct state_node {
            std::multimap<char, single_state> next;
            std::set<single_state> eps_next;
        };

        std::vector<state_node> nodes;
        single_state start_sstate;
        std::set<single_state> stop_sstates;

        std::pair<single_state, std::set<single_state>> import_automaton(const nondeterministic_automaton& atm);
        state state_of(std::initializer_list<single_state> sstates) const;
        void unify_stop_sstates();
    };
}


#endif