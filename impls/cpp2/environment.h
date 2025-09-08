#ifndef _MY_ENVIRONMENT_H_
#define _MY_ENVIRONMENT_H_

#include <string>
#include <memory>
#include <unordered_map>
#include <optional>

#include "types.h"

class MalEnv {
private:
    using Outer = const MalEnv*;
    using Hashmap = std::unordered_map<std::string, MalType>;

public:
    MalEnv(Outer outer = nullptr, Hashmap hashmap = {})
        : m_outer(std::move(outer)), m_hashmap(std::move(hashmap))
    { }

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

extern MalEnv repl_env;

#endif // _MY_ENVIRONMENT_H_
