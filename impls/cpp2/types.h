#ifndef _TYPES_H_
#define _TYPES_H_

#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <memory>
#include <utility>
#include <variant>
#include <functional>

struct MalNumber;
struct MalSymbol;
struct MalNil;
struct MalBool;
struct MalString;
struct MalKeyword;
struct MalList;
struct MalVector;
struct MalHashmap;
struct MalFunction;

using MalType = std::variant<
    MalNumber,
    MalSymbol,
    MalNil,
    MalBool,
    MalString,
    MalKeyword,
    std::shared_ptr<MalList>,
    std::shared_ptr<MalVector>,
    std::shared_ptr<MalHashmap>,
    std::shared_ptr<MalFunction>
>;

struct MalList {
    using T = std::list<MalType>;
    T data;
};

struct MalVector {
    using T = std::vector<MalType>;
    T data;
};

struct MalNumber {
    using T = int;
    T data;
};

struct MalSymbol {
    using T = std::string;
    T data;
};

struct MalNil {
};

struct MalBool {
    using T = bool;
    T data;
};

struct MalString {
    using T = std::string;
    T data;
    bool operator==(const MalString&) const = default;
};

struct MalKeyword {
    using T = std::string;
    T data;
    bool operator==(const MalKeyword&) const = default;
};

struct MalHashmap {
    using Key = std::variant<MalString, MalKeyword>;
    struct KeyHash {
        std::size_t operator()(const Key& k) const;
    };
    struct KeyEq {
        bool operator()(const Key& a, const Key& b) const {
            return a == b;
        }
    };
    using T = std::unordered_map<Key, MalType, KeyHash, KeyEq>;
    T data;
};

struct MalFunction {
    std::function<MalType(const std::vector<MalType>&)> data;
};

MalType MalKeyToMalType(const MalHashmap::Key& k);

std::string MalTypeToString(const MalType& v);

#endif // _TYPES_H_
