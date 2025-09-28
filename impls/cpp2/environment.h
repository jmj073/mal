#ifndef _MY_ENVIRONMENT_H_
#define _MY_ENVIRONMENT_H_

#include <string>
#include <memory>
#include <unordered_map>
#include <optional>
#include <cassert>
#include "ranges_extension.h"

#include "types.h"

class MalEnv {
private:
    using Outer = std::shared_ptr<MalEnv>;
    using Hashmap = std::unordered_map<std::string, MalType>;

public:
    MalEnv(Outer outer = nullptr, Hashmap hashmap = {})
        : m_outer(std::move(outer)), m_hashmap(std::move(hashmap))
    { }

    template <typename T, typename U>
    MalEnv(Outer outer, T&& binds, U&& exprs)
        : m_outer(std::move(outer)), m_hashmap()
    {
        auto itk = binds.begin();
        auto itv = exprs.begin();

        while (itk != binds.end()) {
            if (*itk == "&") {
                auto ls = std::make_shared<MalList>();
                ls->data = MalList::T(itv, exprs.end());
                m_hashmap[*++itk] = ls;
                break;
            }
            m_hashmap[*itk++] = *itv++;
        }
    }

    void set(std::string k, MalType v) {
        m_hashmap[std::move(k)] = std::move(v);
    }

    std::optional<MalType> get(const std::string k) const {
        auto it = m_hashmap.find(k);
        if (it != m_hashmap.end())
            return it->second;
        return (m_outer ? m_outer->get(k) : std::nullopt);
    }

private:
    Outer m_outer;
    Hashmap m_hashmap;
};

extern std::shared_ptr<MalEnv> repl_env;

#endif // _MY_ENVIRONMENT_H_
