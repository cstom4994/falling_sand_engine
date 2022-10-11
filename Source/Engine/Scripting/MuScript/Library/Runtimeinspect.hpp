// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.

#pragma once


#include <cstddef>


#include <functional>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <map>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <vector>

#include <iostream>
#include <limits>
#include <queue>
#include <string>
#include <unordered_map>

/*
 *	Taken from:
 *	http://stackoverflow.com/questions/81870/is-it-possible-to-print-a-variables-type-in-standard-c
 */

#include <cstddef>
#include <cstring>
#include <ostream>
#include <stdexcept>

#include <exception>
#include <string>

#include "Engine/Core.hpp"

class static_string {
    const char *const p_;
    const std::size_t sz_;

public:
    typedef const char *const_iterator;

    template<std::size_t N>
    constexpr static_string(const char (&a)[N]) noexcept
        : p_(a), sz_(N - 1) {}

    constexpr static_string(const char *p, std::size_t N) noexcept
        : p_(p), sz_(N) {}

    constexpr const char *data() const noexcept { return p_; }
    constexpr std::size_t size() const noexcept { return sz_; }

    constexpr const_iterator begin() const noexcept { return p_; }
    constexpr const_iterator end() const noexcept { return p_ + sz_; }

    constexpr char operator[](std::size_t n) const {
        return n < sz_ ? p_[n] : throw std::out_of_range("static_string");
    }
};

inline std::ostream &
operator<<(std::ostream &os, static_string const &s) {
    return os.write(s.data(), s.size());
}

template<class T>
static_string
type_name() {
#ifdef __clang__
    static_string p = __PRETTY_FUNCTION__;
    return static_string(p.data() + 31, p.size() - 31 - 1);
#elif defined(__GNUC__)
    static_string p = __PRETTY_FUNCTION__;
#if __cplusplus < 201402
    return static_string(p.data() + 36, p.size() - 36 - 1);
#else
    return static_string(p.data() + 46, p.size() - 46 - 1);
#endif
#elif defined(_MSC_VER)
    static_string p = __FUNCSIG__;
    return static_string(p.data() + 38, p.size() - 38 - 7);
#endif
}


namespace MuScript::Inspect {
    struct argument_exception : public std::exception
    {
        unsigned int argN;
        std::string value;
        std::string explanation;

        argument_exception();
        argument_exception(std::string what);
        argument_exception(unsigned int, std::string, std::string what = "");

        const char *what() const noexcept;
    };

    struct no_cast_available : public argument_exception
    {
        no_cast_available();
        no_cast_available(unsigned int, std::string);
    };

    struct out_of_range : public argument_exception
    {
        out_of_range(unsigned int, std::string);
    };

    struct invalid_argument : public argument_exception
    {
        invalid_argument(unsigned int, std::string);
    };

    struct cmd_not_found
    {
        std::string command;
        cmd_not_found(std::string);
    };

    struct read_only_variable
    {
    };

    struct parse_exception : public argument_exception
    {
        parse_exception(std::string what);
    };

    struct wrong_argument_count
    {
        std::string command;
        unsigned int expected;
        unsigned int provided;

        wrong_argument_count(std::string, unsigned int, unsigned int);
    };

    struct command_exception
    {

        std::string whatstr;

        command_exception();
        command_exception(std::string);

        const char *what() const noexcept;
    };

}// namespace MuScript::Inspect

//#include "jsoncons/json.hpp"

//#include <cxxabi.h>

namespace MuScript::Inspect {
    // Parsing from string to argument type

    template<typename Received, typename Test>
    void test_num_limit(Received v) {
        if (v > std::numeric_limits<Test>::max())
            throw std::out_of_range("is above max numeric limit!");
        if (v < std::numeric_limits<Test>::min())
            throw std::out_of_range("is below numeric limit!");
    };

    template<typename T>
    T cast(std::string) {
        throw no_cast_available(0, "");
    }

    template<>
    char cast<char>(std::string s);
    template<>
    short cast<short>(std::string s);
    template<>
    int cast<int>(std::string s);
    template<>
    long cast<long>(std::string s);
    template<>
    float cast<float>(std::string s);
    template<>
    double cast<double>(std::string s);
    template<>
    long double cast<long double>(std::string s);
    template<>
    std::string cast<std::string>(std::string s);
    template<>
    const char *cast<const char *>(std::string s);

    // Converting from argument type to string

    template<typename T>
    std::string to_string(const T &val) {
        std::stringstream ss;
        ss << val;
        return ss.str();
    }

    // Converting from parameter type to string

    template<typename T>
    std::string type_str() {
        static_string s = type_name<T>();
        std::string n = std::string(s.data(), s.size());
        std::string m = "std::basic_string<char>";
        size_t p = n.find(m);
        if (p != std::string::npos) {
            std::string r = "std::string";
            n.replace(p, m.size(), r);
        }
        return n;
    }

    // Converters between template structures and string

    //using json = jsoncons::json;

    template<typename T>
    struct translator
    {
        typedef T type;

        // static void build_json(json& j, const T& v) {
        //     j.add(v);
        // }

        static std::string to_str(const T &v) {
            return to_string(v);
        }

        static T parse(std::string s) {
            return cast<T>(s);
        }

        static std::string name() {
            return type_str<T>();
        }
    };

    template<template<typename...> class C, typename F, typename... E>
    struct translator<C<F, E...>>
    {
        typedef F type;

        // static void build_json(json& j, const C<F, E...>& c) {
        //     json k = json::array();
        //     for (auto& v : c)
        //         translator<F>::build_json(k, v);
        //     j.add(k);
        // }

        // static std::string to_str(const C<F, E...>& c) {
        //     json j = json::array();
        //     build_json(j, c);
        //     json f = j[0];
        //     std::stringstream ss;
        //     ss << pretty_print(f);
        //     return ss.str();
        // }

        // static C<F, E...> parse(std::string s) {

        //     try {
        //         C<F, E...> c;

        //         json j = json::parse(s);

        //         for (auto& e : j.elements()) {
        //             std::stringstream ss;
        //             ss << pretty_print(e);
        //             c.push_back(translator<F>::parse(ss.str()));
        //         }

        //         return c;

        //     }
        //     catch (jsoncons::parse_exception& e) {
        //         throw parse_exception(e.what());
        //     }
        // }

        static std::string name() {
            return type_str<C<F, E...>>();
        }
    };

    template<>
    struct translator<std::string>
    {
        typedef std::string type;

        // static void build_json(json& j, const std::string& s) {
        //     j.add(s);
        // }

        static std::string to_str(const std::string &s) {
            return s;
        }

        static std::string parse(std::string s) {
            auto b = s.cbegin();
            auto e = s.cend();

            if (s.size() > 1 && *b == '\"' && *(e - 1) == '\"')
                return std::string(b + 1, e - 1);

            return s;
        }

        static std::string name() {
            return type_str<std::string>();
        }
    };

    template<>
    struct translator<void>
    {
        typedef void type;

        static std::string name() {
            return type_str<void>();
        }
    };

    // Calling void and non-void functions

    template<typename Ret>
    class devoid {
    public:
        template<typename... Args>
        static std::string call(std::function<Ret(Args...)> f, Args... args) {
            return translator<Ret>::to_str(f(args...));
        }
    };

    template<>
    class devoid<void> {
    public:
        template<typename... Args>
        static std::string call(std::function<void(Args...)> f, Args... args) {
            f(args...);
            return std::string();
        }
    };

    template<typename Ret, typename... Args>
    std::string call_to_string(std::function<Ret(Args...)> f, Args... args) {
        return devoid<Ret>::call(f, args...);
    }


}// namespace MuScript::Inspect

#include <string>

namespace MuScript::Inspect {
    class parameter {

        std::string type;
        std::string name;

        parameter(std::string type = "", std::string name = "");

    public:
        template<typename T>
        static parameter make_parameter(std::string name = "");

        std::string get_type() const;
        std::string get_name() const;
    };
}// namespace MuScript::Inspect

template<typename T>
MuScript::Inspect::parameter MuScript::Inspect::parameter::make_parameter(std::string name) {
    return parameter(translator<T>::name(), name);
}


namespace MuScript::Inspect {
    class i_cmd {

    public:
        enum form {
            variable,
            function
        };

        typedef std::vector<parameter> args_t;
        typedef args_t::iterator iterator;
        typedef args_t::const_iterator const_iterator;

    private:
        std::string name;
        unsigned int num_args;

        std::vector<parameter> params;

    protected:
        void add_parameter(const parameter &);

    public:
        i_cmd(std::string name);
        virtual ~i_cmd();

        virtual form get_form() const = 0;
        virtual std::string get_return_type() const = 0;

        std::string get_name() const;
        args_t::size_type size() const;

        iterator begin();
        iterator end();

        const_iterator cbegin() const;
        const_iterator cend() const;

        virtual std::string call(std::queue<std::string>) const = 0;
    };
}// namespace MuScript::Inspect


namespace MuScript::Inspect {
    template<typename Ret, typename... FA>
    class function_cmd : public i_cmd {

        template<typename... T>
        class converter {

            int argN;

        public:
            converter(int n);
            void for_each(std::function<void(const parameter &)>);
            template<typename... A>
            std::string call(std::queue<std::string> q, std::function<Ret(FA...)> func, A... args) const;
        };

        template<typename Current, typename... Next>
        class converter<Current, Next...> {

            int argN;
            converter<Next...> next;

        public:
            converter<Current, Next...>(int argN);
            void for_each(std::function<void(const parameter &)>);
            template<typename... A>
            std::string call(std::queue<std::string> q, std::function<Ret(FA...)> func, A... args) const;
        };

        std::function<Ret(FA...)> func;
        converter<FA...> conv;

    public:
        function_cmd(std::string name, std::function<Ret(FA...)> func);
        i_cmd::form get_form() const;
        std::string get_return_type() const;
        std::string call(std::queue<std::string> q) const;
    };

}// namespace MuScript::Inspect


template<typename Ret, typename... FA>
template<typename... T>
MuScript::Inspect::function_cmd<Ret, FA...>::converter<T...>::converter(int n) {}

template<typename Ret, typename... FA>
template<typename... T>
void MuScript::Inspect::function_cmd<Ret, FA...>::converter<T...>::for_each(std::function<void(const parameter &)> f) {
}

template<typename Ret, typename... FA>
template<typename... T>
template<typename... A>
std::string MuScript::Inspect::function_cmd<Ret, FA...>::converter<T...>::call(std::queue<std::string> s, std::function<Ret(FA...)> func, A... args) const {
    try {
        return call_to_string<Ret, FA...>(func, args...);
    } catch (std::exception &e) {
        throw command_exception(e.what());
    } catch (...) {
        throw command_exception();
    }
}

template<typename Ret, typename... FA>
template<typename Current, typename... Next>
MuScript::Inspect::function_cmd<Ret, FA...>::converter<Current, Next...>::converter(int n) : argN(n), next(n + 1) {}

template<typename Ret, typename... FA>
template<typename Current, typename... Next>
void MuScript::Inspect::function_cmd<Ret, FA...>::converter<Current, Next...>::for_each(std::function<void(const parameter &)> f) {
    parameter p = parameter::make_parameter<Current>();
    f(p);
    next.for_each(f);
}

template<typename Ret, typename... FA>
template<typename Current, typename... Next>
template<typename... A>
std::string MuScript::Inspect::function_cmd<Ret, FA...>::converter<Current, Next...>::call(std::queue<std::string> stack, std::function<Ret(FA...)> func, A... args) const {
    std::string arg = stack.front();
    stack.pop();

    try {
        Current val = translator<Current>::parse(arg);
        return next.call(stack, func, args..., val);
    } catch (std::out_of_range) {
        throw out_of_range(argN, arg);
    } catch (std::invalid_argument &e) {
        throw argument_exception(argN, arg, "invalid argument");
    } catch (parse_exception &e) {
        e.argN = argN;
        e.value = arg;
        throw e;
    } catch (no_cast_available e) {
        if (e.argN > 0)
            throw e;
        throw no_cast_available(argN, arg);
    } catch (command_exception e) {
        throw e;
    }
}

template<typename Ret, typename... FA>
MuScript::Inspect::function_cmd<Ret, FA...>::function_cmd(std::string name, std::function<Ret(FA...)> func) : i_cmd(name), func(func), conv(1) {
    conv.for_each(
            [this](const parameter &p) {
                this->add_parameter(p);
            });
}

template<typename Ret, typename... FA>
MuScript::Inspect::i_cmd::form MuScript::Inspect::function_cmd<Ret, FA...>::get_form() const {
    return i_cmd::function;
}

template<typename Ret, typename... FA>
std::string MuScript::Inspect::function_cmd<Ret, FA...>::get_return_type() const {
    return translator<Ret>::name();
}

template<typename Ret, typename... FA>
std::string MuScript::Inspect::function_cmd<Ret, FA...>::call(std::queue<std::string> q) const {
    if (q.size() != sizeof...(FA))
        throw wrong_argument_count(get_name(), sizeof...(FA), q.size());
    return conv.call(q, func);
}


namespace MuScript::Inspect {
    template<typename T>
    class variable_cmd : public i_cmd {

        T &variable;

    public:
        variable_cmd(std::string name, T &var);
        i_cmd::form get_form() const;
        std::string get_return_type() const;
        std::string call(std::queue<std::string>) const;
    };

    template<typename T>
    class variable_cmd<const T> : public i_cmd {

        const T &variable;

    public:
        variable_cmd(std::string name, const T &var);
        i_cmd::form get_form() const;
        std::string get_return_type() const;
        std::string call(std::queue<std::string>) const;
    };

}// namespace MuScript::Inspect


namespace MuScript::Inspect {
    class Instance {

        std::unordered_map<std::string, i_cmd *> commands;

    public:
        typedef std::unordered_map<std::string, i_cmd *>::const_iterator const_iterator;

        ~Instance();

        template<typename R, typename... T>
        void provide_command(std::string name, R (*func)(T...));
        template<typename C, typename R, typename... T>
        void provide_command(std::string name, R (C::*func)(T...), C *object);
        template<typename L>
        void provide_command(std::string name, L lambda);
        template<typename T>
        void provide_value(std::string name, T &var);

        void remove_command(std::string name);
        std::string call(std::string name, std::queue<std::string> args);
        const_iterator begin();
        const_iterator end();
    };

}// namespace MuScript::Inspect


namespace {
    template<typename Function>
    struct function_traits : public function_traits<decltype(&Function::operator())>
    {
    };

    template<typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType (ClassType::*)(Args...) const>
    {
        using return_type = ReturnType;
        using pointer = ReturnType (*)(Args...);
        using std_function = std::function<ReturnType(Args...)>;
    };

    template<typename Function>
    typename function_traits<Function>::std_function to_function(Function &lambda) {
        return typename function_traits<Function>::std_function(lambda);
    }

    template<template<typename...> class F, typename R, typename... T>
    MuScript::Inspect::function_cmd<R, T...> *mk_function_cmd(std::string name, F<R(T...)> func) {
        return new MuScript::Inspect::function_cmd<R, T...>(name, func);
    }
}// namespace

template<typename R, typename... T>
void MuScript::Inspect::Instance::provide_command(std::string name, R (*funcPtr)(T...)) {
    std::function<R(T...)> func(funcPtr);
    commands[name] = new function_cmd<R, T...>(name, func);
}

template<typename C, typename R, typename... T>
void MuScript::Inspect::Instance::provide_command(std::string name, R (C::*funcPtr)(T...), C *object) {
    std::function<R(T...)> func([=](T... args) -> R {
        return ((*object).*funcPtr)(args...);
    });
    commands[name] = new function_cmd<R, T...>(name, func);
}

template<typename L>
void MuScript::Inspect::Instance::provide_command(std::string name, L lambda) {
    auto func = to_function(lambda);
    commands[name] = mk_function_cmd(name, func);
}

template<typename T>
void MuScript::Inspect::Instance::provide_value(std::string name, T &var) {
    commands[name] = new variable_cmd<T>(name, var);
}

namespace MuScript::Inspect {
    class shell {
    private:
        enum exec_result {
            exec_ok,
            exec_error,
            exec_exit
        };

        // singleton
        static shell *singleton;

        // special commands
        static const std::string exit;
        static const std::string help;
        static const std::string about;
        static const std::string source;

        std::string execute(std::string command, std::queue<std::string> args, exec_result &);
        std::string shell_commands(std::string command, std::queue<std::string> args, exec_result &);

        bool eval(const char *line);

        void print_about();
        void print_signature(i_cmd *);
        void print_help();
        void source_files(std::queue<std::string> file_names);

        Instance &svc;
        std::string last_command;

    public:
        shell(Instance &);
        const Instance &get_instance();
        void start(std::string line);
    };
}// namespace MuScript::Inspect


template<typename T>
MuScript::Inspect::variable_cmd<T>::variable_cmd(std::string name, T &var) : i_cmd(name), variable(var) {
}

template<typename T>
MuScript::Inspect::i_cmd::form MuScript::Inspect::variable_cmd<T>::get_form() const {
    return i_cmd::variable;
}

template<typename T>
std::string MuScript::Inspect::variable_cmd<T>::get_return_type() const {
    return translator<T>::name();
}

template<typename T>
std::string MuScript::Inspect::variable_cmd<T>::call(std::queue<std::string> q) const {
    if (q.empty())
        return translator<T>::to_str(variable);
    else if (q.size() > 1)
        throw wrong_argument_count(get_name(), 1, q.size());
    else
        try {
            variable = translator<T>::parse(q.front());
        } catch (std::out_of_range e) {
            throw out_of_range(1, q.front());
        } catch (std::invalid_argument) {
            throw invalid_argument(1, q.front());
        } catch (no_cast_available) {
            throw no_cast_available(1, q.front());
        }

    return "";
}

template<typename T>
MuScript::Inspect::variable_cmd<const T>::variable_cmd(std::string name, const T &var) : i_cmd(name), variable(var) {
}

template<typename T>
MuScript::Inspect::i_cmd::form MuScript::Inspect::variable_cmd<const T>::get_form() const {
    return i_cmd::variable;
}

template<typename T>
std::string MuScript::Inspect::variable_cmd<const T>::get_return_type() const {
    return translator<const T>::name();
}

template<typename T>
std::string MuScript::Inspect::variable_cmd<const T>::call(std::queue<std::string> q) const {
    if (q.empty())
        return MuScript::Inspect::to_string<T>(variable);

    throw read_only_variable();
}
