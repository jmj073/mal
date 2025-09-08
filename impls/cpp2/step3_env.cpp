#include <iostream>
#include <string>

#include "readline.h"
#include "reader.h"
#include "printer.h"
#include "eval.h"

using namespace std;

static MalType READ(const string& in);
static MalType EVAL(const MalType& ast, MalEnv& env);
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

static void print_debug_eval_if_activated(const MalType& ast, const MalEnv& env) {
    auto opt = env.get("DEBUG-EVAL");

    if (!opt) return;

    bool active = visit([](auto&& v) {
        using T = decay_t<decltype(v)>;
        if constexpr (is_same_v<T, MalNil>)
            return false;
        if constexpr (is_same_v<T, MalBool>)
            return v.data;
        return true;
    }, *opt);

    if (!active) return;

    cout << "EVAL: " << pr_str(ast, true) << endl;
}

static MalType EVAL(const MalType& ast, MalEnv& env) {
    print_debug_eval_if_activated(ast, env);
    return eval(ast, env);
}

static string PRINT(const MalType& ast) {
    return pr_str(ast, true);
}

static string rep(const string& in) {
    return PRINT(EVAL(READ(in), repl_env));
}
