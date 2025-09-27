#ifndef _MY_UTIL_H_
#define _MY_UTIL_H_

#include <variant>
#include <type_traits>

template <typename F, typename T>
auto echanger(F f, T e) {
    try {
        return f();
    } catch(...) {
        throw e;
    }
}

template <typename T, typename... Ts>
inline constexpr bool is_all_same_v = (std::is_same_v<T, Ts> && ...);

template <typename T>
inline constexpr bool is_shared_ptr_v = false;

template <typename T>
inline constexpr bool is_shared_ptr_v<std::shared_ptr<T>> = true;

#endif // _MY_UTIL_H_
