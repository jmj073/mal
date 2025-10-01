#include "environment.h"

using namespace std;

shared_ptr<MalEnv> repl_env( new MalEnv(
    nullptr, {
    { "DEBUG-EVAL", MalBool(false) },
}));
