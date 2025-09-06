#include <cstdint>

#include "types.h"

using namespace std;

// static const string KEYWORD_PREFIX = u8"\u029E";
static const string KEYWORD_PREFIX = "\u029E";

size_t MalHashmap::KeyHash::operator()(const Key& k) const {
    return visit([](auto&& v) -> size_t {
        using T = decay_t<decltype(v)>;
        if constexpr (is_same_v<T, MalString>)  return hash<string>{}(v.data);
        if constexpr (is_same_v<T, MalKeyword>) return hash<string>{}(KEYWORD_PREFIX + v.data);
    }, k);
}

MalType MalKeyToMalType(const MalHashmap::Key& k) {
    return visit([](auto&& v) -> MalType { return v; }, k);
}
