#include <cstdio>
#include <iostream>
#include <string>
#include "regex.hpp"

using namespace std;
using regexs::regex;

int main() {
    string regex_str;

    cout << "输入正则表达式：";
    getline(cin, regex_str);

    regex reg(regex_str);

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