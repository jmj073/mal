#include <regex>

#include "printer.h"

using namespace std;

static string encode_string(const string& s) {
    std::string out = s;

    // 단순 치환: \, ", \n, \r, \t
    out = std::regex_replace(out, std::regex(R"(\\)"), R"(\\)");
    out = std::regex_replace(out, std::regex(R"(")"), R"(\")");
    out = std::regex_replace(out, std::regex(R"(\n)"), R"(\n)");
    out = std::regex_replace(out, std::regex(R"(\r)"), R"(\r)");
    out = std::regex_replace(out, std::regex(R"(\t)"), R"(\t)");

    // 비ASCII 문자 처리 (\xHH;), regex로 검색
    std::regex nonAscii(R"(([\x00-\x08\x0B\x0C\x0E-\x1F\x80-\xFF]))"); // 제어문자 + 비ASCII
    std::smatch match;
    std::string result;
    std::string::const_iterator searchStart(out.cbegin());

    while (std::regex_search(searchStart, out.cend(), match, nonAscii)) {
        result.append(match.prefix());
        unsigned char c = static_cast<unsigned char>(match[1].str()[0]);
        std::ostringstream oss;
        oss << "\\x" << std::hex << std::uppercase << (int)c << ";";
        result.append(oss.str());
        searchStart = match.suffix().first;
    }
    result.append(searchStart, out.cend());

    return result;
}

static string pr_list(const MalList& l, bool print_readably) {
    const auto& e = l.data;
    string ret = "(";

    for (auto it = e.begin(); it != e.end();) {
        ret += pr_str(*it, print_readably);
        if (++it == e.end()) {
            break;
        }
        ret += ' ';
    }

    return ret + ")";
}

static string pr_vector(const MalVector& v, bool print_readably) {
    const auto& e = v.data;
    string ret = "[";

    for (size_t i = 0; i < e.size(); ++i) {
        ret += pr_str(e[i], print_readably);
        if (i + 1 != e.size()) {
            ret += ' ';
        }
    }

    return ret + "]";
}

static string pr_hashmap(const MalHashmap& v, bool print_readably) {
    const auto& e = v.data;
    string ret = "{";

    for (auto it = e.begin(); it != e.end();) {
        ret += pr_str(MalKeyToMalType(it->first), print_readably) + ' ';
        ret += pr_str(it->second, print_readably);
        if (++it == e.end()) {
            break;
        }
        ret += ' ';
    }

    return ret + "}";
}

static string pr_number(const MalNumber& n, bool print_readably) {
    return to_string(n.data);
}

static string pr_symbol(const MalSymbol& s, bool print_readably) {
    return s.data;
}

static string pr_nil(const MalNil& nil, bool print_readably) {
    return "nil";
}

static string pr_bool(const MalBool& b, bool print_readably) {
    return b.data ? "true" : "false";
}

static string pr_string(const MalString& s, bool print_readably) {
    return '"' + (print_readably ? encode_string(s.data) : s.data) + '"';
}

static string pr_keyword(const MalKeyword& k, bool print_readably) {
    return ':' + k.data;
}

string pr_str(const MalType& ast, bool print_readably) {
    return visit([&](auto&& v) -> string {
        using T = decay_t<decltype(v)>;
        if constexpr (is_same_v<T, MalNumber>)
            return pr_number(v, print_readably);
        if constexpr (is_same_v<T, MalSymbol>)
            return pr_symbol(v, print_readably);
        if constexpr (is_same_v<T, MalNil>)
            return pr_nil(v, print_readably);
        if constexpr (is_same_v<T, MalBool>)
            return pr_bool(v, print_readably);
        if constexpr (is_same_v<T, MalString>)
            return pr_string(v, print_readably);
        if constexpr (is_same_v<T, MalKeyword>)
            return pr_keyword(v, print_readably);
        if constexpr (is_same_v<T, shared_ptr<MalList>>)
            return pr_list(*v, print_readably);
        if constexpr (is_same_v<T, shared_ptr<MalVector>>)
            return pr_vector(*v, print_readably);
        if constexpr (is_same_v<T, shared_ptr<MalHashmap>>)
            return pr_hashmap(*v, print_readably);
        if constexpr (is_same_v<T, shared_ptr<MalFunction>>)
            return "#<function>";
        return "<unknown>";
    }, ast);
}
