#ifndef _READER_H_
#define _READER_H_

#include <utility>

template <typename T>
class Reader {
public:
    Reader(T&& tokenizer)
        : m_tokenizer(std::forward(tokenizer))
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
    T::iterator m_cur;
    T::iterator m_last;
};

template <typename T>
auto make_reader(T&& tokenizer) {
    return Reader<T>(std::forward<T>(tokenizer));
}

#endif // _READER_H_
