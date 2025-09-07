#include <ranges>

#include "eval.h"
#include "util.h"

using namespace std;

MalType eval(const MalType& ast, MalEnv& env) {
    return visit([&](auto&& v) -> MalType {
        using T = decay_t<decltype(v)>;
        if constexpr (is_same_v<T, MalSymbol>) {
            auto it = env.find(v.data);
            if (it == env.end()) {
                throw MalEvalFailed("not found");
            }
            return it->second;
        }
        if constexpr (is_same_v<T, shared_ptr<MalList>>) {
            if (v->data.empty()) {
                return v;
            }
            auto r = v->data
                | views::transform([&](auto&& expr) { return eval(expr, env); });
            auto it = r.begin();
            auto it_end = r.end();
            auto fn = echanger(
                [&](){ return unwrap_variant<shared_ptr<MalFunction>>(*it); },
                MalEvalFailed("not a function")
            );
            vector<MalType> args(++it, it_end);
            return fn->data(args);
        }
        return v;
    }, ast);
}
