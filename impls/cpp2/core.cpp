#include <ranges>
#include <numeric>
#include <algorithm>
#include <functional>
#include <iostream>
#include <fstream>

#include "core.h"
#include "reader.h"
#include "printer.h"
#include "eval.h"

using namespace std;

template <typename T>
auto arg_transformer(const MalType& arg) {
    return echanger(
        [&]() { return get<T>(arg); },
        [&]() { return MalRuntimeError("invalid argument type: " + MalTypeToString(arg)); }
    );
}

static inline void argument_count_checker(const vector<MalType>& args, size_t cnt) {
    if (args.size() != cnt) {
        throw MalRuntimeError("invalid argument count: " + to_string(args.size()));
    }
}

template <typename T>
void argument_type_checker(const MalType& arg) {
    if (!holds_alternative<T>(arg)) {
        throw MalRuntimeError("invalid argument type: " + MalTypeToString(arg));
    }
}

static string tostr(const vector<MalType>& args, const string& sep, bool print_readably) {
    auto r = args
        | views::transform([=](auto&& arg) { return pr_str(arg, print_readably); });
    vector<string> strs(r.begin(), r.end());

    string ret;

    for (size_t i = 0; i < strs.size(); ++i) {
        ret += strs[i];
        if (i + 1 != strs.size()) {
            ret += sep;
        }
    }

    return ret;
}

static shared_ptr<MalList> vec2ls(shared_ptr<MalVector> vec) {
    auto ls = make_shared<MalList>();
    ls->data = MalList::T(vec->data.begin(), vec->data.end());
    return ls;
}

static MalType mal_plus(const vector<MalType>& args) {
    auto int_args = args
        | views::transform(arg_transformer<MalNumber>)
        | views::transform([](auto arg) { return arg.data; });
    return MalNumber(accumulate(int_args.begin(), int_args.end(), 0));
}

static MalType mal_minus(const vector<MalType>& args) {
    auto int_args = args
        | views::transform(arg_transformer<MalNumber>)
        | views::transform([](auto arg) { return arg.data; });
    auto vec = vector(int_args.begin(), int_args.end());
    if (vec.empty()) {
        throw MalRuntimeError("invalid argument count: 0");
    }
    if (vec.size() == 1) {
        return MalNumber(-vec[0]);
    }
    return MalNumber(vec[0] - accumulate(vec.begin() + 1, vec.end(), 0));
}

static MalType mal_multiply(const vector<MalType>& args) {
    auto int_args = args
        | views::transform(arg_transformer<MalNumber>)
        | views::transform([](auto arg) { return arg.data; });
    return MalNumber(accumulate(int_args.begin(), int_args.end(), 1, multiplies<>()));
}

static MalType mal_divide(const vector<MalType>& args) {
    auto int_args = args
        | views::transform(arg_transformer<MalNumber>)
        | views::transform([](auto arg) { return arg.data; });
    auto vec = vector(int_args.begin(), int_args.end());
    if (vec.empty()) {
        throw MalRuntimeError("invalid argument count: 0");
    }
    if (vec.size() == 1) {
        return MalNumber(1 / vec[0]);
    }
    return MalNumber(vec[0] / accumulate(vec.begin() + 1, vec.end(), 1, multiplies<>()));
}

static MalType mal_list(const vector<MalType>& args) {
    return make_shared<MalList>(list<MalType>(args.begin(), args.end()));
}

static MalType mal_is_list(const vector<MalType>& args) {
    argument_count_checker(args, 1);
    return MalBool(holds_alternative<shared_ptr<MalList>>(args.front()));
}

static MalType mal_is_empty(const vector<MalType>& args) {
    argument_count_checker(args, 1);

    return MalBool(visit([](auto&& v) -> bool {
        using T = decay_t<decltype(v)>;

        if constexpr (is_same_v<T, shared_ptr<MalList>>)
            return v->data.empty();
        if constexpr (is_same_v<T, shared_ptr<MalVector>>)
            return v->data.empty();
        if constexpr (is_same_v<T, MalNil>)
            return true;

        throw MalRuntimeError("invalid argument type: " + MalTypeToString(v));
    }, args.front()));
}

static MalType mal_count(const vector<MalType>& args) {
    argument_count_checker(args, 1);

    return MalNumber(visit([](auto&& v) -> int {
        using T = decay_t<decltype(v)>;

        if constexpr (is_same_v<T, shared_ptr<MalList>>)
            return v->data.size();
        if constexpr (is_same_v<T, shared_ptr<MalVector>>)
            return v->data.size();
        if constexpr (is_same_v<T, MalNil>)
            return 0;

        throw MalRuntimeError("invalid argument type: " + MalTypeToString(v));
    }, args.front()));
}

static bool _equal(const MalType& _a, const MalType& _b) {
    auto x = _a;
    auto y = _b;

    if (holds_alternative<shared_ptr<MalVector>>(x)) {
        x = vec2ls(get<shared_ptr<MalVector>>(x));
    }
    if (holds_alternative<shared_ptr<MalVector>>(y)) {
        y = vec2ls(get<shared_ptr<MalVector>>(y));
    }

    if (x.index() != y.index()) return false;

    return visit([](auto&& a, auto&& b) -> bool {
        using T = decay_t<decltype(a)>;
        using U = decay_t<decltype(b)>;
        if constexpr (is_all_same_v<T, U, MalNil>) {
            return true;
        } else if constexpr (is_all_same_v<T, U, shared_ptr<MalList>>
                || is_all_same_v<T, U, shared_ptr<MalVector>>) {
            if (a->data.size() != b->data.size()) return false;

            auto ait = a->data.begin();
            auto bit = b->data.begin();

            while (ait != a->data.end()) {
                if (!_equal(*ait++, *bit++)) {
                    return false;
                }
            }

            return true;
        } else if constexpr (is_all_same_v<T, U, shared_ptr<MalHashmap>>) {
            if (a->data.size() != b->data.size()) return false;

            for (auto& [k, v]: a->data) {
                auto it = b->data.find(k);
                if (it == b->data.end()) {
                    return false;
                }
                if (!_equal(v, it->second)) {
                    return false;
                }
            }
            
            return true;
        } else if constexpr (is_all_same_v<T, U, shared_ptr<MalFunction>>) {
            return a.get() == b.get();
        } else if constexpr (is_all_same_v<T, U, shared_ptr<MalAtom>>) {
            return _equal(a->data, b->data);
        } else if constexpr (is_all_same_v<T, U>) {
            return a.data == b.data;
        }
        return false;
    }, x, y);
}

static MalType mal_equal_value(const vector<MalType>& args) {
    if (args.empty()) return MalBool(true);

    for (auto it = args.begin() + 1; it != args.end(); ++it) {
        if (!_equal(args.front(), *it)) {
            return MalBool(false);
        }
    }
    
    return MalBool(true);
}

static MalType mal_equal(const vector<MalType>& args) {
    if (args.empty()) return MalBool(true);

    for (auto it = args.begin() + 1; it != args.end(); ++it) {
        bool cond = visit([](auto&& a, auto&& b) {
            using T = decay_t<decltype(a)>;
            using U = decay_t<decltype(b)>;

            if constexpr (is_shared_ptr_v<T>
                    && is_shared_ptr_v<U>) {
                return (const void*)a.get() == (const void*)b.get();
            }

            return _equal(a, b);
        }, args.front(), *it);

        if (!cond) {
            return MalBool(false);
        }
    }
    
    return MalBool(true);
}

static MalType mal_less(const vector<MalType>& args) {
    auto nums = args
        | views::transform(arg_transformer<MalNumber>)
        | views::transform([](auto arg) { return arg.data; });

    return MalBool(adjacent_find(nums.begin(), nums.end(),
        greater_equal<>()) == nums.end());
}

static MalType mal_less_equal(const vector<MalType>& args) {
    auto nums = args
        | views::transform(arg_transformer<MalNumber>)
        | views::transform([](auto arg) { return arg.data; });

    return MalBool(adjacent_find(nums.begin(), nums.end(),
        greater<>()) == nums.end());
}

static MalType mal_greater(const vector<MalType>& args) {
    auto nums = args
        | views::transform(arg_transformer<MalNumber>)
        | views::transform([](auto arg) { return arg.data; });

    return MalBool(adjacent_find(nums.begin(), nums.end(),
        less_equal<>()) == nums.end());
}

static MalType mal_greater_equal(const vector<MalType>& args) {
    auto nums = args
        | views::transform(arg_transformer<MalNumber>)
        | views::transform([](auto arg) { return arg.data; });

    return MalBool(adjacent_find(nums.begin(), nums.end(),
        less<>()) == nums.end());
}

static MalType mal_pr_str(const vector<MalType>& args) {
    return MalString(tostr(args, " ", true));
}

static MalType mal_str(const vector<MalType>& args) {
    return MalString(tostr(args, "", false));
}

static MalType mal_prn(const vector<MalType>& args) {
    cout << tostr(args, " ", true) << endl;
    return MalNil();
}

static MalType mal_println(const vector<MalType>& args) {
    cout << tostr(args, " ", false) << endl;
    return MalNil();
}

static MalType mal_read_string(const vector<MalType>& args) {
    argument_count_checker(args, 1);
    return read_str(get<MalString>(args[0]).data);
}

static MalType mal_slurp(const vector<MalType>& args) {
    argument_count_checker(args, 1);
    auto& path = get<MalString>(args[0]).data;
    
    ifstream file(path);

    if (!file) {
        throw MalRuntimeError("can't open file '" + path + "'");
    }

    string str((istreambuf_iterator<char>(file)),
               istreambuf_iterator<char>());

    return MalString(std::move(str));
}

static MalType mal_eval(const vector<MalType>& args) {
    argument_count_checker(args, 1);
    return eval(args[0], repl_env);
}

static MalType mal_atom(const vector<MalType>& args) {
    argument_count_checker(args, 1);
    return make_shared<MalAtom>(args.front());
}

static MalType mal_is_atom(const vector<MalType>& args) {
    argument_count_checker(args, 1);
    return MalBool(holds_alternative<shared_ptr<MalAtom>>(args.front()));
}

static MalType mal_deref(const vector<MalType>& args) {
    argument_count_checker(args, 1);
    return arg_transformer<shared_ptr<MalAtom>>(args.front())->data;
}

static MalType mal_reset(const vector<MalType>& args) {
    argument_count_checker(args, 2);
    return arg_transformer<shared_ptr<MalAtom>>(args.front())->data = args[1];
}

static MalType mal_swap(const vector<MalType>& args) {
    if (args.size() < 2) {
        throw MalRuntimeError("invalid argument count:" + to_string(args.size()));
    }

    auto atom = arg_transformer<shared_ptr<MalAtom>>(args[0]);
    auto fn = arg_transformer<shared_ptr<MalFunction>>(args[1]);
    auto ls = make_shared<MalList>();

    ls->data.push_back(fn);
    ls->data.push_back(atom->data);
    ls->data.insert(ls->data.end(), args.begin() + 2, args.end());

    return atom->data = eval(ls, repl_env);
}

static MalType mal_cons(const vector<MalType>& args) {
    argument_count_checker(args, 2);
    
    return visit([&](auto&& v) -> MalType {
        using T = decay_t<decltype(v)>;

        if constexpr (is_same_v<T, shared_ptr<MalList>>
                || is_same_v<T, shared_ptr<MalVector>>) {
            MalList::T ls;
            ls.push_back(args[0]);
            ls.insert(ls.end(), v->data.begin(), v->data.end());
            return make_shared<MalList>(std::move(ls));
        }

        throw MalRuntimeError("invalid argument type: " + MalTypeToString(v));
    }, args[1]);
}

static MalType mal_concat(const vector<MalType>& args) {
    MalList::T ls;

    for (auto& arg: args) {
        visit([&](auto&& v) {
            using T = decay_t<decltype(v)>;

            if constexpr (is_same_v<T, shared_ptr<MalList>>
                    || is_same_v<T, shared_ptr<MalVector>>) {
                ls.insert(ls.end(), v->data.begin(), v->data.end());
                return;
            }

            throw MalRuntimeError("invalid argument type: " + MalTypeToString(v));
        }, arg);
    }

    return make_shared<MalList>(std::move(ls));
}

static optional<MalType> unbox_unquote(const MalType& ast) {
    if (!holds_alternative<shared_ptr<MalList>>(ast)) {
        return nullopt;
    }
    auto ls = get<shared_ptr<MalList>>(ast);
    if (ls->data.empty()) return nullopt;
    if (!holds_alternative<MalSymbol>(ls->data.front())) {
        return nullopt;
    }
    auto sym = get<MalSymbol>(ls->data.front());
    if (sym.data != "unquote") return nullopt;
    if (ls->data.size() != 2) {
        throw MalEvalFailed(ls, "invalid unquote form");
    }
    return ls->data.back();
}

static optional<MalType> unbox_splice_unquote(const MalType& ast) {
    if (!holds_alternative<shared_ptr<MalList>>(ast)) {
        return nullopt;
    }
    auto ls = get<shared_ptr<MalList>>(ast);
    if (ls->data.empty()) return nullopt;
    if (!holds_alternative<MalSymbol>(ls->data.front())) {
        return nullopt;
    }
    auto sym = get<MalSymbol>(ls->data.front());
    if (sym.data != "splice-unquote") return nullopt;
    if (ls->data.size() != 2) {
        throw MalEvalFailed(ls, "invalid splice-unquote form");
    }
    return ls->data.back();
}

static MalType _quasiquote(const MalType& ast) {
    if (auto opt = unbox_unquote(ast)) {
        return *opt;
    }

    if (holds_alternative<shared_ptr<MalList>>(ast)) {
        MalList::T res;
        auto ls = get<shared_ptr<MalList>>(ast);

        for (auto rit = ls->data.rbegin(); rit != ls->data.rend(); ++rit) {
            if (auto opt = unbox_splice_unquote(*rit)) {
                MalList::T tmp;
                tmp.push_back(MalSymbol("concat"));
                tmp.push_back(*opt);
                tmp.push_back(make_shared<MalList>(std::move(res)));
                res = std::move(tmp);
            } else {
                MalList::T tmp;
                tmp.push_back(MalSymbol("cons"));
                tmp.push_back(_quasiquote(*rit));
                tmp.push_back(make_shared<MalList>(std::move(res)));
                res = std::move(tmp);
            }
        }

        return make_shared<MalList>(std::move(res));
    }

    if (holds_alternative<shared_ptr<MalHashmap>>(ast)
            || holds_alternative<MalSymbol>(ast)) {
        auto ls = make_shared<MalList>();
        ls->data.push_back(MalSymbol("quote"));
        ls->data.push_back(ast);
        return ls;
    }

    if (holds_alternative<shared_ptr<MalVector>>(ast)) {
        MalList::T res;
        res.push_back(MalSymbol("vec"));
        res.push_back(_quasiquote(vec2ls(get<shared_ptr<MalVector>>(ast))));
        return make_shared<MalList>(std::move(res));
    }

    return ast;
}

static MalType mal_quasiquote(const vector<MalType>& args) {
    argument_count_checker(args, 1);
    return _quasiquote(args.front());
}

static MalType mal_vec(const vector<MalType>& args) {
    argument_count_checker(args, 1);

    return visit([&](auto&& v) -> MalType {
        using T = decay_t<decltype(v)>;

        if constexpr (is_same_v<T, shared_ptr<MalList>>
                || is_same_v<T, shared_ptr<MalVector>>) {
            MalVector::T vec(v->data.begin(), v->data.end());
            return make_shared<MalVector>(std::move(vec));
        }

        throw MalRuntimeError("invalid argument type: " + MalTypeToString(v));
    }, args.front());
}

unordered_map<string, MalFunction> core_fn{
    { "+", MalFunction(mal_plus) },
    { "-", MalFunction(mal_minus) },
    { "*", MalFunction(mal_multiply) },
    { "/", MalFunction(mal_divide) },
    { "list", MalFunction(mal_list) },
    { "list?", MalFunction(mal_is_list) },
    { "empty?", MalFunction(mal_is_empty) },
    { "count", MalFunction(mal_count) },
    { "=", MalFunction(mal_equal_value) },
    { "eq?", MalFunction(mal_equal) },
    { "<", MalFunction(mal_less) },
    { "<=", MalFunction(mal_less_equal) },
    { ">", MalFunction(mal_greater) },
    { ">=", MalFunction(mal_greater_equal) },
    { "pr-str", MalFunction(mal_pr_str) },
    { "str", MalFunction(mal_str) },
    { "prn", MalFunction(mal_prn) },
    { "println", MalFunction(mal_println) },
    { "read-string", MalFunction(mal_read_string) },
    { "slurp", MalFunction(mal_slurp) },
    { "eval", MalFunction(mal_eval) },
    { "atom", MalFunction(mal_atom) },
    { "atom?", MalFunction(mal_is_atom) },
    { "deref", MalFunction(mal_deref) },
    { "reset!", MalFunction(mal_reset) },
    { "swap!", MalFunction(mal_swap) },
    { "cons", MalFunction(mal_cons) },
    { "concat", MalFunction(mal_concat) },
    { "quasiquote", MalFunction(mal_quasiquote) },
    { "vec", MalFunction(mal_vec) },
};

