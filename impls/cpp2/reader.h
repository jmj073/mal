#ifndef _READER_H_
#define _READER_H_

#include <ranges>
#include <utility>
#include <memory>
#include <stdexcept>

#include "types.h"

template <typename T>
class Reader {
private:
    using iterator = std::ranges::iterator_t<T>;

public:
    template <typename U>
    Reader(U&& tokenizer)
        : m_tokenizer(std::forward<U>(tokenizer))
        , m_cur(m_tokenizer.begin())
        , m_last(m_tokenizer.end())
    {
    }

    std::string next() {
        if (m_cur == m_last) return "";
        return *m_cur++;
    }
    std::string peek() const {
        if (m_cur == m_last) return "";
        return *m_cur;
    }

private:
    T m_tokenizer;
    iterator m_cur;
    iterator m_last;
};

template <typename T>
auto make_reader(T&& tokenizer) {
    return Reader<T>(std::forward<T>(tokenizer));
}

class MalSyntaxError: public std::runtime_error {
public:
    template <typename ...Types>
    MalSyntaxError(Types&& ...args)
        : std::runtime_error(std::forward<Types>(args)...)
    { }
};

class MalNoToken: public std::runtime_error {
public:
    MalNoToken()
        : std::runtime_error("No token")
    { }
};

class MalInvalidHashmapKey: public std::runtime_error {
public:
    MalInvalidHashmapKey()
        : std::runtime_error("Invalid hashmap key")
    { }
};

MalType read_str(const std::string& str);


#endif // _READER_H_
