#include <iostream>
#include <string>

#include "readline.h"
#include "reader.h"
#include "printer.h"
#include "eval.h"
#include "core.h"

using namespace std;

static MalType READ(const string& in);
static MalType EVAL(const MalType& ast, shared_ptr<MalEnv> env);
static string PRINT(const MalType& ast);
static string rep(const string& in);

static ReadLine read_line("~/.mal-history");

static void add_core_fn(MalEnv& env) {
    for (auto& [k, v]: core_fn) {
        env.set(k, make_shared<MalFunction>(v));
    }
    rep("(def! not (fn* (a) (if a false true)))");
    rep("(def! compose (fn* (f g) (fn* (x) (g (f x)))))");
    rep(
        "(def! load-file                                           "
        "   (let* (do-str (fn* (s) (str \"(do \" s \" nil)\"))     "
        "          eval-from-string (compose read-string eval))    "
        "       (compose (compose slurp do-str) eval-from-string)))"
    );
}

int main() {
    add_core_fn(*repl_env);

    const string prompt = "user> ";
    string input;

    while (read_line.get(prompt, input)) {
        try {
            cout << rep(input) << endl;
        } catch (MalSyntaxError& e) {
            cerr << e.what() << endl;
        } catch (MalEvalFailed& e) {
            cout << e.what() << endl;
        } catch (MalRuntimeError& e) {
            cout << e.what() << endl;
        } catch (MalNoToken& e) {
        }
    }
}

static MalType READ(const string& in) {
    return read_str(in);
}

static MalType EVAL(const MalType& ast, shared_ptr<MalEnv> env) {
    return eval(ast, env);
}

static string PRINT(const MalType& ast) {
    return pr_str(ast, true);
}

static string rep(const string& in) {
    return PRINT(EVAL(READ(in), repl_env));
}
