#include "regex_parse.hpp"
#include "regex_nfa.hpp"
#include <array>
#include <memory>
#include <stdexcept>

#define assert(condition) \
    if (!(condition)) throw std::runtime_error("Assertion failed: "#condition)

namespace regexs {
    std::vector<std::shared_ptr<token>> regex_tokenize(std::string_view sv) {
        std::vector<std::shared_ptr<token>> tokens;

        bool reading_string = true;

        std::string_view::size_type from_index, to_index;
        from_index = to_index = 0;
        for (to_index = 0; to_index < sv.size(); to_index++) {
            if (oper::is_operator(sv[to_index])) {
                if (from_index != to_index) {
                    tokens.push_back(std::make_shared<plain_string>(sv.substr(from_index, to_index - from_index)));
                    if (sv[to_index] == '(') {
                        tokens.push_back(std::make_shared<oper_concat>());
                    }
                } else if (sv[to_index] == '(' && !reading_string) {
                    tokens.push_back(std::make_shared<oper_concat>());
                }
                tokens.push_back(oper::operator_from(sv[to_index]));
                from_index = to_index + 1;
                reading_string = oper::operator_from(sv[to_index])->operand_count() == 2;
                continue;
            }
            if (
                from_index < to_index &&
                to_index + 1 < sv.size() &&
                oper::is_operator(sv[to_index + 1]) &&
                oper::operator_from(sv[to_index + 1])->priority() > oper_concat().priority()
            ) {
                tokens.push_back(std::make_shared<plain_string>(sv.substr(from_index, to_index - from_index)));
                tokens.push_back(std::make_shared<oper_concat>());
                tokens.push_back(std::make_shared<plain_string>(sv.substr(to_index, 1)));
                from_index = to_index + 1;
                reading_string = true;
                continue;
            }
            if (!reading_string) {
                tokens.push_back(std::make_shared<oper_concat>());
            }
            reading_string = true;
            if (sv[to_index] == '[') {
                if (from_index != to_index) {
                    tokens.push_back(std::make_shared<plain_string>(sv.substr(from_index, to_index - from_index)));
                    tokens.push_back(std::make_shared<oper_concat>());
                }
                std::string_view::size_type left, right;
                left = right = to_index;
                while (sv[right] != ']') {
                    if (sv[right] == '\\') right++;
                    right++;
                }
                tokens.push_back(std::make_shared<char_selector>(sv.substr(left, right - left + 1)));
                from_index = to_index = right;
                from_index++;
                reading_string = false;
                continue;
            }
        }

        if (from_index != to_index) {
            tokens.push_back(std::make_shared<plain_string>(sv.substr(from_index, to_index - from_index)));
        }

        return tokens;
    }

    nondeterministic_automaton build_nfa(const std::vector<std::shared_ptr<token>>& tokens) {
        std::deque<nondeterministic_automaton> operands;
        std::deque<std::shared_ptr<token>> opers;

        for (const std::shared_ptr<token>& tk : tokens) {
            switch (tk->get_type()) {
            case token::STRING:
                operands.push_back(string_automaton(dynamic_cast<plain_string&>(*tk).content()));
                break;
            case token::CHAR_SELECTOR:
                operands.push_back(selector_automaton(dynamic_cast<char_selector&>(*tk)));
                break;
            case token::OPERATOR:
                {
                    oper& op = dynamic_cast<oper&>(*tk);
                    while (
                        !opers.empty() &&
                        opers.back()->get_type() == token::OPERATOR &&
                        dynamic_cast<oper&>(*opers.back()).priority() > op.priority()
                    ) {
                        dynamic_cast<oper&>(*opers.back()).apply_operator(operands);
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
                    while (
                        !opers.empty() &&
                        opers.back()->get_type() == token::OPERATOR
                    ) {
                        dynamic_cast<oper&>(*opers.back()).apply_operator(operands);
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
            dynamic_cast<oper&>(*opers.back()).apply_operator(operands);
            opers.pop_back();
        }

        assert(operands.size() == 1);

        return operands.back();
    }

    nondeterministic_automaton string_automaton(std::string_view s) {
        nondeterministic_automaton atm;

        auto state = atm.start_single_state();
        for (char c : s) {
            auto next_state = atm.add_state();
            atm.add_jump(state, c, next_state);
            state = next_state;
        }
        atm.set_stop_state(state);

        return atm;
    }

    nondeterministic_automaton selector_automaton(const char_selector& selector) {
        using single_state = nondeterministic_automaton::single_state;
        
        std::string sel_content = selector.content();

        std::array<bool, 128> char_sel;
        char_sel.fill(false);
        bool negative = false;
        for (size_t i = 0; i < sel_content.size(); i++) {
            if (i == 0 && sel_content[i] == '^') {
                negative = true;
                continue;
            }
            if (sel_content[i] == '\\') {
                i++;
                if (sel_content[i] == '-') {
                    char_sel['-'] = true;
                    continue;
                }
            }
            if (i+2 < sel_content.size() && sel_content[i+1] == '-') {
                char from = sel_content[i], to = sel_content[i+2];
                if (to == '\\' && i+3 < sel_content.size()) {
                    to = sel_content[i+3];
                    i = i+3;
                } else {
                    i = i+2;
                }
                for (char c = from; c <= to; c++) {
                    char_sel[c] = true;
                }
                continue;
            }
            char_sel[sel_content[i]] = true;
        }
        if (negative) {
            for (bool& val : char_sel) {
                val = !val;
            }
        }


        nondeterministic_automaton atm;
        single_state start_state, stop_state;

        start_state = atm.start_single_state();
        atm.set_stop_state(stop_state = atm.add_state());
        
        for (char ch=0x20; ch<0x7f; ch++) {
            if (char_sel[ch]) {
                atm.add_jump(start_state, ch, stop_state);
            }
        }

        return atm;
    }
}