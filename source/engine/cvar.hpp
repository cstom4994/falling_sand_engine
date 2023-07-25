// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_CVAR_HPP
#define ME_CVAR_HPP

#include <cstddef>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <map>
#include <ostream>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <vector>

#include "engine/core/core.hpp"
#include "engine/game_utils/jsonwarp.h"
#include "engine/meta/reflection.hpp"
#include "engine/utils/type.hpp"
#include "libs/parallel_hashmap/phmap.h"

namespace ME {

enum CommandType { CVAR_VAR = 0, CVAR_FUNC = 1 };

struct GlobalDEF {
    bool draw_frame_graph;
    bool draw_background;
    bool draw_background_grid;
    bool draw_load_zones;
    bool draw_physics_debug;
    bool draw_b2d_shape;
    bool draw_b2d_joint;
    bool draw_b2d_aabb;
    bool draw_b2d_pair;
    bool draw_b2d_centerMass;
    bool draw_chunk_state;
    bool draw_debug_stats;
    bool draw_material_info;
    bool draw_detailed_material_info;
    bool draw_uinode_bounds;
    bool draw_temperature_map;
    bool draw_cursor;

    bool ui_tweak;

    bool draw_shaders;
    int water_overlay;
    bool water_showFlow;
    bool water_pixelated;
    f32 lightingQuality;
    bool draw_light_overlay;
    bool simpleLighting;
    bool lightingEmission;
    bool lightingDithering;

    bool tick_world;
    bool tick_box2d;
    bool tick_temperature;
    bool hd_objects;

    int hd_objects_size;

    bool draw_ui_debug;
    bool draw_imgui_debug;
    bool draw_profiler;
    bool draw_console;
    bool draw_pack_editor;
    bool draw_code_editor;

    int cell_iter;
    int brush_size;
};
// METADOT_STRUCT(GlobalDEF, draw_frame_graph, draw_background, draw_background_grid, draw_load_zones, draw_physics_debug, draw_b2d_shape, draw_b2d_joint, draw_b2d_aabb, draw_b2d_pair,
//                draw_b2d_centerMass, draw_chunk_state, draw_debug_stats, draw_material_info, draw_detailed_material_info, draw_uinode_bounds, draw_temperature_map, draw_cursor, ui_tweak,
//                draw_shaders, water_overlay, water_showFlow, water_pixelated, lightingQuality, draw_light_overlay, simpleLighting, lightingEmission, lightingDithering, tick_world, tick_box2d,
//                tick_temperature, hd_objects, hd_objects_size, draw_ui_debug, draw_imgui_debug, draw_profiler, draw_console, draw_pack_editor, cell_iter);

void InitGlobalDEF(GlobalDEF* s, bool openDebugUIs);
void LoadGlobalDEF(std::string globaldef_src);
void SaveGlobalDEF(std::string globaldef_src);

namespace cvar {

template <typename T>
T Cast(std::string) {
    ME_ASSERT("Cast unavailable");
    return T{};
}

#define CVAR_CAST_DEF(_type, _func)          \
    template <>                              \
    _type cvar::Cast<_type>(std::string s) { \
        return _func(s);                     \
    }

#define CVAR_CAST_DECL(_type) \
    template <>               \
    _type Cast<_type>(std::string s)

CVAR_CAST_DECL(bool);
CVAR_CAST_DECL(char);
CVAR_CAST_DECL(short);
CVAR_CAST_DECL(int);
CVAR_CAST_DECL(long);
CVAR_CAST_DECL(float);
CVAR_CAST_DECL(double);
CVAR_CAST_DECL(long double);
CVAR_CAST_DECL(std::string);
CVAR_CAST_DECL(const char*);

template <typename T>
std::string NameOFType() {
    std::string_view name_of_type = cpp::type_name<T>().View();
    std::string name_of_type_str = std::string(name_of_type.data(), name_of_type.size());
    return name_of_type_str;
}

using Json = ::ME::Json::Json;

template <typename T>
struct Translator {
    typedef T type;

    static void BuildJson(Json& j, const T& v) { j.add(v); }
    static std::string to_str(const T& v) { return std::to_string(v); }
    static T Parse(std::string s) { return Cast<T>(s); }
    static std::string Name() { return NameOFType<T>(); }
};

template <template <typename...> class C, typename F, typename... E>
struct Translator<C<F, E...>> {
    typedef F type;

    static void BuildJson(Json& j, const C<F, E...>& c) {
        Json k = Json::array();
        for (auto& v : c) Translator<F>::BuildJson(k, v);
        j.add(k);
    }

    static std::string to_str(const C<F, E...>& c) {
        Json j = Json::array();
        BuildJson(j, c);
        Json f = j[0];
        std::stringstream ss;
        ss << f.print();
        return ss.str();
    }

    static C<F, E...> Parse(std::string str) {

        try {
            C<F, E...> c;

            Json j = Json::parse(str);

            for (auto& e : j) {
                std::stringstream ss;
                ss << e.print();
                c.push_back(Translator<F>::Parse(ss.str()));
            }

            return c;

        } catch (std::exception& e) {
            throw e.what();
        }
    }

    static std::string Name() { return NameOFType<C<F, E...>>(); }
};

template <>
struct Translator<std::string> {
    typedef std::string type;

    static void BuildJson(Json& j, const std::string& str) { j.add(str); }

    static std::string to_str(const std::string& str) { return str; }

    static std::string Parse(std::string str) {
        auto begin = str.cbegin();
        auto end = str.cend();

        if (str.size() > 1 && *begin == '\"' && *(end - 1) == '\"') return std::string(begin + 1, end - 1);

        return str;
    }

    static std::string Name() { return NameOFType<std::string>(); }
};

template <>
struct Translator<void> {
    typedef void type;

    static std::string Name() { return NameOFType<void>(); }
};

// Call command with return valve
template <typename R>
class CommandCaller {
public:
    template <typename... Args>
    static std::string Call(std::function<R(Args...)> f, Args... args) {
        return Translator<R>::to_str(f(args...));
    }
};

// Call command without return valve
template <>
class CommandCaller<void> {
public:
    template <typename... Args>
    static std::string Call(std::function<void(Args...)> f, Args... args) {
        f(args...);
        return std::string();
    }
};

template <typename R, typename... Args>
std::string CallToString(std::function<R(Args...)> f, Args... args) {
    return CommandCaller<R>::Call(f, args...);
}

class CommandParameter {

    std::string type;
    std::string name;

    CommandParameter(std::string type = "", std::string name = "");

public:
    template <typename T>
    static CommandParameter Make(std::string name = "");

    std::string GetType() const;
    std::string GetName() const;
};

class BaseCommand {

public:
    typedef std::vector<CommandParameter> CommandArgs;

    typedef CommandArgs::iterator iterator;
    // typedef CommandArgs::const_iterator const_iterator;

private:
    std::string name;
    unsigned int num_args;

    std::vector<CommandParameter> params;

protected:
    void AddParameter(const CommandParameter&);

public:
    BaseCommand(std::string name);
    virtual ~BaseCommand();

    virtual CommandType GetCmdType() const = 0;
    virtual std::string GetReturnType() const = 0;

    std::string GetName() const;
    CommandArgs::size_type size() const;

    iterator begin();
    iterator end();
    // const_iterator cbegin() const;
    // const_iterator cend() const;

    virtual std::string Call(std::queue<std::string>) const = 0;
};

template <typename R, typename... FA>
class FunctionCommand : public BaseCommand {

    template <typename... T>
    class ArgsConverter {

        int N;

    public:
        ArgsConverter(int n);
        void for_each(std::function<void(const CommandParameter&)>);
        template <typename... A>
        std::string Call(std::queue<std::string> cmd, std::function<R(FA...)> func, A... args) const;
    };

    template <typename C, typename... Next>
    class ArgsConverter<C, Next...> {

        int N;
        ArgsConverter<Next...> next;

    public:
        ArgsConverter<C, Next...>(int N);
        void for_each(std::function<void(const CommandParameter&)>);
        template <typename... A>
        std::string Call(std::queue<std::string> cmd, std::function<R(FA...)> func, A... args) const;
    };

public:
    std::function<R(FA...)> func;
    ArgsConverter<FA...> converter;

public:
    FunctionCommand(std::string name, std::function<R(FA...)> func);
    CommandType GetCmdType() const;
    std::string GetReturnType() const;
    std::string Call(std::queue<std::string> cmd) const;
};

template <typename T>
class VariableCommand : public BaseCommand {

    T& variable;

public:
    VariableCommand(std::string name, T& var);
    CommandType GetCmdType() const;
    std::string GetReturnType() const;
    std::string Call(std::queue<std::string>) const;
};

template <typename T>
class VariableCommand<const T> : public BaseCommand {

    const T& variable;

public:
    VariableCommand(std::string name, const T& var);
    CommandType GetCmdType() const;
    std::string GetReturnType() const;
    std::string Call(std::queue<std::string>) const;
};

class ConVar {

public:
    typedef phmap::flat_hash_map<std::string, BaseCommand*> ConVars;
    typedef ConVars::const_iterator const_iterator;

    ConVars convars;

public:
    ~ConVar();

    template <typename R, typename... T>
    void Command(std::string name, R (*func)(T...));
    template <typename C, typename R, typename... T>
    void Command(std::string name, R (C::*func)(T...), C* object);
    template <typename L>
    void Command(std::string name, L lambda);
    template <typename T>
    void Value(std::string name, T& var);

    void RemoveCommand(std::string name);
    std::string Call(std::string name, std::queue<std::string> args);

public:
    const_iterator begin() const;
    const_iterator end() const;
};

template <typename T>
cvar::CommandParameter cvar::CommandParameter::Make(std::string name) {
    return CommandParameter(Translator<T>::Name(), name);
}

template <typename R, typename... FA>
template <typename... T>
cvar::FunctionCommand<R, FA...>::ArgsConverter<T...>::ArgsConverter(int n) {}

template <typename R, typename... FA>
template <typename... T>
void cvar::FunctionCommand<R, FA...>::ArgsConverter<T...>::for_each(std::function<void(const CommandParameter&)> f) {}

template <typename R, typename... FA>
template <typename... T>
template <typename... A>
std::string cvar::FunctionCommand<R, FA...>::ArgsConverter<T...>::Call(std::queue<std::string> s, std::function<R(FA...)> func, A... args) const {
    try {
        return CallToString<R, FA...>(func, args...);
    } catch (std::exception& e) {
        throw e.what();
    }
}

template <typename R, typename... FA>
template <typename C, typename... Next>
cvar::FunctionCommand<R, FA...>::ArgsConverter<C, Next...>::ArgsConverter(int n) : N(n), next(n + 1) {}

template <typename R, typename... FA>
template <typename C, typename... Next>
void cvar::FunctionCommand<R, FA...>::ArgsConverter<C, Next...>::for_each(std::function<void(const CommandParameter&)> f) {
    CommandParameter p = CommandParameter::Make<C>();
    f(p);
    next.for_each(f);
}

template <typename R, typename... FA>
template <typename C, typename... Next>
template <typename... A>
std::string cvar::FunctionCommand<R, FA...>::ArgsConverter<C, Next...>::Call(std::queue<std::string> stack, std::function<R(FA...)> func, A... args) const {
    std::string arg = stack.front();
    stack.pop();

    try {
        C val = Translator<C>::Parse(arg);
        return next.Call(stack, func, args..., val);
    } catch (std::exception& e) {
        throw e.what();
    }
}

template <typename R, typename... FA>
cvar::FunctionCommand<R, FA...>::FunctionCommand(std::string name, std::function<R(FA...)> func) : BaseCommand(name), func(func), converter(1) {
    converter.for_each([this](const CommandParameter& p) { this->AddParameter(p); });
}

template <typename R, typename... FA>
CommandType cvar::FunctionCommand<R, FA...>::GetCmdType() const {
    return CommandType::CVAR_FUNC;
}

template <typename R, typename... FA>
std::string cvar::FunctionCommand<R, FA...>::GetReturnType() const {
    return Translator<R>::Name();
}

template <typename R, typename... FA>
std::string cvar::FunctionCommand<R, FA...>::Call(std::queue<std::string> cmd) const {
    if (cmd.size() != sizeof...(FA)) {
        // throw std::exception(GetName(), sizeof...(FA), cmd.size());
    }
    return converter.Call(cmd, func);
}

template <template <typename...> class F, typename R, typename... T>
cvar::FunctionCommand<R, T...>* MakeFunctionCommand(std::string name, F<R(T...)> func) {
    return new cvar::FunctionCommand<R, T...>(name, func);
}

template <typename T>
cvar::VariableCommand<T>::VariableCommand(std::string name, T& var) : BaseCommand(name), variable(var) {}

template <typename T>
CommandType cvar::VariableCommand<T>::GetCmdType() const {
    return CommandType::CVAR_VAR;
}

template <typename T>
std::string cvar::VariableCommand<T>::GetReturnType() const {
    return Translator<T>::Name();
}

template <typename T>
std::string cvar::VariableCommand<T>::Call(std::queue<std::string> cmd) const {
    if (cmd.empty())
        return Translator<T>::to_str(variable);
    else if (cmd.size() > 1) {
        // throw std::exception(GetName(), 1, cmd.size());
    } else {
        try {
            variable = Translator<T>::Parse(cmd.front());
        } catch (std::exception& e) {
            throw e.what();
        }
    }

    return "";
}

template <typename T>
cvar::VariableCommand<const T>::VariableCommand(std::string name, const T& var) : BaseCommand(name), variable(var) {}

template <typename T>
CommandType cvar::VariableCommand<const T>::GetCmdType() const {
    return CommandType::CVAR_VAR;
}

template <typename T>
std::string cvar::VariableCommand<const T>::GetReturnType() const {
    return Translator<const T>::name();
}

template <typename T>
std::string cvar::VariableCommand<const T>::Call(std::queue<std::string> cmd) const {
    if (cmd.empty()) return cvar::to_string<T>(variable);

    throw read_only_variable();
}

template <typename R, typename... T>
void cvar::ConVar::Command(std::string name, R (*funcPtr)(T...)) {
    std::function<R(T...)> func(funcPtr);
    convars[name] = new FunctionCommand<R, T...>(name, func);
}

template <typename C, typename R, typename... T>
void cvar::ConVar::Command(std::string name, R (C::*funcPtr)(T...), C* object) {
    std::function<R(T...)> func([=](T... args) -> R { return ((*object).*funcPtr)(args...); });
    convars[name] = new FunctionCommand<R, T...>(name, func);
}

template <typename L>
void cvar::ConVar::Command(std::string name, L lambda) {
    auto func = to_function(lambda);
    convars[name] = MakeFunctionCommand(name, func);
}

template <typename T>
void cvar::ConVar::Value(std::string name, T& var) {
    convars[name] = new VariableCommand<T>(name, var);
}

}  // namespace cvar

}  // namespace ME

#endif