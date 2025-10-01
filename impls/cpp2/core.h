#pragma once

#include <unordered_map>
#include <string>

#include "types.h"

class MalRuntimeError: public std::runtime_error {
public:
    template <typename ...Types>
    MalRuntimeError(Types&& ...args)
        : std::runtime_error(std::forward<Types>(args)...)
    { }
};

extern std::unordered_map<std::string, MalFunction> core_fn;
