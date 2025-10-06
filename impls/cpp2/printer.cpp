#include <regex>
#include <iostream>

#include "printer.h"
#include "util.h"

using namespace std;

static string encode_string(const string& s) {
    return regex_replace_callback(s, regex("[\\\\\n\t\r\b\"]"),
        [] (const smatch& m) -> string {
            switch (m.str()[0]) {
                case '\\': return R"(\\)";
                case '\n': return R"(\n)";
                case '\t': return R"(\t)";
                case '\r': return R"(\r)";
                case '\b': return R"(\b)";
                case '"':  return R"(\")";
            }
            return "";
        });
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
    return (print_readably ? '"' + encode_string(s.data) + '"' : s.data);
}

static string pr_keyword(const MalKeyword& k, bool print_readably) {
    return ':' + k.data;
}

static string pr_atom(const MalAtom& atom, bool print_readably) {
    return "(atom " + pr_str(atom.data, print_readably) + ")";
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
        if constexpr (is_same_v<T, shared_ptr<MalAtom>>)
            return pr_atom(*v, print_readably);
        return "<unknown>";
    }, ast);
}
