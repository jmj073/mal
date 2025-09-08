#ifndef _MY_EVAL_H_
#define _MY_EVAL_H_

#include <stdexcept>

#include "types.h"
#include "environment.h"
#include "util.h"

class MalEvalFailed: public std::runtime_error {
public:
    template <typename ...Types>
    MalEvalFailed(Types&& ...args)
        : std::runtime_error(std::forward<Types>(args)...)
    { }
};

MalType eval(const MalType& ast, MalEnv& env);

template <typename T>
auto arg_transformer(const MalType& arg) {
    return echanger(
        [&]() { return get<T>(arg); },
        MalEvalFailed("invalid argument type")
    );
}

#endif // _MY_EVAL_H_
