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

std::string MalTypeToString(const MalType& v) {
    return visit([](auto&& v) -> string {
        using T = decay_t<decltype(v)>;
        if constexpr (is_same_v<T, MalNumber>)
            return "number";
        if constexpr (is_same_v<T, MalSymbol>)
            return "symbol";
        if constexpr (is_same_v<T, MalNil>)
            return "nil";
        if constexpr (is_same_v<T, MalBool>)
            return "bool";
        if constexpr (is_same_v<T, MalString>)
            return "string";
        if constexpr (is_same_v<T, MalKeyword>)
            return "keyword";
        if constexpr (is_same_v<T, shared_ptr<MalList>>)
            return "list";
        if constexpr (is_same_v<T, shared_ptr<MalVector>>)
            return "vector";
        if constexpr (is_same_v<T, shared_ptr<MalHashmap>>)
            return "hashmap";
        if constexpr (is_same_v<T, shared_ptr<MalFunction>>)
            return "function";
        if constexpr (is_same_v<T, shared_ptr<MalAtom>>)
            return "atom";
        return "unknown";
    }, v);
}
