#include <iostream>
#include <string>
#include <numeric>

#include "readline.h"
#include "reader.h"
#include "printer.h"

using namespace std;

class MalSymbolNotFound: public std::runtime_error {
public:
    MalSymbolNotFound()
        : std::runtime_error("not found")
    { }
};

struct UnwrapVariantFailed {
};

class MalEvalFailed: public std::runtime_error {
public:
    MalEvalFailed(const char* msg)
        : std::runtime_error(msg)
    { }
};

template <typename T, typename U>
T unwrap_variant(U&& a) {
    return visit([] (auto&& v) -> T {
        using V = decay_t<decltype(v)>;
        if constexpr (is_same_v<V, T>) {
            return v;
        }
        throw UnwrapVariantFailed();
    }, forward<U>(a));
}

template <typename F, typename T>
auto echanger(F f, T e) {
    try {
        return f();
    } catch(...) {
        throw e;
    }
}

template <typename T>
auto arg_transformer(const MalType& arg) {
    return echanger(
        [&](){ return unwrap_variant<T>(arg); },
        MalEvalFailed("invalid argument type")
    );
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

using ReplEnv = unordered_map<string, MalType>;
static ReplEnv repl_env = {
    { "+", make_shared<MalFunction>(mal_plus) },
    { "-", make_shared<MalFunction>(mal_minus) },
    { "*", make_shared<MalFunction>(mal_multiply) },
    { "/", make_shared<MalFunction>(mal_divide) },
};

static MalType READ(const string& in);
static MalType EVAL(const MalType& ast, ReplEnv& env);
static string PRINT(const MalType& ast);
static string rep(const string& in);

static ReadLine read_line("~/.mal-history");

int main() {
    const string prompt = "user> ";
    string input;

    while (read_line.get(prompt, input)) {
        try {
            cout << rep(input) << endl;
        } catch (MalSyntaxError& e) {
            cerr << e.what() << endl;
        } catch (MalInvalidHashmapKey& e) {
            cerr << e.what() << endl;
        } catch (MalEvalFailed& e) {
            cout << e.what() << endl;
        } catch (MalNoToken& e) {
        }
    }
}

static MalType READ(const string& in) {
    return read_str(in);
}

static MalType EVAL(const MalType& ast, ReplEnv& env) {
    return visit([&](auto&& v) -> MalType {
        using T = decay_t<decltype(v)>;
        if constexpr (is_same_v<T, MalSymbol>) {
            auto it = env.find(v.data);
            if (it == env.end()) {
                throw MalSymbolNotFound();
            }
            return it->second;
        }
        if constexpr (is_same_v<T, shared_ptr<MalList>>) {
            if (v->data.empty()) {
                return v;
            }
            auto r = v->data
                | views::transform([&](auto&& expr) { return EVAL(expr, env); });
            auto it = r.begin();
            auto it_end = r.end();
            auto fn = echanger(
                [&](){ return unwrap_variant<shared_ptr<MalFunction>>(*it); },
                MalEvalFailed("not a function")
            );
            vector<MalType> args(++it, it_end);
            return fn->data(args);
        }
        return v;
    }, ast);
}

static string PRINT(const MalType& ast) {
    return pr_str(ast, true);
}

static string rep(const string& in) {
    return PRINT(EVAL(READ(in), repl_env));
}
