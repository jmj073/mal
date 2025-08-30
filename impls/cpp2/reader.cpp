#include <regex>
#include <ranges>
#include <cassert>
#include <cctype>

#include "reader.h"
#include "types.h"

using namespace std;
using namespace ranges;

struct regex_view : view_interface<regex_view> {
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
    const std::string* m_str;
    const std::regex* m_re;
};

unique_ptr<MalType> read_str(const string& str);
auto tokenize(const string& str, const regex& re);
template <typename T>
unique_ptr<MalType> read_form(Reader<T>& reader);
template <typename T>
unique_ptr<MalList> read_list(Reader<T>& reader);
template <typename T>
unique_ptr<MalAtom> read_atom(Reader<T>& reader);




auto tokenize(const string& str, const regex& re) {
    return regex_view(str, re)
        | views::transform([] (const auto& m) { return m[1].matched ? m[1].str() : m.str(); })
        | views::filter([] (auto token) { return !token.empty(); })
        | views::filter([] (auto token) { return token[0] == ';'; });
}

template <typename T>
unique_ptr<MalType> read_form(Reader<T>& reader) {
    auto token = reader.peek();

    switch (token[0]) {
        case '(':
            return read_list(reader);
        default:
            return read_atom(reader);
    }
}

template <typename T>
unique_ptr<MalList> read_list(Reader<T>& reader) {
    auto token = reader.next();
    assert(token == "(");

    auto mal_list = make_unique<MalList>();

    while (true) {
        auto token = reader.peek();
        assert(!token.empty());

        mal_list.push_back(read_form(reader));

        if (token == ")") {
            token.next();
            break;
        }
    }

    return mal_list;
}

template <typename T>
unique_ptr<MalAtom> read_atom(Reader<T>& reader) {
    auto token = reader.next();

    if (isdigit(token[0])) {
        return atoi(
    }
}


unique_ptr<MalType> read_str(const string& str) {
    auto re = regex(R"([\s,]*(~@|[\[\]{}()'`~^@]|"(?:\\.|[^\\"])*"?|;.*|[^\s\[\]{}('"`,;)]*))");
    auto reader = make_reader(tokenize(str, re));
    return read_form(reader);
}

int main() {
}
