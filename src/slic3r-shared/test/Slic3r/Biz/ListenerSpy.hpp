#pragma once
#include <tuple>
#include <vector>

// === [1] Template helper ==================================================
template<typename>
struct spy_method;

template<typename... Args>
struct spy_method<void(Args...)> {
    using tuple_type = std::tuple<Args...>;
    static void call(std::vector<tuple_type>& storage, Args... args) {
        storage.emplace_back(args...);
    }
};

// === [2] Macros for parameter name generation ==============================
#define GET_PARAM_LIST_0()
#define GET_PARAM_LIST_1(t1)               t1 arg0
#define GET_PARAM_LIST_2(t1, t2)           t1 arg0, t2 arg1
#define GET_PARAM_LIST_3(t1, t2, t3)       t1 arg0, t2 arg1, t3 arg2
#define GET_PARAM_LIST_4(t1, t2, t3, t4)   t1 arg0, t2 arg1, t3 arg2, t4 arg3

#define GET_ARG_LIST_0()
#define GET_ARG_LIST_1()                   arg0
#define GET_ARG_LIST_2()                   arg0, arg1
#define GET_ARG_LIST_3()                   arg0, arg1, arg2
#define GET_ARG_LIST_4()                   arg0, arg1, arg2, arg3

#define NUM_ARGS_IMPL(_1, _2, _3, _4, N, ...) N
#define NUM_ARGS(...) NUM_ARGS_IMPL(__VA_ARGS__, 4, 3, 2, 1, 0)

#define EXPAND(x) x

#define GET_PARAM_LIST_FROM_TYPES(...) EXPAND(GET_PARAM_LIST_IMPL(NUM_ARGS(__VA_ARGS__), __VA_ARGS__))
#define GET_PARAM_LIST_IMPL(n, ...) EXPAND(GET_PARAM_LIST_##n(__VA_ARGS__))

#define GET_ARG_LIST_FROM_TYPES(...) EXPAND(GET_ARG_LIST_IMPL(NUM_ARGS(__VA_ARGS__)))
#define GET_ARG_LIST_IMPL(n) EXPAND(GET_ARG_LIST_##n())

// === [3] Macros to process the signature tuple =============================
#define EXTRACT_PARAM_TYPES(sig) EXTRACT_PARAM_TYPES_ sig
#define EXTRACT_PARAM_TYPES_(ret, ...) __VA_ARGS__

#define TUPLE_TO_FUNC(sig) TUPLE_TO_FUNC_ sig
#define TUPLE_TO_FUNC_(ret, ...) ret(__VA_ARGS__)

// === [4] The DEFINE_SPY_METHOD Macro ======================================
#define DEFINE_SPY_METHOD(method, signature)                                  \
    using method##_args_vec_t = std::vector<typename spy_method<TUPLE_TO_FUNC(signature)>::tuple_type>;  \
    method##_args_vec_t m_##method##_args_vec;                                  \
    virtual void method( EXPAND(GET_PARAM_LIST_FROM_TYPES(EXPAND(EXTRACT_PARAM_TYPES(signature))) ) ) override { \
         spy_method<TUPLE_TO_FUNC(signature)>::call(                          \
             m_##method##_args_vec,                                           \
             EXPAND(GET_ARG_LIST_FROM_TYPES(EXPAND(EXTRACT_PARAM_TYPES(signature))) ) \
         );                                                                   \
    }


// === [5] Example interface and mock implementation ========================
struct IFoo {
    virtual void foo(int, double) = 0;
    virtual ~IFoo() = default;
};

struct MockFoo : public IFoo {
    // Here the signature is provided as a tuple: (void, int, double)
    DEFINE_SPY_METHOD(foo, (void, int, double))
};

