#ifndef _MY_EVAL_H_
#define _MY_EVAL_H_

#include <stdexcept>

#include "types.h"
#include "environment.h"
#include "util.h"

class MalEvalFailed: public std::runtime_error {
public:
    MalEvalFailed(const char* msg)
        : std::runtime_error(msg)
    { }
};

MalType eval(const MalType& ast, MalEnv& env);

template <typename T>
auto arg_transformer(const MalType& arg) {
    return echanger(
        [&](){ return unwrap_variant<T>(arg); },
        MalEvalFailed("invalid argument type")
    );
}

#endif // _MY_EVAL_H_
