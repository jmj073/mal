#include <cassert>

#include "printer.h"

using namespace std;

static string pr_list(const MalList* l) {
    const auto& v = l->data;
    string ret = "(";

    for (size_t i = 0; i < v.size(); ++i) {
        ret += pr_str(v[i].get());
        if (i + 1 != v.size()) {
            ret += ' ';
        }
    }

    return ret + ")";
}

static string pr_number(const MalNumber* n) {
    return to_string(n->data);
}

static string pr_symbol(const MalSymbol* s) {
    return s->data;
}

string pr_str(const MalType* ast) {
    if (auto l = dynamic_cast<const MalList*>(ast)) {
        return pr_list(l);
    } else if (auto n = dynamic_cast<const MalNumber*>(ast)) {
        return pr_number(n);
    } else if (auto s = dynamic_cast<const MalSymbol*>(ast)) {
        return pr_symbol(s);
    }

    assert(!"invalid type");
}
