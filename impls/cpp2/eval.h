#ifndef _MY_EVAL_H_
#define _MY_EVAL_H_

#include <stdexcept>

#include "environment.h"
#include "util.h"
#include "printer.h"

class MalEvalFailed: public std::runtime_error {
public:
    MalEvalFailed(const MalType& ast, const std::string& msg)
        : std::runtime_error(pr_str(ast, true) + ": " + msg)
    { }
};


MalType eval(const MalType& ast, std::shared_ptr<MalEnv> env);


#endif // _MY_EVAL_H_
