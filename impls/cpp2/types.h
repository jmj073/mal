#ifndef _TYPES_H_
#ifdef _TYPES_H_

#include <vector>
#include <memory>
#include <utility>

class MalType {

    virtual ~MalType() { }
};

class MalList: public MalType, public std::vector<std::unique_ptr<MalType>> {
public:
    template <typename Types>
    MalList(Types&& ... args)
        : MalType(), std::vector<std::unique_ptr<MalType>>(std::forward<Types>(args)...)
    {
    }
};

class MalAtom: public MalType {
};

#endif // _TYPES_H_
