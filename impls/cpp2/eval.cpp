#include <ranges>
#include <map>
#include <cassert>
#include <iostream>

#include "eval.h"
#include "util.h"
#include "printer.h"

using namespace std;
using namespace ranges;

static void print_debug_eval_if_activated(const MalType& ast, const MalEnv& env) {
    auto opt = env.get("DEBUG-EVAL");

    if (!opt) return;

    bool active = visit([](auto&& v) {
        using T = decay_t<decltype(v)>;
        if constexpr (is_same_v<T, MalNil>)
            return false;
        if constexpr (is_same_v<T, MalBool>)
            return v.data;
        return true;
    }, *opt);

    if (!active) return;

    cout << "EVAL: " << pr_str(ast, true) << endl;
}

static MalType def_if_valid(const MalType& k, const MalType& v, shared_ptr<MalEnv> env) {
    auto var_name = echanger(
        [&]() { return get<MalSymbol>(k); },
        MalEvalFailed("not a symbol")
    ).data;
    auto value = eval(v, env);

    env->set(std::move(var_name), value);

    return value;
}

template <typename T>
static void let_def(T&& cont, shared_ptr<MalEnv> env) {
    if (cont.size() % 2) {
        throw MalEvalFailed("invalid let* def count");
    }
    for (auto&& chunk: cont | views::chunk(2)) {
        def_if_valid(*chunk.begin(), *++chunk.begin(), env);
    }
}

static MalType core_form_def(const MalList& ls, shared_ptr<MalEnv> env) {
    assert(ls.data.size() == 3);
    auto it = ls.data.begin();
    assert(get<MalSymbol>(*it++).data == "def!");

    auto var_name_it = it++;
    auto expr_it = it;

    return def_if_valid(*var_name_it, *expr_it, env);
}

static MalType core_form_let(const MalList& ls, shared_ptr<MalEnv> env) {
    assert(ls.data.size() >= 2);
    auto it = ls.data.begin();
    assert(get<MalSymbol>(*it++).data == "let*");

    auto let_env = make_shared<MalEnv>(env);

    visit([&](auto&& cont) {
        using T = decay_t<decltype(cont)>;
        if constexpr (is_same_v<T, shared_ptr<MalList>>) {
            let_def(cont->data, let_env);
            return;
        }
        if constexpr (is_same_v<T, shared_ptr<MalVector>>) {
            let_def(cont->data, let_env);
            return;
        }
        throw MalEvalFailed("invalid let* form");
    }, *it++);

    MalType ret = MalNil();

    while (it != ls.data.end()) {
        ret = eval(*it++, let_env);
    }

    return ret;
}

static MalType core_form_do(const MalList& ls, shared_ptr<MalEnv> env) {
    assert(ls.data.size() > 1);
    auto it = ls.data.begin();
    assert(get<MalSymbol>(*it++).data == "do");

    MalType ret = MalNil();

    while (it != ls.data.end()) {
        ret = eval(*it++, env);
    }

    return ret;
}

static MalType core_form_if(const MalList& ls, shared_ptr<MalEnv> env) {
    assert(ls.data.size() == 3 || ls.data.size() == 4);
    auto it = ls.data.begin();
    assert(get<MalSymbol>(*it++).data == "if");

    auto cond_tmp = eval(*it++, env);
    bool cond = visit([](auto&& v) {
        using T = decay_t<decltype(v)>;
        if constexpr (is_same_v<T, MalNil>)
            return false;
        if constexpr (is_same_v<T, MalBool>)
            return v.data;
        return true;
    }, cond_tmp);

    if (cond) return eval(*it, env);
    return (ls.data.size() == 4 ? eval(*++it, env) : MalNil());
}

static MalType core_form_fn(const MalList& ls, shared_ptr<MalEnv> env) {
    assert(ls.data.size() >= 2);
    auto it = ls.data.begin();
    assert(get<MalSymbol>(*it++).data == "fn*");

    auto param_list = visit([](auto&& params) -> list<string> {
        using T = decay_t<decltype(params)>;
        if constexpr (is_same_v<T, shared_ptr<MalList>>
                || is_same_v<T, shared_ptr<MalVector>>) {
            list<string> param_list;

            for (auto& arg_name: params->data) {
                param_list.push_back(get<MalSymbol>(arg_name).data);
            }

            return param_list;
        }
        throw MalEvalFailed("invalid let* form");
    }, *it++);

    return make_shared<MalFunction>(
        [=](const vector<MalType>& args) {
            auto fn_env = make_shared<MalEnv>(env, param_list, args);
            auto it = ls.data.begin();
            std::advance(it, 2);
            MalType ret = MalNil();

            while (it != ls.data.end()) {
                ret = eval(*it++, fn_env);
            }

            return ret;
        }
    );
}

static map<string, function<MalType(const MalList&, shared_ptr<MalEnv>)>> core_form = {
    { "def!", core_form_def },
    { "let*", core_form_let },
    { "do", core_form_do },
    { "if", core_form_if },
    { "fn*", core_form_fn },
};

static MalType apply(const MalList& ls) {
    auto it = ls.data.begin();
    auto it_end = ls.data.end();
    auto fn = echanger(
        [&]() { return get<shared_ptr<MalFunction>>(*it); },
        MalEvalFailed("not a function")
    );
    vector<MalType> args(++it, it_end);
    return fn->data(args);
}

static MalType eval_symbol(const MalSymbol& sym, shared_ptr<MalEnv> env) {
    auto opt = env->get(sym.data);
    if (!opt) {
        throw MalEvalFailed("'" + sym.data + "' not found");
    }
    return *opt;
}

static MalType eval_list(const shared_ptr<MalList>& ls, shared_ptr<MalEnv> env) {
    if (ls->data.empty()) {
        return ls;
    }

    if (holds_alternative<MalSymbol>(ls->data.front())) {
        const string& s = get<MalSymbol>(ls->data.front()).data;
        auto it = core_form.find(s);
        if (it != core_form.end())
            return it->second(*ls, env);
    }

    auto r = ls->data
        | views::transform([&](auto&& expr) { return eval(expr, env); });
    auto ret = MalList();
    ret.data = MalList::T(r.begin(), r.end());
    return apply(ret);
}

static MalType eval_vector(const shared_ptr<MalVector>& vec, shared_ptr<MalEnv> env) {
    auto r = vec->data
        | views::transform([&](auto&& expr) { return eval(expr, env); });
    auto ret = make_shared<MalVector>();
    ret->data = MalVector::T(r.begin(), r.end());
    return ret;
}

static MalType eval_hashmap(const shared_ptr<MalHashmap>& hm, shared_ptr<MalEnv> env) {
    auto r = hm->data
        | views::transform([&](auto&& kv) {
            return make_pair(kv.first, eval(kv.second, env));
        });
    auto ret = make_shared<MalHashmap>();
    ret->data = MalHashmap::T(r.begin(), r.end());
    return ret;
}

MalType eval(const MalType& ast, shared_ptr<MalEnv> env) {
    print_debug_eval_if_activated(ast, *env);

    return visit([&](auto&& v) -> MalType {
        using T = decay_t<decltype(v)>;
        if constexpr (is_same_v<T, MalSymbol>) {
            return eval_symbol(v, env);
        }
        if constexpr (is_same_v<T, shared_ptr<MalList>>) {
            return eval_list(v, env);
        }
        if constexpr (is_same_v<T, shared_ptr<MalVector>>) {
            return eval_vector(v, env);
        }
        if constexpr (is_same_v<T, shared_ptr<MalHashmap>>) {
            return eval_hashmap(v, env);
        }
        return v;
    }, ast);
}
