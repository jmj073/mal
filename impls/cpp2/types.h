#ifndef _TYPES_H_
#define _TYPES_H_

#include <vector>
#include <memory>
#include <utility>
#include <string>

struct MalType {
    MalType() { }
    virtual ~MalType() { }
};

struct MalList: public MalType {
private:
    using T = std::vector<std::unique_ptr<MalType>>;

public:
    MalList()
        : MalType(), data()
    { }
    MalList(T&& d)
        : MalType(), data(::std::move(d))
    { }
    T data;
};

struct MalAtom: public MalType {
    MalAtom()
        : MalType()
    { }
};

struct MalNumber: public MalAtom {
    MalNumber()
        : MalAtom(), data()
    { }
    MalNumber(int d)
        : MalAtom(), data(d)
    { }
    int data;
};

struct MalSymbol: public MalAtom {
    MalSymbol()
        : MalAtom(), data()
    { }
    MalSymbol(std::string&& d)
        : MalAtom(), data(::std::move(d))
    { }
    MalSymbol(const std::string& d)
        : MalAtom(), data(d)
    { }
    std::string data;
};

#endif // _TYPES_H_
