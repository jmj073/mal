#include <ranges>
#include <numeric>

#include "environment.h"
#include "eval.h"

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

shared_ptr<MalEnv> repl_env( new MalEnv(
    nullptr, {
    { "+", make_shared<MalFunction>(mal_plus) },
    { "-", make_shared<MalFunction>(mal_minus) },
    { "*", make_shared<MalFunction>(mal_multiply) },
    { "/", make_shared<MalFunction>(mal_divide) },
    { "DEBUG-EVAL", MalBool(false) },
}));
