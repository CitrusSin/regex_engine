#include <cstdio>
#include <iostream>
#include <string>

#include "regex.hpp"

using namespace std;
using regexs::regex;

int main(int argc, char **argv) {
    string regex_str;

    cout << "输入正则表达式：";
    getline(cin, regex_str);

    regex reg(regex_str);

    auto tokens = reg.tokens();
    cout << "分词结果：\n";
    for (auto& t : tokens) {
        cout << t << '\n';
    }

    cout << "\n自动机：\n" << reg.automaton().serialize() << "\n";
    cout << "确定自动机：\n" << reg.deter_automaton().serialize() << "\n";

    string input_str;
    while (1) {
        cout << "输入字符串（空格结束）：";
        getline(cin, input_str);

        if (input_str.empty()) {
            break;
        }

        bool ans = reg.match(input_str);

        cout << "匹配结果：" << (ans ? "匹配" : "不匹配") << '\n';
    }

    return 0;
}