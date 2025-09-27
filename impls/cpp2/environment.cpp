#include "environment.h"
#include "eval.h"

using namespace std;

shared_ptr<MalEnv> repl_env( new MalEnv(
    nullptr, {
    { "DEBUG-EVAL", MalBool(false) },
}));
