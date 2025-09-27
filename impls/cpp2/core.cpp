#include <ranges>
#include <numeric>
#include <algorithm>
#include <functional>
#include <iostream>

#include "core.h"
#include "eval.h"
#include "printer.h"

using namespace std;

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
        throw MalEvalFailed("invalid argument count");
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
        throw MalEvalFailed("invalid argument count");
    }
    if (vec.size() == 1) {
        return MalNumber(1 / vec[0]);
    }
    return MalNumber(vec[0] / accumulate(vec.begin() + 1, vec.end(), 1, multiplies<>()));
}

static MalType mal_prn(const vector<MalType>& args) {
    if (args.size() != 1) {
        throw MalEvalFailed("invalid argument count");
    }

    cout << pr_str(args.front(), true) << endl;

    return MalNil();
}

static MalType mal_list(const vector<MalType>& args) {
    return make_shared<MalList>(list<MalType>(args.begin(), args.end()));
}

static MalType mal_is_list(const vector<MalType>& args) {
    if (args.size() != 1) {
        throw MalEvalFailed("invalid argument count");
    }
    return MalBool(holds_alternative<shared_ptr<MalList>>(args.front()));
}

static MalType mal_is_empty(const vector<MalType>& args) {
    if (args.size() != 1) {
        throw MalEvalFailed("invalid argument count");
    }

    return MalBool(visit([](auto&& v) -> bool {
        using T = decay_t<decltype(v)>;

        if constexpr (is_same_v<T, shared_ptr<MalList>>)
            return v->data.empty();
        if constexpr (is_same_v<T, MalNil>)
            return true;

        throw MalEvalFailed("invalid argument type");
    }, args.front()));
}

static MalType mal_count(const vector<MalType>& args) {
    if (args.size() != 1) {
        throw MalEvalFailed("invalid argument count");
    }

    return MalNumber(visit([](auto&& v) -> int {
        using T = decay_t<decltype(v)>;

        if constexpr (is_same_v<T, shared_ptr<MalList>>)
            return v->data.size();
        if constexpr (is_same_v<T, MalNil>)
            return 0;

        throw MalEvalFailed("invalid argument type");
    }, args.front()));
}

static bool _equal(const MalType& _a, const MalType& _b) {
    if (_a.index() != _b.index()) return false;

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
        } else if constexpr (is_all_same_v<T, U>) {
            return a.data == b.data;
        }
        return false;
    }, _a, _b);
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

unordered_map<string, MalFunction> core_fn{
    { "+", MalFunction(mal_plus) },
    { "-", MalFunction(mal_minus) },
    { "*", MalFunction(mal_multiply) },
    { "/", MalFunction(mal_divide) },
    { "prn", MalFunction(mal_prn) },
    { "list", MalFunction(mal_list) },
    { "list?", MalFunction(mal_is_list) },
    { "empty?", MalFunction(mal_is_empty) },
    { "count", MalFunction(mal_count) },
    { "=", MalFunction(mal_equal_value) },
    { "<", MalFunction(mal_less) },
    { "<=", MalFunction(mal_less_equal) },
    { ">", MalFunction(mal_greater) },
    { ">=", MalFunction(mal_greater_equal) },
    { "eq?", MalFunction(mal_equal) },
};

