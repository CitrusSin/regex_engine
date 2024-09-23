#include "regex_parse.hpp"
#include <stdexcept>

#define assert(condition) \
    if (!(condition)) throw std::runtime_error("Assertion failed: "#condition)

namespace regexs {
    std::vector<std::shared_ptr<token>> regex_tokenize(std::string_view sv) {
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

    nondeterministic_automaton build_nfa(const std::vector<std::shared_ptr<token>>& tokens) {
        std::deque<nondeterministic_automaton> operands;
        std::deque<std::shared_ptr<oper>> opers;

        for (const std::shared_ptr<token>& tk : tokens) {
            switch (tk->get_type()) {
            case token::STRING:
                operands.push_back(nondeterministic_automaton::string_automaton(dynamic_cast<plain_string&>(*tk).content()));
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
}