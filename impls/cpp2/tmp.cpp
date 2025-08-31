#include <regex>
#include <ranges>
#include <iostream>

using namespace std;
using namespace ranges;

struct regex_view: view_interface<regex_view> {
    using iterator = sregex_iterator;

    regex_view(const string& str, const regex& re)
        : m_str(&str), m_re(&re) {}

    iterator begin() const {
        return sregex_iterator(m_str->begin(), m_str->end(), *m_re);
    }

    iterator end() const {
        return sregex_iterator();
    }

private:
    const string* m_str;
    const regex* m_re;
};


auto tokenize(const string& str, const regex& re) {
    return regex_view(str, re)
        | views::transform([] (const auto& m) { return m[1].matched ? m[1].str() : m.str(); })
        | views::filter([] (auto token) { return !token.empty(); })
        | views::filter([] (auto token) { return token[0] != ';'; });
}

int main() {
    auto re = regex(R"([\s,]*(~@|[\[\]{}()'`~^@]|"(?:\\.|[^\\"])*"?|;.*|[^\s\[\]{}('"`,;)]*))");

    string s;
    getline(cin, s);

    auto tokenizer = tokenize(s, re);

    for (auto token: tokenizer) {
        cout << token << endl;
    }
}
