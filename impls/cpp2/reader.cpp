#include <regex>
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
MalType read_form(Reader<T>& reader);
template <typename T>
shared_ptr<MalList> read_list(Reader<T>& reader);
template <typename T>
shared_ptr<MalVector> read_vector(Reader<T>& reader);
template <typename T>
shared_ptr<MalHashmap> read_hashmap(Reader<T>& reader);
template <typename T>
MalType read_atom(Reader<T>& reader);

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
        reader.next();
        auto l = make_shared<MalList>();
        l->data.push_back(MalSymbol("quote"));
        l->data.push_back(read_form(reader));
        return l;
    } else if (token == "`") {
        reader.next();
        auto l = make_shared<MalList>();
        l->data.push_back(MalSymbol("quasiquote"));
        l->data.push_back(read_form(reader));
        return l;
    } else if (token == "~") {
        reader.next();
        auto l = make_shared<MalList>();
        l->data.push_back(MalSymbol("unquote"));
        l->data.push_back(read_form(reader));
        return l;
    } else if (token == "~@") {
        reader.next();
        auto l = make_shared<MalList>();
        l->data.push_back(MalSymbol("splice-unquote"));
        l->data.push_back(read_form(reader));
        return l;
    } else if (token == "@") {
        reader.next();
        auto l = make_shared<MalList>();
        l->data.push_back(MalSymbol("deref"));
        l->data.push_back(read_form(reader));
        return l;
    } else {
        return read_atom(reader);
    }
}

template <typename T>
shared_ptr<MalList> read_list(Reader<T>& reader) {
    auto token = reader.next();
    if (token != "(") {
        throw MalSyntaxError("unbalanced");
    }

    auto ob = string() + opposite_bracket(token[0]);
    auto ls = make_shared<MalList>();

    while (reader.peek() != ob) {
        auto token = reader.peek();
        ls->data.push_back(read_form(reader));
    }

    reader.next();

    return ls;
}

template <typename T>
shared_ptr<MalVector> read_vector(Reader<T>& reader) {
    auto token = reader.next();
    if (token != "[") {
        throw MalSyntaxError("unbalanced");
    }

    auto ob = string() + opposite_bracket(token[0]);
    auto vec = make_shared<MalVector>();

    while (reader.peek() != ob) {
        auto token = reader.peek();
        vec->data.push_back(read_form(reader));
    }

    reader.next();

    return vec;
}

template <typename T>
shared_ptr<MalHashmap> read_hashmap(Reader<T>& reader) {
    auto token = reader.next();
    if (token != "{") {
        throw MalSyntaxError("unbalanced");
    }

    auto ob = string() + opposite_bracket(token[0]);
    auto hm = make_shared<MalHashmap>();

    while (reader.peek() != ob) {
        auto k = visit([](auto&& v) -> MalHashmap::Key {
            using U = decay_t<decltype(v)>;
            if constexpr (is_same_v<U, MalString>) return v;
            if constexpr (is_same_v<U, MalKeyword>) return v;
            throw MalInvalidHashmapKey();
        }, read_form(reader));
        auto v = read_form(reader);

        if (hm->data.contains(k)) {
            throw MalInvalidHashmapKey();
        }

        hm->data[::std::move(k)] = v;
    }

    reader.next();

    return hm;
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
