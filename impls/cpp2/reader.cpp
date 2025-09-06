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
    const string* m_str;
    const regex* m_re;
};

auto tokenize(const string& str, const regex& re);

template <typename T>
MalType read_form(Reader<T>& reader);
template <typename T>
MalList read_list(Reader<T>& reader);
template <typename T>
MalVector read_vector(Reader<T>& reader);
template <typename T>
MalHashmap read_hashmap(Reader<T>& reader);
template <typename T>
MalType read_atom(Reader<T>& reader);

static bool is_left_bracket(char c) {
    constexpr string_view brackets = "({[";
    return brackets.find(c) != string::npos;
}

static bool is_right_bracket(char c) {
    constexpr string_view brackets = ")}]";
    return brackets.find(c) != string::npos;
}

static bool is_bracket(char c) {
    return is_left_bracket(c) || is_right_bracket(c);
}

static char opposite_bracket(char c) {
    switch (c) {
        case '(': return ')';
        case ')': return '(';
        case '{': return '}';
        case '}': return '{';
        case '[': return ']';
        case ']': return '[';
    }

    return c;
}

static bool is_balanced_string(const string& s) {
    static const regex balanced(R"(^"(?:\\.|[^\\"])*"$)");
    return regex_match(s, balanced);
}

static string decode_string(const string& s) {
    string out = s;

    // 단순 치환
    out = regex_replace(out, regex(R"(\\n)"), "\n");
    out = regex_replace(out, regex(R"(\\t)"), "\t");
    out = regex_replace(out, regex(R"(\\r)"), "\r");
    out = regex_replace(out, regex(R"(\\\")"), "\"");
    out = regex_replace(out, regex(R"(\\\\)"), "\\");

    // \xHH 유니코드 처리
    regex hexPattern(R"(\\x([0-9A-Fa-f]+);)");
    smatch match;
    string result;
    string::const_iterator searchStart(out.cbegin());

    while (regex_search(searchStart, out.cend(), match, hexPattern)) {
        result.append(match.prefix()); // 매치 전까지 복사
        int code = stoi(match[1].str(), nullptr, 16);

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
MalType read_form(Reader<T>& reader) {
    auto token = reader.peek();
    if (token.empty()) {
        throw MalSyntaxError("EOF");
    }

    if (token == "(") {
        return read_list(reader);
    } else if (token == "[") {
        return read_vector(reader);
    } else if (token == "{") {
        return read_hashmap(reader);
    } else if (is_right_bracket(token[0])) {
        throw MalSyntaxError("unbalanced");
    } else if (token == "'") {
        auto l = MalList();
        l.data.push_back(MalSymbol("quote"));
        reader.next();
        l.data.push_back(read_form(reader));
        return l;
    } else if (token == "`") {
        auto l = MalList();
        l.data.push_back(MalSymbol("quasiquote"));
        reader.next();
        l.data.push_back(read_form(reader));
        return l;
    } else if (token == "~") {
        auto l = MalList();
        l.data.push_back(MalSymbol("unquote"));
        reader.next();
        l.data.push_back(read_form(reader));
        return l;
    } else if (token == "~@") {
        auto l = MalList();
        l.data.push_back(MalSymbol("splice-unquote"));
        reader.next();
        l.data.push_back(read_form(reader));
        return l;
    } else {
        return read_atom(reader);
    }
}

template <typename T>
MalList read_list(Reader<T>& reader) {
    auto token = reader.next();
    if (token != "(") {
        throw MalSyntaxError("unbalanced");
    }
    auto ob = string() + opposite_bracket(token[0]);

    auto mal_list = MalList();

    while (reader.peek() != ob) {
        auto token = reader.peek();

        if (token.empty()) {
            throw MalSyntaxError("unbalanced");
        }

        mal_list.data.push_back(read_form(reader));
    }

    reader.next();

    return mal_list;
}

template <typename T>
MalVector read_vector(Reader<T>& reader) {
    auto token = reader.next();
    if (token != "[") {
        throw MalSyntaxError("unbalanced");
    }
    auto ob = string() + opposite_bracket(token[0]);

    auto mal_vector = MalVector();

    while (reader.peek() != ob) {
        auto token = reader.peek();

        if (token.empty()) {
            throw MalSyntaxError("unbalanced");
        }

        mal_vector.data.push_back(read_form(reader));
    }

    reader.next();

    return mal_vector;
}

template <typename T>
MalHashmap read_hashmap(Reader<T>& reader) {
    auto token = reader.next();
    if (token != "{") {
        throw MalSyntaxError("unbalanced");
    }
    auto ob = string() + opposite_bracket(token[0]);

    auto mal_hashmap = MalHashmap();

    while (reader.peek() != ob) {
        auto token = reader.peek();

        if (token.empty()) {
            throw MalSyntaxError("unbalanced");
        }

        mal_hashmap.data.push_back(read_form(reader));
    }

    reader.next();

    return mal_hashmap;
}

template <typename T>
MalType read_atom(Reader<T>& reader) {
    static const regex num_regex(R"(^[+-]?\d+$)", regex::optimize);

    auto token = reader.next();
    if (token.empty()) {
        throw MalSyntaxError("EOF");
    }

    if (token[0] == '"') { // string
        if (!is_balanced_string(token)) {
            throw MalSyntaxError("unbalanced");
        }
        auto str = string(token.begin() + 1, token.end() - 1);
        return MalString(decode_string(str));
    } else if (token[0] == ':') { // keyword
        auto str = string(token.begin() + 1, token.end());
        return MalKeyword(str);
    } else if (isdigit(token[0])) { // number
        if (regex_match(token, num_regex)) { 
            return MalNumber(stoi(token));
        } else {
            throw MalSyntaxError("invalid number");
        }
    } else if (regex_match(token, num_regex)) {
        return MalNumber(stoi(token));
    } else if (token == "nil") { // nil
        return MalNil();
    } else if (token == "true") { // true
        return MalBool(true);
    } else if (token == "false") { // false
        return MalBool(false);
    } else { // symbol
        return MalSymbol(::std::move(token));
    }
}

MalType read_str(const string& str) {
    auto re = regex(R"([\s,]*(~@|[\[\]{}()'`~^@]|"(?:\\.|[^\\"])*"?|;.*|[^\s\[\]{}('"`,;)]*))");
    auto tokenizer = tokenize(str, re);
    if (tokenizer.begin() == tokenizer.end()) {
        throw MalNoToken();
    }
    auto reader = make_reader(::std::move(tokenizer));
    return read_form(reader);
}
