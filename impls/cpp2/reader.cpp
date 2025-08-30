#include <regex>
#include <ranges>

#include "reader.h"

using namespace std;
using namespace ranges;

template <typename It>
struct Iterable {
    Iterable(It f, It l)
        : first(f)
        , last(l)
    {
    }

    auto begin() { return first; }
    auto end() { return last; }

    It first, last;
};


class Tokenizer {
public:

public:
    template <typename S, typename R>
    Tokenizer::Tokenizer(S&& str, R&& re)
        : m_str(forward<S>(str))
        , m_token_re(forward<R>(re))
        , m_iterable(
            Iterable(sregex_iterator(str.begin(), str.end(), m_token_re)
                     sregex_iterator())
            | views::transform([] (auto mat) { return (*it)[1].matched ? (*it)[1].str() : (*it).str(); })
            | views::filter([] (auto token) { return !token.empty(); })
            | views::filter([] (auto token) { return token[0] == ';'; })
          )
    {
    }

    

private:
    string m_str;
    regex m_token_re;
    auto m_iterable;
};

int main() {
}
