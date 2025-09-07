#ifndef _MY_UTIL_H_
#define _MY_UTIL_H_

#include <variant>

struct UnwrapVariantFailed {
};

template <typename T, typename U>
T unwrap_variant(U&& a) {
    return std::visit([] (auto&& v) -> T {
        using V = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<V, T>) {
            return v;
        }
        throw UnwrapVariantFailed();
    }, std::forward<U>(a));
}

template <typename F, typename T>
auto echanger(F f, T e) {
    try {
        return f();
    } catch(...) {
        throw e;
    }
}

#endif // _MY_UTIL_H_
