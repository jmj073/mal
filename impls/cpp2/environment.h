#ifndef _MY_ENVIRONMENT_H_
#define _MY_ENVIRONMENT_H_

#include <string>
#include <unordered_map>

#include "types.h"

using MalEnv = std::unordered_map<std::string, MalType>;

extern MalEnv repl_env;

#endif // _MY_ENVIRONMENT_H_
