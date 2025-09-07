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
    std::list<MalType> data;
};

struct MalVector {
    std::vector<MalType> data;
};

struct MalNumber {
    int data;
};

struct MalSymbol {
    std::string data;
};

struct MalNil {
};

struct MalBool {
    bool data;
};

struct MalString {
    std::string data;
    bool operator==(const MalString&) const = default;
};

struct MalKeyword {
    std::string data;
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

#endif // _TYPES_H_
