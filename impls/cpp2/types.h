#ifndef _TYPES_H_
#define _TYPES_H_

#include <vector>
#include <list>
#include <string>
#include <memory>
#include <utility>
#include <variant>

struct MalList;
struct MalVector;
struct MalNumber;
struct MalSymbol;
struct MalNil;
struct MalBool;
struct MalString;
struct MalKeyword;

using MalType = std::variant<
    MalList,
    MalVector,
    MalNumber,
    MalSymbol,
    MalNil,
    MalBool,
    MalString,
    MalKeyword
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
};

struct MalKeyword{
    std::string data;
};

#endif // _TYPES_H_
