#include <regex>
#include <cassert>
#include <cctype>
#include <cstdio>

#include "reader.h"

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

auto tokenize(const string& str, const regex& re);

template <typename T>
unique_ptr<MalType> read_form(Reader<T>& reader);
template <typename T>
unique_ptr<MalList> read_list(Reader<T>& reader);
template <typename T>
unique_ptr<MalAtom> read_atom(Reader<T>& reader);

static string decode_string(const string& s) {
    string out = s;

    // 단순 치환
    out = std::regex_replace(out, std::regex(R"(\\n)"), "\n");
    out = std::regex_replace(out, std::regex(R"(\\t)"), "\t");
    out = std::regex_replace(out, std::regex(R"(\\r)"), "\r");
    out = std::regex_replace(out, std::regex(R"(\\\")"), "\"");
    out = std::regex_replace(out, std::regex(R"(\\\\)"), "\\");

    // \xHH 유니코드 처리
    std::regex hexPattern(R"(\\x([0-9A-Fa-f]+);)");
    std::smatch match;
    std::string result;
    std::string::const_iterator searchStart(out.cbegin());

    while (std::regex_search(searchStart, out.cend(), match, hexPattern)) {
        result.append(match.prefix()); // 매치 전까지 복사
        int code = std::stoi(match[1].str(), nullptr, 16);

        // 간단하게 BMP 영역까지만 UTF-8 변환
        if (code <= 0x7F) {
            result.push_back(static_cast<char>(code));
        } else if (code <= 0x7FF) {
            result.push_back(0xC0 | (code >> 6));
            result.push_back(0x80 | (code & 0x3F));
        } else {
            result.push_back(0xE0 | (code >> 12));
            result.push_back(0x80 | ((code >> 6) & 0x3F));
            result.push_back(0x80 | (code & 0x3F));
        }

        searchStart = match.suffix().first; // 다음 검색 위치 갱신
    }
    result.append(searchStart, out.cend()); // 마지막 부분 복사

    return result;
}


auto tokenize(const string& str, const regex& re) {
    return regex_view(str, re)
        | views::transform([] (const auto& m) { return m[1].matched ? m[1].str() : m.str(); })
        | views::filter([] (auto token) { return !token.empty(); })
        | views::filter([] (auto token) { return token[0] != ';'; });
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

    while (reader.peek() != ")") {
        auto token = reader.peek();
        assert(!token.empty());

        mal_list->data.push_back(read_form(reader));
    }

    reader.next();

    return mal_list;
}

template <typename T>
unique_ptr<MalAtom> read_atom(Reader<T>& reader) {
    static const regex num_regex(R"(^[+-]?\d+$)", regex::optimize);

    auto token = reader.next();

    if (token[0] == '"') { // string
        assert(token.back() == '"');
        auto str = string(token.begin() + 1, token.end() - 1);
        return make_unique<MalString>(decode_string(str));
    } else if (token == "nil") { // nil
        return make_unique<MalNil>();
    } else if (token == "true") { // true
        return make_unique<MalBool>(true);
    } else if (token == "false") { // false
        return make_unique<MalBool>(false);
    } else if (regex_match(token, num_regex)) { // number
        return make_unique<MalNumber>(stoi(token));
    } else { // symbol
        return make_unique<MalSymbol>(::std::move(token));
    }
}

unique_ptr<MalType> read_str(const string& str) {
    auto re = regex(R"([\s,]*(~@|[\[\]{}()'`~^@]|"(?:\\.|[^\\"])*"?|;.*|[^\s\[\]{}('"`,;)]*))");
    auto reader = make_reader(tokenize(str, re));
    return read_form(reader);
}
