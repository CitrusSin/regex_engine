#include <iostream>
#include <sstream>
#include <string>

#include "regex.hpp"
#include "regex_dfa.hpp"
#include "regex_nfa.hpp"

using namespace std;
using regexs::regex;
using namespace regexs::literal;

template <typename T>
static string serialize_set(const set<T>& s) {
    stringstream ss;
    ss << '{';
    bool fsm = true;
    for (auto& val : s) {
        if (!fsm) ss << ", ";
        fsm = false;
        ss << val;
    }
    ss << '}';
    return ss.str();
}

int main(int argc, char **argv) {
    size_t n;
    cout << "输入正则表达式数量：";
    cin >> n;
    while (cin.peek() != '\n') cin.get();
    cin.get();

    regexs::nondeterministic_automaton nfa;

    for (size_t i=0; i<n; i++) {
        string regex_str;
        cout << "输入" << i << "号正则表达式：";
        getline(cin, regex_str);

        auto automaton = regex(regex_str).automaton();
        automaton.add_end_state_mark(i);
        nfa.add_automaton(nfa.start_single_state(), automaton);
    }

    regexs::deterministic_automaton dfa = nfa.to_deterministic();
    
    cout << "确定自动机：\n" << dfa.serialize() << "\n";

    string input_str;
    while (1) {
        cout << "输入字符串（空格结束）：";
        getline(cin, input_str);

        if (input_str.empty()) {
            break;
        }

        regexs::deterministic_automaton::state st = dfa.start_state();
        for (char c : input_str) {
            st = dfa.next_state(st, c);
        }

        if (dfa.is_stop_state(st)) {
            set<int> mark = dfa.state_mark(st);
            cout << "匹配结果：" << serialize_set(mark) << '\n';
        } else {
            cout << "无匹配项\n";
        }
    }

    return 0;
}