#include <cassert>
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

static string pr_list(const MalList* l, bool print_readably) {
    const auto& v = l->data;
    string ret = "(";

    for (size_t i = 0; i < v.size(); ++i) {
        ret += pr_str(v[i].get(), print_readably);
        if (i + 1 != v.size()) {
            ret += ' ';
        }
    }

    return ret + ")";
}

static string pr_number(const MalNumber* n, bool print_readably) {
    return to_string(n->data);
}

static string pr_symbol(const MalSymbol* s, bool print_readably) {
    return s->data;
}

static string pr_nil(const MalNil* nil, bool print_readably) {
    return "nil";
}

static string pr_bool(const MalBool* b, bool print_readably) {
    return b-> data ? "true" : "false";
}

static string pr_string(const MalString* s, bool print_readably) {
    return '"' + (print_readably ? encode_string(s->data) : s->data) + '"';
}

static string pr_keyword(const MalKeyword* k, bool print_readably) {
    return ':' + k->data;
}

string pr_str(const MalType* ast, bool print_readably) {
    if (auto l = dynamic_cast<const MalList*>(ast)) {
        return pr_list(l, print_readably);
    } else if (auto n = dynamic_cast<const MalNumber*>(ast)) {
        return pr_number(n, print_readably);
    } else if (auto s = dynamic_cast<const MalSymbol*>(ast)) {
        return pr_symbol(s, print_readably);
    } else if (auto nil = dynamic_cast<const MalNil*>(ast)) {
        return pr_nil(nil, print_readably);
    } else if (auto b = dynamic_cast<const MalBool*>(ast)) {
        return pr_bool(b, print_readably);
    } else if (auto s = dynamic_cast<const MalString*>(ast)) {
        return pr_string(s, print_readably);
    } else if (auto k = dynamic_cast<const MalKeyword*>(ast)) {
        return pr_keyword(k, print_readably);
    }

    assert(!"invalid type");
}
