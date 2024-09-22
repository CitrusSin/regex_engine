#include <deque>
#include <string>
#include <memory>
#include <stack>
#include <cassert>

#include "regex.hpp"


template <typename T>
constexpr static inline std::set<T>& operator+=(std::set<T>& s1, const std::set<T>& s2) {
    s1.insert(s2.begin(), s2.end());
    return s1;
}


namespace regexs {
    class token {
    public:
        virtual ~token() {}

        enum type {
            STRING, OPERATOR, LEFT_BRACKET, RIGHT_BRACKET
        };
        virtual type get_type() const = 0;
        virtual std::string serialize() const = 0;
    };

    class plain_string : public token {
    public:
        explicit plain_string(std::string_view content) : __content(content) {}

        constexpr const std::string& content() const {
            return __content;
        }

        type get_type() const override {
            return STRING;
        }

        std::string serialize() const override {
            return "plain_string\"" + __content + "\"";
        }
    private:
        std::string __content;
    };

    class oper : public token {
    public:
        virtual ~oper() {}

        virtual int priority() const = 0;
        virtual int operand_count() const = 0;
        virtual char content() const = 0;
        virtual void apply_operator(std::deque<automaton>& operands) = 0;

        virtual std::string serialize() const override {
            return std::string("oper\'") + content() + "\'";
        }

        virtual type get_type() const override {
            return OPERATOR;
        }

        static constexpr inline bool is_operator(char c);
        static std::unique_ptr<oper> operator_from(char op);
    };

    class oper_bracket : public oper {
    public:
        explicit oper_bracket(char ch = '(') : __content(ch) {}

        int priority() const override       { return -1; }
        int operand_count() const override  { return (__content == ')') ? 1 : 2; }
        char content() const override       { return __content; }

        type get_type() const override { return (__content == '(') ? LEFT_BRACKET : RIGHT_BRACKET; }

        void apply_operator(std::deque<automaton>& operands) override {}
    private:
        char __content;
    };

    class oper_plus : public oper {
    public:
        int priority() const override       { return 2; }
        int operand_count() const override  { return 1; }
        char content() const override       { return '+'; }

        void apply_operator(std::deque<automaton>& operands) override {
            operands.back().refactor_to_repetitive();
        }
    };

    class oper_optional : public oper {
    public:
        int priority() const override       { return 2; }
        int operand_count() const override  { return 1; }
        char content() const override       { return '?'; }

        void apply_operator(std::deque<automaton>& operands) override {
            operands.back().refactor_to_skippable();
        }
    };

    class oper_asterisk : public oper {
    public:
        int priority() const override       { return 2; }
        int operand_count() const override  { return 1; }
        char content() const override       { return '*'; }

        void apply_operator(std::deque<automaton>& operands) override {
            operands.back().refactor_to_repetitive();
            operands.back().refactor_to_skippable();
        }
    };

    class oper_concat : public oper {
    public:
        int priority() const override       { return 1; }
        int operand_count() const override  { return 2; }
        char content() const override       { return 'C'; }

        void apply_operator(std::deque<automaton>& operands) override {
            automaton& r2 = operands[operands.size()-1];
            automaton& r1 = operands[operands.size()-2];
            r1.connect(r2);
            operands.pop_back();
        }
    };

    class oper_or : public oper {
    public:
        int priority() const override       { return 0; }
        int operand_count() const override  { return 2; }
        char content() const override       { return '|'; }

        void apply_operator(std::deque<automaton>& operands) override {
            automaton& r2 = operands[operands.size()-1];
            automaton& r1 = operands[operands.size()-2];
            r1 |= r2;
            operands.pop_back();
        }
    };

    constexpr inline bool oper::is_operator(char c) {
        switch (c) {
        case '(':
        case ')':
        case '+':
        case '?':
        case '*':
        case '|':
        case '\0':
            return true;
        default:
            return false;
        }
    }

    std::unique_ptr<oper> oper::operator_from(char op) {
        switch (op) {
        case '(':
            return std::make_unique<oper_bracket>('(');
        case ')':
            return std::make_unique<oper_bracket>(')');
        case '+':
            return std::make_unique<oper_plus>();
        case '?':
            return std::make_unique<oper_optional>();
        case '*':
            return std::make_unique<oper_asterisk>();
        case '|':
            return std::make_unique<oper_or>();
        case '\0':
            return std::make_unique<oper_concat>();
        default:
            return std::make_unique<oper_concat>();
        }
    }

    static std::vector<std::shared_ptr<token>> tokenize(std::string_view sv) {
        std::vector<std::shared_ptr<token>> tokens;

        std::shared_ptr<oper> last_oper = oper::operator_from('\0');

        std::string_view::size_type from_index, to_index;
        from_index = to_index = 0;
        for (to_index = 0; to_index < sv.size(); to_index++) {
            if (oper::is_operator(sv[to_index])) {
                if (from_index != to_index) {
                    tokens.push_back(std::make_shared<plain_string>(sv.substr(from_index, to_index - from_index)));
                    if (sv[to_index] == '(') {
                        tokens.push_back(oper::operator_from('\0'));
                    }
                } else if (sv[to_index] == '(' && last_oper->operand_count() == 1) {
                    tokens.push_back(oper::operator_from('\0'));
                }
                tokens.push_back(oper::operator_from(sv[to_index]));
                from_index = to_index + 1;
                last_oper = oper::operator_from(sv[to_index]);
            } else if (
                from_index < to_index &&
                to_index + 1 < sv.size() &&
                oper::is_operator(sv[to_index + 1]) &&
                oper::operator_from(sv[to_index + 1])->priority() > oper::operator_from('\0')->priority()
            ) {
                tokens.push_back(std::make_shared<plain_string>(sv.substr(from_index, to_index - from_index)));
                tokens.push_back(oper::operator_from('\0'));
                tokens.push_back(std::make_shared<plain_string>(sv.substr(to_index, 1)));
                from_index = to_index + 1;
                last_oper = oper::operator_from('\0');
            } else {
                if (last_oper->operand_count() == 1) {
                    tokens.push_back(oper::operator_from('\0'));
                }
                last_oper = oper::operator_from('\0');
            }
        }

        if (from_index != to_index) {
            tokens.push_back(std::make_shared<plain_string>(sv.substr(from_index, to_index - from_index)));
        }

        return tokens;
    }

    static automaton build_automaton(const std::vector<std::shared_ptr<token>>& tokens) {
        std::deque<automaton> operands;
        std::deque<std::shared_ptr<oper>> opers;

        for (const std::shared_ptr<token>& tk : tokens) {
            switch (tk->get_type()) {
            case token::STRING:
                operands.push_back(automaton::string_automaton(dynamic_cast<plain_string&>(*tk).content()));
                break;
            case token::OPERATOR:
                {
                    oper& op = dynamic_cast<oper&>(*tk);
                    while (!opers.empty() && opers.back()->priority() > op.priority()) {
                        opers.back()->apply_operator(operands);
                        opers.pop_back();
                    }
                    opers.push_back(std::dynamic_pointer_cast<oper>(tk));
                }
                break;
            case token::LEFT_BRACKET:
                opers.push_back(std::dynamic_pointer_cast<oper>(tk));
                break;
            case token::RIGHT_BRACKET:
                {
                    while (!opers.empty() && opers.back()->get_type() != token::LEFT_BRACKET) {
                        opers.back()->apply_operator(operands);
                        opers.pop_back();
                    }
                    if (!opers.empty() && opers.back()->get_type() == token::LEFT_BRACKET) {
                        opers.pop_back();
                    }
                }
                break;
            }
        }

        while (!opers.empty()) {
            assert(opers.back()->get_type() == token::OPERATOR);
            opers.back()->apply_operator(operands);
            opers.pop_back();
        }

        assert(operands.size() == 1);

        return operands.back();
    }


    automaton::single_state automaton::add_state() {
        nodes.push_back({.next={}, .eps_next={}});
        return nodes.size() - 1;
    }

    void automaton::add_jump(single_state from, char ch, single_state to) {
        nodes[from].next.insert(std::make_pair(ch, to));
    }

    void automaton::add_epsilon_jump(single_state from, single_state to) {
        nodes[from].eps_next.insert(to);
    }

    bool automaton::contains_epsilon_jump(single_state from, single_state to) const {
        return nodes[from].eps_next.count(to) > 0;
    }

    automaton::state automaton::epsilon_closure(single_state s) const {
        return epsilon_closure(state{s});
    }

    automaton::state automaton::epsilon_closure(state states) const {
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

    automaton::state automaton::next_state(single_state prev, char ch) const {
        auto iterator_begin = nodes[prev].next.find(ch);
        auto iterator_end = iterator_begin;

        state st;
        while (iterator_end != nodes[prev].next.end() && iterator_end->first == ch) {
            st.insert(iterator_end->second);
            iterator_end++;
        }

        return epsilon_closure(std::move(st));
    }

    automaton::state automaton::next_state(const state& prev, char ch) const {
        state s;
        for (single_state ss : prev) {
            s += next_state(ss, ch);
        }
        return s;
    }

    automaton::state automaton::start_state() const {
        return epsilon_closure(start_sstate);
    }

    automaton::single_state automaton::start_single_state() const {
        return start_sstate;
    }

    void automaton::set_stop_state(single_state s, bool stop) {
        if (stop) {
            stop_sstates.insert(s);
        } else {
            stop_sstates.erase(s);
        }
    }

    bool automaton::is_stop_state(single_state s) const {
        return stop_sstates.count(s);
    }

    bool automaton::is_stop_state(const state& s) const {
        for (auto ss : s) {
            if (is_stop_state(ss)) return true;
        }
        return false;
    }

    void automaton::add_automaton(single_state from, const automaton& atm) {
        auto [start, stop] = import_automaton(atm);
        
        add_epsilon_jump(from, start);
        stop_sstates += stop;
    }

    void automaton::refactor_to_repetitive() {
        unify_stop_sstates();

        if (stop_sstates.size() == 0) {
            return;
        }

        if (contains_epsilon_jump(*stop_sstates.begin(), start_sstate)) {
            return;
        }

        add_epsilon_jump(*stop_sstates.begin(), start_sstate);
    }

    void automaton::refactor_to_skippable() {
        unify_stop_sstates();

        if (stop_sstates.size() == 0) {
            return;
        }

        if (contains_epsilon_jump(start_sstate, *stop_sstates.begin())) {
            return;
        }

        add_epsilon_jump(start_sstate, *stop_sstates.begin());
    }

    void automaton::connect(const automaton& atm) {
        unify_stop_sstates();

        single_state sstate = *stop_sstates.begin();
        stop_sstates.clear();

        add_automaton(sstate, atm);
    }

    automaton& automaton::operator|=(const automaton& m2) {
        add_automaton(start_sstate, m2);
        return *this;
    }

    
    std::pair<automaton::single_state, std::set<automaton::single_state>> automaton::import_automaton(const automaton& atm) {
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

    void automaton::unify_stop_sstates() {
        if (stop_sstates.size() <= 1) return;

        single_state new_stop = add_state();
        for (single_state sstate : stop_sstates) {
            add_epsilon_jump(sstate, new_stop);
        }

        stop_sstates = {new_stop};
    }

    automaton automaton::string_automaton(std::string_view s) {
        automaton atm;

        auto state = atm.start_single_state();
        for (char c : s) {
            auto next_state = atm.add_state();
            atm.add_jump(state, c, next_state);
            state = next_state;
        }
        atm.set_stop_state(state);

        return atm;
    }

    regex::regex(std::string_view sv) : atm(build_automaton(tokenize(sv))) {}

    bool regex::match(std::string_view sv) const {
        automaton::state s = atm.start_state();
        for (char c : sv) {
            s = atm.next_state(s, c);
        }
        return atm.is_stop_state(s);
    }
}
