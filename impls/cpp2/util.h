#ifndef _MY_UTIL_H_
#define _MY_UTIL_H_

#include <variant>

template <typename F, typename T>
auto echanger(F f, T e) {
    try {
        return f();
    } catch(...) {
        throw e;
    }
}

#endif // _MY_UTIL_H_
