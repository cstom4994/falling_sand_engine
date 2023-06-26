
#ifndef ME_PROMISE_HPP
#define ME_PROMISE_HPP

#include "engine/core/core.hpp"

#ifndef METADOT_PROMISE_MULTITHREAD
#define METADOT_PROMISE_MULTITHREAD 1
#endif

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <vector>

#ifdef __cpp_rtti
#include <typeindex>
namespace ME::cpp::promise {
using type_index = std::type_index;

template <typename T>
inline type_index type_id() {
    return typeid(T);
}
}  // namespace ME::cpp::promise
#else
namespace ME::cpp::promise {
using type_index = ptrdiff_t;

template <typename T>
inline type_index type_id() {
    static char idHolder;
    return (type_index)&idHolder;
}
}  // namespace ME::cpp::promise
#endif

namespace std {

#if (defined(_MSVC_LANG) && _MSVC_LANG < 201402L) || (!defined(_MSVC_LANG) && __cplusplus < 201402L)
template <size_t... Ints>
struct index_sequence {
    using type = index_sequence;
    using value_type = size_t;
    static constexpr std::size_t size() noexcept { return sizeof...(Ints); }
};

template <class Sequence1, class Sequence2>
struct _merge_and_renumber;

template <size_t... I1, size_t... I2>
struct _merge_and_renumber<index_sequence<I1...>, index_sequence<I2...>> : index_sequence<I1..., (sizeof...(I1) + I2)...> {};

template <size_t N>
struct make_index_sequence : _merge_and_renumber<typename make_index_sequence<N / 2>::type, typename make_index_sequence<N - N / 2>::type> {};

template <>
struct make_index_sequence<0> : index_sequence<> {};
template <>
struct make_index_sequence<1> : index_sequence<0> {};
#endif  //__cplusplus < 201402L

#if (defined(_MSVC_LANG) && _MSVC_LANG < 201703L) || (!defined(_MSVC_LANG) && __cplusplus < 201703L)

template <class...>
using void_t = void;

#endif  //__cplusplus < 201703L

#if (defined(_MSVC_LANG) && _MSVC_LANG < 202002L) || (!defined(_MSVC_LANG) && __cplusplus < 202002L)

template <class T>
struct remove_cvref {
    typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type type;
};

#endif  //__cplusplus < 202002L

}  // namespace std

namespace ME::cpp::promise {

template <typename T>
struct tuple_remove_cvref {
    using type = typename std::remove_cvref<T>::type;
};
template <typename... T>
struct tuple_remove_cvref<std::tuple<T...>> {
    using type = std::tuple<typename std::remove_cvref<T>::type...>;
};

template <typename T, typename = void>
struct is_iterable : std::false_type {};

// this gets used only when we can call std::begin() and std::end() on that type
template <typename T>
struct is_iterable<T, std::void_t<decltype(std::begin(std::declval<T>())), decltype(std::end(std::declval<T>()))>> : std::true_type {};

template <typename T>
struct is_std_function : std::false_type {};
template <typename T>
struct is_std_function<std::function<T>> : std::true_type {};

/* 将任意可执行函数或对象的可调用性质
 *
 *  判断类型T是否可被调用
 *    call_traits<T>::is_callable
 *
 *  将对象t转为std::function
 *    call_traits<decltype(t)>::to_std_function(t)
 *    如果t不可调用，会返回一个空函数
 */

template <typename T, bool is_basic_type = (std::is_void<T>::value || std::is_fundamental<T>::value || (std::is_pointer<T>::value && !std::is_function<typename std::remove_pointer<T>::type>::value) ||
                                            std::is_union<T>::value || std::is_enum<T>::value || std::is_array<T>::value) &&
                                           !std::is_function<T>::value>
struct call_traits_impl;

template <typename T>
struct has_operator_parentheses {
private:
    struct Fallback {
        void operator()();
    };
    struct Derived : T, Fallback {};

    template <typename U, U>
    struct Check;

    template <typename>
    static std::true_type test(...);

    template <typename C>
    static std::false_type test(Check<void (Fallback::*)(), &C::operator()> *);

public:
    typedef decltype(test<Derived>(nullptr)) type;
};

template <typename T, bool has_operator_parentheses>
struct operator_parentheses_traits {
    typedef std::function<void(void)> fun_type;
    typedef void result_type;
    typedef std::tuple<> argument_type;

    static fun_type to_std_function(const T &t) {
        (void)t;
        return nullptr;
    }
};

template <typename FUNCTOR>
struct operator_parentheses_traits<FUNCTOR, true> {
private:
    typedef decltype(&FUNCTOR::operator()) callable_type;
    typedef call_traits_impl<callable_type> the_type;

public:
    typedef typename the_type::fun_type fun_type;
    typedef typename the_type::result_type result_type;
    typedef typename the_type::argument_type argument_type;

    static fun_type to_std_function(const FUNCTOR &functor) {
        if (is_std_function<FUNCTOR>::value) {
            // on windows, FUNCTOR is not same as std::function<...>, must return functor directly.
            return functor;
        } else {
            return call_traits_impl<callable_type>::to_std_function(const_cast<FUNCTOR &>(functor), &FUNCTOR::operator());
        }
    }
};

// template<typename T, bool is_basic_type>
// struct call_traits_impl {
//     typedef call_traits_impl<T, is_fundamental, is_pointer> the_type;
//     static const bool is_callable = the_type::is_callable;
//     typedef typename the_type::fun_type fun_type;

//    static fun_type to_std_function(const T &t) {
//        return the_type::to_std_function(t);
//    }
//};

template <typename T>
struct call_traits_impl<T, true> {
    static const bool is_callable = false;
    typedef std::function<void(void)> fun_type;
    typedef void result_type;
    typedef std::tuple<> argument_type;

    static fun_type to_std_function(const T &t) {
        (void)t;
        return nullptr;
    }
};

template <typename RET, class T, typename... ARG>
struct call_traits_impl<RET (T::*)(ARG...), false> {
    static const bool is_callable = true;
    typedef std::function<RET(ARG...)> fun_type;
    typedef RET result_type;
    typedef std::tuple<ARG...> argument_type;

    static fun_type to_std_function(T &obj, RET (T::*func)(ARG...)) {
        return [obj, func](ARG... arg) -> RET { return (const_cast<typename std::remove_const<T>::type &>(obj).*func)(arg...); };
    }

    // Never called, to make compiler happy
    static fun_type to_std_function(RET (T::*)(ARG...)) { return nullptr; }
};

template <typename RET, class T, typename... ARG>
struct call_traits_impl<RET (T::*)(ARG...) const, false> {
    static const bool is_callable = true;
    typedef std::function<RET(ARG...)> fun_type;
    typedef RET result_type;
    typedef std::tuple<ARG...> argument_type;

    static fun_type to_std_function(T &obj, RET (T::*func)(ARG...) const) {
        return [obj, func](ARG... arg) -> RET { return (obj.*func)(arg...); };
    }

    // Never called, to make compiler happy
    static fun_type to_std_function(RET (T::*)(ARG...)) { return nullptr; }
};

template <typename RET, typename... ARG>
struct call_traits_impl<RET (*)(ARG...), false> {
    static const bool is_callable = true;
    typedef std::function<RET(ARG...)> fun_type;
    typedef RET result_type;
    typedef std::tuple<ARG...> argument_type;

    static fun_type to_std_function(RET (*func)(ARG...)) { return func; }
};

template <typename RET, typename... ARG>
struct call_traits_impl<RET(ARG...), false> {
    static const bool is_callable = true;
    typedef std::function<RET(ARG...)> fun_type;
    typedef RET result_type;
    typedef std::tuple<ARG...> argument_type;

    static fun_type to_std_function(RET (*func)(ARG...)) { return func; }
};

template <typename T>
struct call_traits_impl<T, false> {
    static const bool is_callable = has_operator_parentheses<T>::type::value;

private:
    typedef operator_parentheses_traits<T, is_callable> the_type;

public:
    typedef typename the_type::fun_type fun_type;
    typedef typename the_type::result_type result_type;
    typedef typename the_type::argument_type argument_type;

    static fun_type to_std_function(const T &t) { return the_type::to_std_function(t); }
};

template <typename T>
struct call_traits {
private:
    using RawT = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
    typedef call_traits_impl<RawT> the_type;

public:
    static const bool is_callable = the_type::is_callable;
    typedef typename the_type::fun_type fun_type;
    typedef typename the_type::result_type result_type;
    typedef typename the_type::argument_type argument_type;

    static fun_type to_std_function(const T &t) { return the_type::to_std_function(t); }
};

#if 0  // Not used currently

template <typename ...P>
struct subst_gather {};

template <bool match, size_t lastN, template <typename ...> class Obj, typename T, typename ...P>
struct subst_matcher;

template <size_t lastN, template <typename ...> class Obj, typename ...P1, typename T, typename ...P2>
struct subst_matcher<false, lastN, Obj, subst_gather<P1...>, T, P2...>
{
    using type = typename subst_matcher<
        (lastN >= std::tuple_size<std::tuple<P2...>>::value),
        lastN, Obj, subst_gather<P1..., T>, P2...>::type;
};

template <size_t lastN, template <typename ...> class Obj, typename ...P1, typename ...P2>
struct subst_matcher<true, lastN, Obj, subst_gather<P1...>, P2...>
{
    using type = Obj<P1...>;
};

template <size_t lastN, template <typename ...> class Obj, typename ...P>
struct subst_all_but_last {
    using type = typename subst_matcher<
        (lastN >= std::tuple_size<std::tuple<P...>>::value),
        lastN, Obj, subst_gather<>, P...>::type;
};


template <class... ARGS>
struct call_with_more_args_helper {
    template<typename FUNC, typename ...MORE>
    static auto call(FUNC &&func, ARGS&&...args, MORE&&...) -> typename call_traits<FUNC>::result_type {
        return func(args...);
    }
};


template<typename FUNC, typename ...ARGS>
auto call_with_more_args(FUNC &&func, ARGS&&...args) -> typename call_traits<FUNC>::result_type {
    using Type = typename subst_all_but_last<
        (std::tuple_size<std::tuple<ARGS...>>::value - std::tuple_size<typename call_traits<FUNC>::argument_type>::value),
        call_with_more_args_helper, ARGS...>::type;
    return Type::call(std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

#endif

// Any library
// See http://www.boost.org/libs/any for Documentation.
// what:  variant type any
// who:   contributed by Kevlin Henney,
//        with features contributed and bugs found by
//        Ed Brey, Mark Rodgers, Peter Dimov, and James Curran
// when:  July 2001
// where: tested with BCC 5.5, MSVC 6.0, and g++ 2.95

class any;
template <typename ValueType>
inline ValueType any_cast(const any &operand);

class any {
public:  // structors
    any() : content(0) {}

    template <typename ValueType>
    any(const ValueType &value) : content(new holder<typename std::remove_cvref<ValueType>::type>(value)) {}

    template <typename RET, typename... ARG>
    any(RET value(ARG...)) : content(new holder<RET (*)(ARG...)>(value)) {}

    any(const any &other) : content(other.content ? other.content->clone() : 0) {}

    // Move constructor
    any(any &&other) : content(other.content) { other.content = 0; }

    // Perfect forwarding of ValueType
    template <typename ValueType>
    any(ValueType &&value, typename std::enable_if<!std::is_same<any &, ValueType>::value>::type * = nullptr  // disable if value has type `any&`
        ,
        typename std::enable_if<!std::is_const<ValueType>::value>::type * = nullptr)  // disable if value has type `const ValueType&&`
        : content(new holder<typename std::remove_cvref<ValueType>::type>(static_cast<ValueType &&>(value))) {}

    ~any() {
        if (content != nullptr) {
            delete (content);
        }
    }

    any call(const any &arg) const { return content ? content->call(arg) : any(); }

    template <typename ValueType, typename std::enable_if<!std::is_pointer<ValueType>::value>::type *dummy = nullptr>
    inline ValueType cast() const {
        return any_cast<ValueType>(*this);
    }

    template <typename ValueType, typename std::enable_if<std::is_pointer<ValueType>::value>::type *dummy = nullptr>
    inline ValueType cast() const {
        if (this->empty())
            return nullptr;
        else
            return any_cast<ValueType>(*this);
    }

public:  // modifiers
    any &swap(any &rhs) {
        std::swap(content, rhs.content);
        return *this;
    }

    template <typename ValueType>
    any &operator=(const ValueType &rhs) {
        any(rhs).swap(*this);
        return *this;
    }

    any &operator=(const any &rhs) {
        any(rhs).swap(*this);
        return *this;
    }

public:  // queries
    bool empty() const { return !content; }

    void clear() { any().swap(*this); }

    type_index type() const { return content ? content->type() : type_id<void>(); }

public:  // types (public so any_cast can be non-friend)
    class placeholder {
    public:  // structors
        virtual ~placeholder() {}

    public:  // queries
        virtual type_index type() const = 0;
        virtual placeholder *clone() const = 0;
        virtual any call(const any &arg) const = 0;
    };

    template <typename ValueType>
    class holder : public placeholder {
    public:  // structors
        holder(const ValueType &value) : held(value) {}

    public:  // queries
        virtual type_index type() const { return type_id<ValueType>(); }

        virtual placeholder *clone() const { return new holder(held); }

        virtual any call(const any &arg) const { return any_call(held, arg); }

    public:  // representation
        ValueType held;

    private:  // intentionally left unimplemented
        holder &operator=(const holder &);
    };

public:  // representation (public so any_cast can be non-friend)
    placeholder *content;
};

class bad_any_cast : public std::bad_cast {
public:
    type_index from_;
    type_index to_;
    bad_any_cast(const type_index &from, const type_index &to) : from_(from), to_(to) {
        // fprintf(stderr, "bad_any_cast: from = %s, to = %s\n", from.name(), to.name());
    }
    virtual const char *what() const throw() { return "bad_any_cast"; }
};

template <typename ValueType>
ValueType *any_cast(any *operand) {
    typedef typename any::template holder<ValueType> holder_t;
    return operand && operand->type() == type_id<ValueType>() ? &static_cast<holder_t *>(operand->content)->held : 0;
}

template <typename ValueType>
inline const ValueType *any_cast(const any *operand) {
    return any_cast<ValueType>(const_cast<any *>(operand));
}

template <typename ValueType>
ValueType any_cast(any &operand) {
    typedef typename std::remove_cvref<ValueType>::type nonref;

    nonref *result = any_cast<nonref>(&operand);
    if (!result) throw bad_any_cast(operand.type(), type_id<ValueType>());
    return *result;
}

template <typename ValueType>
inline ValueType any_cast(const any &operand) {
    typedef typename std::remove_cvref<ValueType>::type nonref;
    return any_cast<nonref &>(const_cast<any &>(operand));
}

template <typename RET, typename NOCVR_ARGS, typename FUNC>
struct any_call_t;

template <typename RET, typename... NOCVR_ARGS, typename FUNC>
struct any_call_t<RET, std::tuple<NOCVR_ARGS...>, FUNC> {

    static inline RET call(const typename FUNC::fun_type &func, const any &arg) { return call(func, arg, std::make_index_sequence<sizeof...(NOCVR_ARGS)>()); }

    template <size_t... I>
    static inline RET call(const typename FUNC::fun_type &func, const any &arg, const std::index_sequence<I...> &) {
        using nocvr_argument_type = std::tuple<NOCVR_ARGS...>;
        using any_arguemnt_type = std::vector<any>;

        const any_arguemnt_type &args = (arg.type() != type_id<any_arguemnt_type>() ? any_arguemnt_type{arg} : any_cast<any_arguemnt_type &>(arg));
        if (args.size() < sizeof...(NOCVR_ARGS)) throw bad_any_cast(arg.type(), type_id<nocvr_argument_type>());

        return func(any_cast<typename std::tuple_element<I, nocvr_argument_type>::type &>(args[I])...);
    }
};

template <typename RET, typename NOCVR_ARG, typename FUNC>
struct any_call_t<RET, std::tuple<NOCVR_ARG>, FUNC> {

    static inline RET call(const typename FUNC::fun_type &func, const any &arg) {
        using nocvr_argument_type = std::tuple<NOCVR_ARG>;
        using any_arguemnt_type = std::vector<any>;

        if (arg.type() == type_id<std::exception_ptr>()) {
            try {
                std::rethrow_exception(any_cast<std::exception_ptr>(arg));
            } catch (const NOCVR_ARG &ex_arg) {
                return func(const_cast<NOCVR_ARG &>(ex_arg));
            }
        }

        if (type_id<NOCVR_ARG>() == type_id<any_arguemnt_type>()) {
            return func(any_cast<NOCVR_ARG &>(arg));
        }

        const any_arguemnt_type &args = (arg.type() != type_id<any_arguemnt_type>() ? any_arguemnt_type{arg} : any_cast<any_arguemnt_type &>(arg));
        if (args.size() < 1) throw bad_any_cast(arg.type(), type_id<nocvr_argument_type>());
        // printf("[%s] [%s]\n", args.front().type().name(), type_id<NOCVR_ARG>().name());
        return func(any_cast<NOCVR_ARG &>(args.front()));
    }
};

template <typename RET, typename FUNC>
struct any_call_t<RET, std::tuple<any>, FUNC> {
    static inline RET call(const typename FUNC::fun_type &func, const any &arg) {
        using any_arguemnt_type = std::vector<any>;
        if (arg.type() != type_id<any_arguemnt_type>()) return (func(const_cast<any &>(arg)));

        any_arguemnt_type &args = any_cast<any_arguemnt_type &>(arg);
        if (args.size() == 0) {
            any empty;
            return (func(empty));
        } else if (args.size() == 1)
            return (func(args.front()));
        else
            return (func(const_cast<any &>(arg)));
    }
};

template <typename RET, typename NOCVR_ARGS, typename FUNC>
struct any_call_with_ret_t {
    static inline any call(const typename FUNC::fun_type &func, const any &arg) { return any_call_t<RET, NOCVR_ARGS, FUNC>::call(func, arg); }
};

template <typename NOCVR_ARGS, typename FUNC>
struct any_call_with_ret_t<void, NOCVR_ARGS, FUNC> {
    static inline any call(const typename FUNC::fun_type &func, const any &arg) {
        any_call_t<void, NOCVR_ARGS, FUNC>::call(func, arg);
        return any();
    }
};

template <typename FUNC>
inline any any_call(const FUNC &func, const any &arg) {
    using func_t = call_traits<FUNC>;
    using nocvr_argument_type = typename tuple_remove_cvref<typename func_t::argument_type>::type;
    const auto &stdFunc = func_t::to_std_function(func);
    if (!stdFunc) return any();

    if (arg.type() == type_id<std::exception_ptr>()) {
        try {
            std::rethrow_exception(any_cast<std::exception_ptr>(arg));
        } catch (const any &ex_arg) {
            return any_call_with_ret_t<typename call_traits<FUNC>::result_type, nocvr_argument_type, func_t>::call(stdFunc, ex_arg);
        } catch (...) {
        }
    }

    return any_call_with_ret_t<typename call_traits<FUNC>::result_type, nocvr_argument_type, func_t>::call(stdFunc, arg);
}

using pm_any = any;

// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

enum class TaskState { kPending, kResolved, kRejected };

struct PromiseHolder;
struct SharedPromise;
class Promise;

struct Task {
    TaskState state_;
    std::weak_ptr<PromiseHolder> promiseHolder_;
    any onResolved_;
    any onRejected_;
};

#if METADOT_PROMISE_MULTITHREAD
struct Mutex {
public:
    ME_INLINE Mutex();
    ME_INLINE void lock();
    ME_INLINE void unlock();
    ME_INLINE void lock(size_t lock_count);
    ME_INLINE void unlock(size_t lock_count);
    inline size_t lock_count() const { return lock_count_; }
    std::condition_variable_any cond_;

private:
    std::recursive_mutex mutex_;
    size_t lock_count_;
};
#endif

/*
 * Task state in TaskList always be kPending
 */
struct PromiseHolder {
    ME_INLINE PromiseHolder();
    ME_INLINE ~PromiseHolder();
    std::list<std::weak_ptr<SharedPromise>> owners_;
    std::list<std::shared_ptr<Task>> pendingTasks_;
    TaskState state_;
    any value_;
#if METADOT_PROMISE_MULTITHREAD
    std::shared_ptr<Mutex> mutex_;
#endif

    ME_INLINE void dump() const;
    ME_INLINE static any *getUncaughtExceptionHandler();
    ME_INLINE static any *getDefaultUncaughtExceptionHandler();
    ME_INLINE static void onUncaughtException(const any &arg);
    ME_INLINE static void handleUncaughtException(const any &onUncaughtException);
};

// Check if ...ARGS only has one any type
template <typename... ARGS>
struct is_one_any : public std::is_same<typename tuple_remove_cvref<std::tuple<ARGS...>>::type, std::tuple<any>> {};

struct SharedPromise {
    std::shared_ptr<PromiseHolder> promiseHolder_;
    ME_INLINE void dump() const;
#if METADOT_PROMISE_MULTITHREAD
    ME_INLINE std::shared_ptr<Mutex> obtainLock() const;
#endif
};

class Defer {
public:
    template <typename... ARGS, typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void resolve(ARGS &&...args) const {
        resolve(any{std::vector<any>{std::forward<ARGS>(args)...}});
    }

    template <typename... ARGS, typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void reject(ARGS &&...args) const {
        reject(any{std::vector<any>{std::forward<ARGS>(args)...}});
    }

    ME_INLINE void resolve(const any &arg) const;
    ME_INLINE void reject(const any &arg) const;

    ME_INLINE Promise getPromise() const;

private:
    friend class Promise;
    friend ME_INLINE Promise newPromise(const std::function<void(Defer &defer)> &run);
    ME_INLINE Defer(const std::shared_ptr<Task> &task);
    std::shared_ptr<Task> task_;
    std::shared_ptr<SharedPromise> sharedPromise_;
};

class DeferLoop {
public:
    template <typename... ARGS, typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void doBreak(ARGS &&...args) const {
        doBreak(any{std::vector<any>{std::forward<ARGS>(args)...}});
    }

    template <typename... ARGS, typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void reject(ARGS &&...args) const {
        reject(any{std::vector<any>{std::forward<ARGS>(args)...}});
    }

    ME_INLINE void doContinue() const;
    ME_INLINE void doBreak(const any &arg) const;
    ME_INLINE void reject(const any &arg) const;

    ME_INLINE Promise getPromise() const;

private:
    friend ME_INLINE Promise doWhile(const std::function<void(DeferLoop &loop)> &run);
    ME_INLINE DeferLoop(const Defer &cb);
    Defer defer_;
};

class Promise {
public:
    ME_INLINE Promise &then(const any &deferOrPromiseOrOnResolved);
    ME_INLINE Promise &then(const any &onResolved, const any &onRejected);
    ME_INLINE Promise &fail(const any &onRejected);
    ME_INLINE Promise &always(const any &onAlways);
    ME_INLINE Promise &finally(const any &onFinally);

    template <typename... ARGS, typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void resolve(ARGS &&...args) const {
        resolve(any{std::vector<any>{std::forward<ARGS>(args)...}});
    }
    template <typename... ARGS, typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void reject(ARGS &&...args) const {
        reject(any{std::vector<any>{std::forward<ARGS>(args)...}});
    }

    ME_INLINE void resolve(const any &arg) const;
    ME_INLINE void reject(const any &arg) const;

    ME_INLINE void clear();
    ME_INLINE operator bool() const;

    ME_INLINE void dump() const;

    std::shared_ptr<SharedPromise> sharedPromise_;
};

ME_INLINE Promise newPromise(const std::function<void(Defer &defer)> &run);
ME_INLINE Promise newPromise();
ME_INLINE Promise doWhile(const std::function<void(DeferLoop &loop)> &run);
template <typename... ARGS>
inline Promise resolve(ARGS &&...args) {
    return newPromise([&args...](Defer &defer) { defer.resolve(std::forward<ARGS>(args)...); });
}

template <typename... ARGS>
inline Promise reject(ARGS &&...args) {
    return newPromise([&args...](Defer &defer) { defer.reject(std::forward<ARGS>(args)...); });
}

/* Returns a promise that resolves when all of the promises in the iterable
   argument have resolved, or rejects with the reason of the first passed
   promise that rejects. */
ME_INLINE Promise all(const std::list<Promise> &promise_list);
template <typename METADOT_PROMISE_LIST, typename std::enable_if<is_iterable<METADOT_PROMISE_LIST>::value && !std::is_same<METADOT_PROMISE_LIST, std::list<Promise>>::value>::type *dummy = nullptr>
inline Promise all(const METADOT_PROMISE_LIST &promise_list) {
    std::list<Promise> copy_list = {std::begin(promise_list), std::end(promise_list)};
    return all(copy_list);
}
template <typename PROMISE0, typename... METADOT_PROMISE_LIST, typename std::enable_if<!is_iterable<PROMISE0>::value>::type *dummy = nullptr>
inline Promise all(PROMISE0 defer0, METADOT_PROMISE_LIST... promise_list) {
    return all(std::list<Promise>{defer0, promise_list...});
}

/* returns a promise that resolves or rejects as soon as one of
the promises in the iterable resolves or rejects, with the value
or reason from that promise. */
ME_INLINE Promise race(const std::list<Promise> &promise_list);
template <typename METADOT_PROMISE_LIST, typename std::enable_if<is_iterable<METADOT_PROMISE_LIST>::value && !std::is_same<METADOT_PROMISE_LIST, std::list<Promise>>::value>::type *dummy = nullptr>
inline Promise race(const METADOT_PROMISE_LIST &promise_list) {
    std::list<Promise> copy_list = {std::begin(promise_list), std::end(promise_list)};
    return race(copy_list);
}
template <typename PROMISE0, typename... METADOT_PROMISE_LIST, typename std::enable_if<!is_iterable<PROMISE0>::value>::type *dummy = nullptr>
inline Promise race(PROMISE0 defer0, METADOT_PROMISE_LIST... promise_list) {
    return race(std::list<Promise>{defer0, promise_list...});
}

ME_INLINE Promise raceAndReject(const std::list<Promise> &promise_list);
template <typename METADOT_PROMISE_LIST, typename std::enable_if<is_iterable<METADOT_PROMISE_LIST>::value && !std::is_same<METADOT_PROMISE_LIST, std::list<Promise>>::value>::type *dummy = nullptr>
inline Promise raceAndReject(const METADOT_PROMISE_LIST &promise_list) {
    std::list<Promise> copy_list = {std::begin(promise_list), std::end(promise_list)};
    return raceAndReject(copy_list);
}
template <typename PROMISE0, typename... METADOT_PROMISE_LIST, typename std::enable_if<!is_iterable<PROMISE0>::value>::type *dummy = nullptr>
inline Promise raceAndReject(PROMISE0 defer0, METADOT_PROMISE_LIST... promise_list) {
    return raceAndReject(std::list<Promise>{defer0, promise_list...});
}

ME_INLINE Promise raceAndResolve(const std::list<Promise> &promise_list);
template <typename METADOT_PROMISE_LIST, typename std::enable_if<is_iterable<METADOT_PROMISE_LIST>::value && !std::is_same<METADOT_PROMISE_LIST, std::list<Promise>>::value>::type *dummy = nullptr>
inline Promise raceAndResolve(const METADOT_PROMISE_LIST &promise_list) {
    std::list<Promise> copy_list = {std::begin(promise_list), std::end(promise_list)};
    return raceAndResolve(copy_list);
}
template <typename PROMISE0, typename... METADOT_PROMISE_LIST, typename std::enable_if<!is_iterable<PROMISE0>::value>::type *dummy = nullptr>
inline Promise raceAndResolve(PROMISE0 defer0, METADOT_PROMISE_LIST... promise_list) {
    return raceAndResolve(std::list<Promise>{defer0, promise_list...});
}

inline void handleUncaughtException(const any &onUncaughtException) { PromiseHolder::handleUncaughtException(onUncaughtException); }

static inline void healthyCheck(int line, PromiseHolder *promiseHolder) {
    (void)line;
    (void)promiseHolder;
#ifndef NDEBUG
    if (!promiseHolder) {
        fprintf(stderr, "line = %d, %d, promiseHolder is null\n", line, __LINE__);
        throw std::runtime_error("");
    }

    for (const auto &owner_ : promiseHolder->owners_) {
        auto owner = owner_.lock();
        if (owner && owner->promiseHolder_.get() != promiseHolder) {
            fprintf(stderr, "line = %d, %d, owner->promiseHolder_ = %p, promiseHolder = %p\n", line, __LINE__, owner->promiseHolder_.get(), promiseHolder);
            throw std::runtime_error("");
        }
    }

    for (const std::shared_ptr<Task> &task : promiseHolder->pendingTasks_) {
        if (!task) {
            fprintf(stderr, "line = %d, %d, promiseHolder = %p, task is null\n", line, __LINE__, promiseHolder);
            throw std::runtime_error("");
        }
        if (task->state_ != TaskState::kPending) {
            fprintf(stderr, "line = %d, %d, promiseHolder = %p, task = %p, task->state_ = %d\n", line, __LINE__, promiseHolder, task.get(), (int)task->state_);
            throw std::runtime_error("");
        }
        if (task->promiseHolder_.lock().get() != promiseHolder) {
            fprintf(stderr, "line = %d, %d, promiseHolder = %p, task = %p, task->promiseHolder_ = %p\n", line, __LINE__, promiseHolder, task.get(), task->promiseHolder_.lock().get());
            throw std::runtime_error("");
        }
    }
#endif
}

void Promise::dump() const {
#ifndef NDEBUG
    printf("Promise = %p, SharedPromise = %p\n", this, this->sharedPromise_.get());
    if (this->sharedPromise_) this->sharedPromise_->dump();
#endif
}

void SharedPromise::dump() const {
#ifndef NDEBUG
    printf("SharedPromise = %p, PromiseHolder = %p\n", this, this->promiseHolder_.get());
    if (this->promiseHolder_) this->promiseHolder_->dump();
#endif
}

void PromiseHolder::dump() const {
#ifndef NDEBUG
    printf("PromiseHolder = %p, owners = %d, pendingTasks = %d\n", this, (int)this->owners_.size(), (int)this->pendingTasks_.size());
    for (const auto &owner_ : owners_) {
        auto owner = owner_.lock();
        printf("  owner = %p\n", owner.get());
    }
    for (const auto &task : pendingTasks_) {
        if (task) {
            auto promiseHolder = task->promiseHolder_.lock();
            printf("  task = %p, PromiseHolder = %p\n", task.get(), promiseHolder.get());
        } else {
            printf("  task = %p\n", task.get());
        }
    }
#endif
}

static inline void join(const std::shared_ptr<PromiseHolder> &left, const std::shared_ptr<PromiseHolder> &right) {
    healthyCheck(__LINE__, left.get());
    healthyCheck(__LINE__, right.get());
    // left->dump();
    // right->dump();

    for (const std::shared_ptr<Task> &task : right->pendingTasks_) {
        task->promiseHolder_ = left;
    }
    left->pendingTasks_.splice(left->pendingTasks_.end(), right->pendingTasks_);

    std::list<std::weak_ptr<SharedPromise>> owners;
    owners.splice(owners.end(), right->owners_);

    // Looked on resolved if the PromiseHolder was joined to another,
    // so that it will not throw onUncaughtException when destroyed.
    right->state_ = TaskState::kResolved;

    if (owners.size() > 100) {
        fprintf(stderr, "Maybe memory leak, too many promise owners: %d", (int)owners.size());
    }

    for (const std::weak_ptr<SharedPromise> &owner_ : owners) {
        std::shared_ptr<SharedPromise> owner = owner_.lock();
        if (owner) {
            owner->promiseHolder_ = left;
            left->owners_.push_back(owner);
        }
    }

    // left->dump();
    // right->dump();
    // fprintf(stderr, "left->promiseHolder_->owners_ size = %d\n", (int)left->promiseHolder_->owners_.size());

    healthyCheck(__LINE__, left.get());
    healthyCheck(__LINE__, right.get());
}

// Unlock and then lock
#if METADOT_PROMISE_MULTITHREAD
struct unlock_guard_t {
    inline unlock_guard_t(std::shared_ptr<Mutex> mutex) : mutex_(mutex), lock_count_(mutex->lock_count()) { mutex_->unlock(lock_count_); }
    inline ~unlock_guard_t() { mutex_->lock(lock_count_); }
    std::shared_ptr<Mutex> mutex_;
    size_t lock_count_;
};
#endif

static inline void call(std::shared_ptr<Task> task) {
    std::shared_ptr<PromiseHolder> promiseHolder;  // Can hold the temporarily created promise
    while (true) {
        promiseHolder = task->promiseHolder_.lock();
        if (!promiseHolder) return;

        // lock for 1st stage
        {
#if METADOT_PROMISE_MULTITHREAD
            std::shared_ptr<Mutex> mutex = promiseHolder->mutex_;
            std::unique_lock<Mutex> lock(*mutex);
#endif

            if (task->state_ != TaskState::kPending) return;
            if (promiseHolder->state_ == TaskState::kPending) return;

            std::list<std::shared_ptr<Task>> &pendingTasks = promiseHolder->pendingTasks_;
            // promiseHolder->dump();

#if METADOT_PROMISE_MULTITHREAD
            while (pendingTasks.front() != task) {
                mutex->cond_.wait<std::unique_lock<Mutex>>(lock);
            }
#else
            assert(pendingTasks.front() == task);
#endif
            pendingTasks.pop_front();
            task->state_ = promiseHolder->state_;
            // promiseHolder->dump();

            try {
                if (promiseHolder->state_ == TaskState::kResolved) {
                    if (task->onResolved_.empty() || task->onResolved_.type() == type_id<std::nullptr_t>()) {
                        // to next resolved task
                    } else {
                        promiseHolder->state_ = TaskState::kPending;  // avoid recursive task using this state
#if METADOT_PROMISE_MULTITHREAD
                        std::shared_ptr<Mutex> mutex0 = nullptr;
                        auto call = [&]() -> any {
                            unlock_guard_t lock_inner(mutex);
                            const any &value = task->onResolved_.call(promiseHolder->value_);
                            // Make sure the returned promised is locked before than "mutex"
                            if (value.type() == type_id<Promise>()) {
                                Promise &promise = value.cast<Promise &>();
                                mutex0 = promise.sharedPromise_->obtainLock();
                            }
                            return value;
                        };
                        const any &value = call();

                        if (mutex0 == nullptr) {
                            promiseHolder->value_ = value;
                            promiseHolder->state_ = TaskState::kResolved;
                        } else {
                            // join the promise
                            Promise &promise = value.cast<Promise &>();
                            std::lock_guard<Mutex> lock0(*mutex0, std::adopt_lock_t());
                            join(promise.sharedPromise_->promiseHolder_, promiseHolder);
                            promiseHolder = promise.sharedPromise_->promiseHolder_;
                        }
#else
                        const any &value = task->onResolved_.call(promiseHolder->value_);

                        if (value.type() != type_id<Promise>()) {
                            promiseHolder->value_ = value;
                            promiseHolder->state_ = TaskState::kResolved;
                        } else {
                            // join the promise
                            Promise &promise = value.cast<Promise &>();
                            join(promise.sharedPromise_->promiseHolder_, promiseHolder);
                            promiseHolder = promise.sharedPromise_->promiseHolder_;
                        }
#endif
                    }
                } else if (promiseHolder->state_ == TaskState::kRejected) {
                    if (task->onRejected_.empty() || task->onRejected_.type() == type_id<std::nullptr_t>()) {
                        // to next rejected task
                        // promiseHolder->value_ = promiseHolder->value_;
                        // promiseHolder->state_ = TaskState::kRejected;
                    } else {
                        try {
                            promiseHolder->state_ = TaskState::kPending;  // avoid recursive task using this state
#if METADOT_PROMISE_MULTITHREAD
                            std::shared_ptr<Mutex> mutex0 = nullptr;
                            auto call = [&]() -> any {
                                unlock_guard_t lock_inner(mutex);
                                const any &value = task->onRejected_.call(promiseHolder->value_);
                                // Make sure the returned promised is locked before than "mutex"
                                if (value.type() == type_id<Promise>()) {
                                    Promise &promise = value.cast<Promise &>();
                                    mutex0 = promise.sharedPromise_->obtainLock();
                                }
                                return value;
                            };
                            const any &value = call();

                            if (mutex0 == nullptr) {
                                promiseHolder->value_ = value;
                                promiseHolder->state_ = TaskState::kResolved;
                            } else {
                                // join the promise
                                Promise promise = value.cast<Promise>();
                                std::lock_guard<Mutex> lock0(*mutex0, std::adopt_lock_t());
                                join(promise.sharedPromise_->promiseHolder_, promiseHolder);
                                promiseHolder = promise.sharedPromise_->promiseHolder_;
                            }
#else
                            const any &value = task->onRejected_.call(promiseHolder->value_);

                            if (value.type() != type_id<Promise>()) {
                                promiseHolder->value_ = value;
                                promiseHolder->state_ = TaskState::kResolved;
                            } else {
                                // join the promise
                                Promise &promise = value.cast<Promise &>();
                                join(promise.sharedPromise_->promiseHolder_, promiseHolder);
                                promiseHolder = promise.sharedPromise_->promiseHolder_;
                            }
#endif
                        } catch (const bad_any_cast &) {
                            // just go through if argument type is not match
                            promiseHolder->state_ = TaskState::kRejected;
                        }
                    }
                }
            } catch (const ME::cpp::promise::bad_any_cast &ex) {
                fprintf(stderr, "promise::bad_any_cast: %s -> %s", ex.from_.name(), ex.to_.name());
                promiseHolder->value_ = std::current_exception();
                promiseHolder->state_ = TaskState::kRejected;
            } catch (...) {
                promiseHolder->value_ = std::current_exception();
                promiseHolder->state_ = TaskState::kRejected;
            }

            task->onResolved_.clear();
            task->onRejected_.clear();
        }

        // lock for 2nd stage
        // promiseHolder may be changed, so we need to lock again
        {
            // get next task
#if METADOT_PROMISE_MULTITHREAD
            std::shared_ptr<Mutex> mutex = promiseHolder->mutex_;
            std::lock_guard<Mutex> lock(*mutex);
#endif
            std::list<std::shared_ptr<Task>> &pendingTasks2 = promiseHolder->pendingTasks_;
            if (pendingTasks2.size() == 0) {
                return;
            }

            task = pendingTasks2.front();
        }
    }
}

Defer::Defer(const std::shared_ptr<Task> &task) {
    std::shared_ptr<SharedPromise> sharedPromise(new SharedPromise{task->promiseHolder_.lock()});
#if METADOT_PROMISE_MULTITHREAD
    std::shared_ptr<Mutex> mutex = sharedPromise->obtainLock();
    std::lock_guard<Mutex> lock(*mutex, std::adopt_lock_t());
#endif

    task_ = task;
    sharedPromise_ = sharedPromise;
}

void Defer::resolve(const any &arg) const {
#if METADOT_PROMISE_MULTITHREAD
    std::shared_ptr<Mutex> mutex = this->sharedPromise_->obtainLock();
    std::lock_guard<Mutex> lock(*mutex, std::adopt_lock_t());
#endif

    if (task_->state_ != TaskState::kPending) return;
    std::shared_ptr<PromiseHolder> &promiseHolder = sharedPromise_->promiseHolder_;
    promiseHolder->state_ = TaskState::kResolved;
    promiseHolder->value_ = arg;
    call(task_);
}

void Defer::reject(const any &arg) const {
#if METADOT_PROMISE_MULTITHREAD
    std::shared_ptr<Mutex> mutex = this->sharedPromise_->obtainLock();
    std::lock_guard<Mutex> lock(*mutex, std::adopt_lock_t());
#endif

    if (task_->state_ != TaskState::kPending) return;
    std::shared_ptr<PromiseHolder> &promiseHolder = sharedPromise_->promiseHolder_;
    promiseHolder->state_ = TaskState::kRejected;
    promiseHolder->value_ = arg;
    call(task_);
}

Promise Defer::getPromise() const { return Promise{sharedPromise_}; }

struct DoBreakTag {};

DeferLoop::DeferLoop(const Defer &defer) : defer_(defer) {}

void DeferLoop::doContinue() const { defer_.resolve(); }

void DeferLoop::doBreak(const any &arg) const { defer_.reject(DoBreakTag(), arg); }

void DeferLoop::reject(const any &arg) const { defer_.reject(arg); }

Promise DeferLoop::getPromise() const { return defer_.getPromise(); }

#if METADOT_PROMISE_MULTITHREAD
Mutex::Mutex() : cond_(), mutex_(), lock_count_(0) {}

void Mutex::lock() {
    mutex_.lock();
    ++lock_count_;
}

void Mutex::unlock() {
    --lock_count_;
    mutex_.unlock();
}

void Mutex::lock(size_t lock_count) {
    for (size_t i = 0; i < lock_count; ++i) this->lock();
}

void Mutex::unlock(size_t lock_count) {
    for (size_t i = 0; i < lock_count; ++i) this->unlock();
}
#endif

PromiseHolder::PromiseHolder()
    : owners_(),
      pendingTasks_(),
      state_(TaskState::kPending),
      value_()
#if METADOT_PROMISE_MULTITHREAD
      ,
      mutex_(ME::create_ref<Mutex>())
#endif
{
}

PromiseHolder::~PromiseHolder() {
    if (this->state_ == TaskState::kRejected) {
        static thread_local std::atomic<bool> s_inUncaughtExceptionHandler{false};
        if (s_inUncaughtExceptionHandler) return;
        s_inUncaughtExceptionHandler = true;
        struct Releaser {
            Releaser(std::atomic<bool> *inUncaughtExceptionHandler) : inUncaughtExceptionHandler_(inUncaughtExceptionHandler) {}
            ~Releaser() { *inUncaughtExceptionHandler_ = false; }
            std::atomic<bool> *inUncaughtExceptionHandler_;
        } releaser(&s_inUncaughtExceptionHandler);

        PromiseHolder::onUncaughtException(this->value_);
    }
}

any *PromiseHolder::getUncaughtExceptionHandler() {
    static any onUncaughtException;
    return &onUncaughtException;
}

any *PromiseHolder::getDefaultUncaughtExceptionHandler() {
    static any defaultUncaughtExceptionHandler = [](Promise &d) {
        d.fail([](const std::runtime_error &err) { fprintf(stderr, "onUncaughtException in line %d, %s\n", __LINE__, err.what()); }).fail([]() {
            // go here for all other uncaught parameters.
            fprintf(stderr, "onUncaughtException in line %d\n", __LINE__);
        });
    };

    return &defaultUncaughtExceptionHandler;
}

void PromiseHolder::onUncaughtException(const any &arg) {
    any *onUncaughtException = getUncaughtExceptionHandler();
    if (onUncaughtException == nullptr || onUncaughtException->empty()) {
        onUncaughtException = getDefaultUncaughtExceptionHandler();
    }

    try {
        onUncaughtException->call(reject(arg));
    } catch (...) {
        fprintf(stderr, "onUncaughtException in line %d\n", __LINE__);
    }
}

void PromiseHolder::handleUncaughtException(const any &onUncaughtException) { (*getUncaughtExceptionHandler()) = onUncaughtException; }

#if METADOT_PROMISE_MULTITHREAD
std::shared_ptr<Mutex> SharedPromise::obtainLock() const {
    while (true) {
        std::shared_ptr<Mutex> mutex = this->promiseHolder_->mutex_;
        mutex->lock();

        // pointer to mutex may be changed after locked,
        // in this case we should try to lock and test again
        if (mutex == this->promiseHolder_->mutex_) return mutex;
        mutex->unlock();
    }
    return nullptr;
}
#endif

Promise &Promise::then(const any &deferOrPromiseOrOnResolved) {
    if (deferOrPromiseOrOnResolved.type() == type_id<Defer>()) {
        Defer &defer = deferOrPromiseOrOnResolved.cast<Defer &>();
        Promise promise = defer.getPromise();
        Promise &ret = then(
                [defer](const any &arg) -> any {
                    defer.resolve(arg);
                    return nullptr;
                },
                [defer](const any &arg) -> any {
                    defer.reject(arg);
                    return nullptr;
                });

        promise.finally([=]() { ret.reject(); });

        return ret;
    } else if (deferOrPromiseOrOnResolved.type() == type_id<DeferLoop>()) {
        DeferLoop &loop = deferOrPromiseOrOnResolved.cast<DeferLoop &>();
        Promise promise = loop.getPromise();

        Promise &ret = then(
                [loop](const any &arg) -> any {
                    (void)arg;
                    loop.doContinue();
                    return nullptr;
                },
                [loop](const any &arg) -> any {
                    loop.reject(arg);
                    return nullptr;
                });

        promise.finally([=]() { ret.reject(); });

        return ret;
    } else if (deferOrPromiseOrOnResolved.type() == type_id<Promise>()) {
        Promise &promise = deferOrPromiseOrOnResolved.cast<Promise &>();

        std::shared_ptr<Task> task;
        {
#if METADOT_PROMISE_MULTITHREAD
            std::shared_ptr<Mutex> mutex0 = this->sharedPromise_->obtainLock();
            std::lock_guard<Mutex> lock0(*mutex0, std::adopt_lock_t());
            std::shared_ptr<Mutex> mutex1 = promise.sharedPromise_->obtainLock();
            std::lock_guard<Mutex> lock1(*mutex1, std::adopt_lock_t());
#endif

            if (promise.sharedPromise_ && promise.sharedPromise_->promiseHolder_) {
                join(this->sharedPromise_->promiseHolder_, promise.sharedPromise_->promiseHolder_);
                if (this->sharedPromise_->promiseHolder_->pendingTasks_.size() > 0) {
                    task = this->sharedPromise_->promiseHolder_->pendingTasks_.front();
                }
            }
        }
        if (task) call(task);
        return *this;
    } else {
        return then(deferOrPromiseOrOnResolved, any());
    }
}

Promise &Promise::then(const any &onResolved, const any &onRejected) {
    std::shared_ptr<Task> task;
    {
#if METADOT_PROMISE_MULTITHREAD
        std::shared_ptr<Mutex> mutex = this->sharedPromise_->obtainLock();
        std::lock_guard<Mutex> lock(*mutex, std::adopt_lock_t());
#endif

        task = ME::create_ref<Task>(Task{TaskState::kPending, sharedPromise_->promiseHolder_, onResolved, onRejected});
        sharedPromise_->promiseHolder_->pendingTasks_.push_back(task);
    }
    call(task);
    return *this;
}

Promise &Promise::fail(const any &onRejected) { return then(any(), onRejected); }

Promise &Promise::always(const any &onAlways) { return then(onAlways, onAlways); }

Promise &Promise::finally(const any &onFinally) {
    return then(
            [onFinally](const any &arg) -> any {
                return newPromise([onFinally, arg](Defer &defer) {
                    try {
                        onFinally.call(arg);
                    } catch (bad_any_cast &) {
                    }
                    defer.resolve(arg);
                });
            },
            [onFinally](const any &arg) -> any {
                return newPromise([onFinally, arg](Defer &defer) {
                    try {
                        onFinally.call(arg);
                    } catch (bad_any_cast &) {
                    }
                    defer.reject(arg);
                });
            });
}

void Promise::resolve(const any &arg) const {
    if (!this->sharedPromise_) return;
    std::shared_ptr<Task> task;
    {
#if METADOT_PROMISE_MULTITHREAD
        std::shared_ptr<Mutex> mutex = this->sharedPromise_->obtainLock();
        std::lock_guard<Mutex> lock(*mutex, std::adopt_lock_t());
#endif

        std::list<std::shared_ptr<Task>> &pendingTasks_ = this->sharedPromise_->promiseHolder_->pendingTasks_;
        if (pendingTasks_.size() > 0) {
            task = pendingTasks_.front();
        }
    }

    if (task) {
        Defer defer(task);
        defer.resolve(arg);
    }
}

void Promise::reject(const any &arg) const {
    if (!this->sharedPromise_) return;
    std::shared_ptr<Task> task;
    {
#if METADOT_PROMISE_MULTITHREAD
        std::shared_ptr<Mutex> mutex = this->sharedPromise_->obtainLock();
        std::lock_guard<Mutex> lock(*mutex, std::adopt_lock_t());
#endif

        std::list<std::shared_ptr<Task>> &pendingTasks_ = this->sharedPromise_->promiseHolder_->pendingTasks_;
        if (pendingTasks_.size() > 0) {
            task = pendingTasks_.front();
        }
    }

    if (task) {
        Defer defer(task);
        defer.reject(arg);
    }
}

void Promise::clear() { sharedPromise_.reset(); }

Promise::operator bool() const { return sharedPromise_.operator bool(); }

Promise newPromise(const std::function<void(Defer &defer)> &run) {
    Promise promise;
    promise.sharedPromise_ = ME::create_ref<SharedPromise>();
    promise.sharedPromise_->promiseHolder_ = ME::create_ref<PromiseHolder>();
    promise.sharedPromise_->promiseHolder_->owners_.push_back(promise.sharedPromise_);

    // return as is
    promise.then(any(), any());
    std::shared_ptr<Task> &task = promise.sharedPromise_->promiseHolder_->pendingTasks_.front();

    Defer defer(task);
    try {
        run(defer);
    } catch (...) {
        defer.reject(std::current_exception());
    }

    return promise;
}

Promise newPromise() {
    Promise promise;
    promise.sharedPromise_ = ME::create_ref<SharedPromise>();
    promise.sharedPromise_->promiseHolder_ = ME::create_ref<PromiseHolder>();
    promise.sharedPromise_->promiseHolder_->owners_.push_back(promise.sharedPromise_);

    // return as is
    promise.then(any(), any());
    return promise;
}

Promise doWhile(const std::function<void(DeferLoop &loop)> &run) {

    return newPromise([run](Defer &defer) {
               DeferLoop loop(defer);
               run(loop);
           })
            .then(
                    [run](const any &arg) -> any {
                        (void)arg;
                        return doWhile(run);
                    },
                    [](const any &arg) -> any {
                        return newPromise([arg](Defer &defer) {
                            // printf("arg. type = %s\n", arg.type().name());

                            bool isBreak = false;
                            if (arg.type() == type_id<std::vector<any>>()) {
                                std::vector<any> &args = any_cast<std::vector<any> &>(arg);
                                if (args.size() == 2 && args.front().type() == type_id<DoBreakTag>() && args.back().type() == type_id<std::vector<any>>()) {
                                    isBreak = true;
                                    defer.resolve(args.back());
                                }
                            }

                            if (!isBreak) {
                                defer.reject(arg);
                            }
                        });
                    });
}

#if 0
Promise reject(const any &arg) {
    return newPromise([arg](Defer &defer) { defer.reject(arg); });
}

Promise resolve(const any &arg) {
    return newPromise([arg](Defer &defer) { defer.resolve(arg); });
}
#endif

Promise all(const std::list<Promise> &promise_list) {
    if (promise_list.size() == 0) {
        return resolve();
    }

    std::shared_ptr<size_t> finished = ME::create_ref<size_t>(0);
    std::shared_ptr<size_t> size = ME::create_ref<size_t>(promise_list.size());
    std::shared_ptr<std::vector<any>> retArr = ME::create_ref<std::vector<any>>();
    retArr->resize(*size);

    return newPromise([=](Defer &defer) {
        size_t index = 0;
        for (auto promise : promise_list) {
            promise.then(
                    [=](const any &arg) {
                        (*retArr)[index] = arg;
                        if (++(*finished) >= *size) {
                            defer.resolve(*retArr);
                        }
                    },
                    [=](const any &arg) { defer.reject(arg); });

            ++index;
        }
    });
}

static Promise race(const std::list<Promise> &promise_list, std::shared_ptr<int> winner) {
    return newPromise([=](Defer &defer) {
        int index = 0;
        for (auto it = promise_list.begin(); it != promise_list.end(); ++it, ++index) {
            auto promise = *it;
            promise.then(
                    [=](const any &arg) {
                        *winner = index;
                        defer.resolve(arg);
                        return arg;
                    },
                    [=](const any &arg) {
                        *winner = index;
                        defer.reject(arg);
                        return arg;
                    });
        }
    });
}

Promise race(const std::list<Promise> &promise_list) {
    std::shared_ptr<int> winner = ME::create_ref<int>(-1);
    return race(promise_list, winner);
}

Promise raceAndReject(const std::list<Promise> &promise_list) {
    std::shared_ptr<int> winner = ME::create_ref<int>(-1);
    return race(promise_list, winner).finally([promise_list, winner] {
        int index = 0;
        for (auto it = promise_list.begin(); it != promise_list.end(); ++it, ++index) {
            if (index != *winner) {
                auto promise = *it;
                promise.reject();
            }
        }
    });
}

Promise raceAndResolve(const std::list<Promise> &promise_list) {
    std::shared_ptr<int> winner = ME::create_ref<int>(-1);
    return race(promise_list, winner).finally([promise_list, winner] {
        int index = 0;
        for (auto it = promise_list.begin(); it != promise_list.end(); ++it, ++index) {
            if (index != *winner) {
                auto promise = *it;
                promise.resolve();
            }
        }
    });
}

}  // namespace ME::cpp::promise

#endif
