// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_CODEREFLECTION_HPP_
#define _METADOT_CODEREFLECTION_HPP_

#include "Core/Core.hpp"
#include "Libs/nameof.hpp"

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace IamAfuckingNamespace {
    int func1(float f, char c);
    void func2(void);
    void func_log_info(std::string info);
}// namespace IamAfuckingNamespace

// any_function

#ifndef ANY_FUNCTION_H
#define ANY_FUNCTION_H

#include <cassert>   // For assert(...)
#include <functional>// For std::function<F>
#include <memory>    // For std::unique_ptr<T>
#include <vector>    // For std::vector<T>

namespace Meta {

    // https://stackoverflow.com/questions/26107041/how-can-i-determine-the-return-type-of-a-c11-member-function

    template<typename T>
    struct return_type;
    template<typename R, typename... Args>
    struct return_type<R (*)(Args...)>
    {
        using type = R;
    };
    template<typename R, typename C, typename... Args>
    struct return_type<R (C::*)(Args...)>
    {
        using type = R;
    };
    template<typename R, typename C, typename... Args>
    struct return_type<R (C::*)(Args...) const>
    {
        using type = R;
    };
    template<typename R, typename C, typename... Args>
    struct return_type<R (C::*)(Args...) volatile>
    {
        using type = R;
    };
    template<typename R, typename C, typename... Args>
    struct return_type<R (C::*)(Args...) const volatile>
    {
        using type = R;
    };
    template<typename T>
    using return_type_t = typename return_type<T>::type;

    struct any_function
    {
    public:
        struct type
        {
            const std::type_info *info;
            bool is_lvalue_reference, is_rvalue_reference;
            bool is_const, is_volatile;
            bool operator==(const type &r) const {
                return info == r.info && is_lvalue_reference == r.is_lvalue_reference &&
                       is_rvalue_reference == r.is_rvalue_reference && is_const == r.is_const &&
                       is_volatile == r.is_volatile;
            }
            bool operator!=(const type &r) const { return !(*this == r); }
            template<class T>
            static type capture() {
                return {&typeid(T), std::is_lvalue_reference<T>::value,
                        std::is_rvalue_reference<T>::value,
                        std::is_const<typename std::remove_reference<T>::type>::value,
                        std::is_volatile<typename std::remove_reference<T>::type>::value};
            }
        };

        class result {
            struct result_base
            {
                virtual ~result_base() {}
                virtual std::unique_ptr<result_base> clone() const = 0;
                virtual type get_type() const = 0;
                virtual void *get_address() = 0;
            };
            template<class T>
            struct typed_result : result_base
            {
                T x;
                typed_result(T x) : x(get((void *) &x, tag<T>{})) {}
                std::unique_ptr<result_base> clone() const {
                    return std::unique_ptr<typed_result>(
                            new typed_result(get((void *) &x, tag<T>{})));
                }
                type get_type() const { return type::capture<T>(); }
                void *get_address() { return (void *) &x; }
            };
            std::unique_ptr<result_base> p;

        public:
            result() {}
            result(result &&r) : p(move(r.p)) {}
            result(const result &r) { *this = r; }
            result &operator=(result &&r) {
                p.swap(r.p);
                return *this;
            }
            result &operator=(const result &r) {
                p = r.p ? r.p->clone() : nullptr;
                return *this;
            }

            type get_type() const { return p ? p->get_type() : type::capture<void>(); }
            void *get_address() { return p ? p->get_address() : nullptr; }
            template<class T>
            T get_value() {
                assert(get_type() == type::capture<T>());
                return get(p->get_address(), tag<T>{});
            }

            template<class T>
            static result capture(T x) {
                result r;
                r.p.reset(new typed_result<T>(static_cast<T>(x)));
                return r;
            }
        };
        any_function() : result_type{} {}
        any_function(std::nullptr_t) : result_type{} {}
        template<class R, class... A>
        any_function(R (*p)(A...))
            : any_function(p, tag<R>{}, tag<A...>{}, build_indices<sizeof...(A)>{}) {}
        template<class R, class... A>
        any_function(std::function<R(A...)> f)
            : any_function(f, tag<R>{}, tag<A...>{}, build_indices<sizeof...(A)>{}) {}
        template<class F>
        any_function(F f) : any_function(f, &F::operator()) {}

        explicit operator bool() const { return static_cast<bool>(func); }
        const std::vector<type> &get_parameter_types() const { return parameter_types; }
        const type &get_result_type() const { return result_type; }
        result invoke(void *const args[]) const { return func(args); }
        result invoke(std::initializer_list<void *> args) const { return invoke(args.begin()); }

    private:
        template<class... T>
        struct tag
        {
        };
        template<std::size_t... IS>
        struct indices
        {
        };
        template<std::size_t N, std::size_t... IS>
        struct build_indices : build_indices<N - 1, N - 1, IS...>
        {
        };
        template<std::size_t... IS>
        struct build_indices<0, IS...> : indices<IS...>
        {
        };

        template<class T>
        static T get(void *arg, tag<T>) {
            return *reinterpret_cast<T *>(arg);
        }
        template<class T>
        static T &get(void *arg, tag<T &>) {
            return *reinterpret_cast<T *>(arg);
        }
        template<class T>
        static T &&get(void *arg, tag<T &&>) {
            return std::move(*reinterpret_cast<T *>(arg));
        }
        template<class F, class R, class... A, size_t... I>
        any_function(F f, tag<R>, tag<A...>, indices<I...>)
            : parameter_types({type::capture<A>()...}), result_type(type::capture<R>()) {
            func = [f](void *const args[]) mutable {
                return result::capture<R>(f(get(args[I], tag<A>{})...));
            };
        }
        template<class F, class... A, size_t... I>
        any_function(F f, tag<void>, tag<A...>, indices<I...>)
            : parameter_types({type::capture<A>()...}), result_type(type::capture<void>()) {
            func = [f](void *const args[]) mutable {
                return f(get(args[I], tag<A>{})...), result{};
            };
        }
        template<class F, class R>
        any_function(F f, tag<R>, tag<>, indices<>)
            : parameter_types({}), result_type(type::capture<R>()) {
            func = [f](void *const args[]) mutable { return result::capture<R>(f()); };
        }
        template<class F>
        any_function(F f, tag<void>, tag<>, indices<>)
            : parameter_types({}), result_type(type::capture<void>()) {
            func = [f](void *const args[]) mutable { return f(), result{}; };
        }
        template<class F, class R, class... A>
        any_function(F f, R (F::*p)(A...))
            : any_function(f, tag<R>{}, tag<A...>{}, build_indices<sizeof...(A)>{}) {}
        template<class F, class R, class... A>
        any_function(F f, R (F::*p)(A...) const)
            : any_function(f, tag<R>{}, tag<A...>{}, build_indices<sizeof...(A)>{}) {}

        std::function<result(void *const *)> func;
        std::vector<type> parameter_types;
        type result_type;
    };

}// namespace Meta

#endif

template<typename T>
struct StructMetaInfo
{
    static std::tuple<> Info() { return std::make_tuple(); }
};

#define REFL(Struct, ...)                                                                          \
    template<>                                                                                     \
    struct StructMetaInfo<Struct>                                                                  \
    {                                                                                              \
        static decltype(std::make_tuple(__VA_ARGS__)) Info() {                                     \
            return std::make_tuple(__VA_ARGS__);                                                   \
        }                                                                                          \
    };

#define FIELD(class, field) std::make_tuple(#field, &class ::field)

template<typename T, typename Fields, typename F, size_t... Is>
void refl_foreach(T &&obj, Fields &&fields, F &&f, std::index_sequence<Is...>) {
    (void) std::initializer_list<size_t>{
            (f(std::get<0>(std::get<Is>(fields)), obj.*std::get<1>(std::get<Is>(fields))), Is)...};
}

template<typename T, typename F>
void refl_foreach(T &&obj, F &&f) {
    auto fields = StructMetaInfo<std::decay_t<T>>::Info();
    refl_foreach(std::forward<T>(obj), fields, std::forward<F>(f),
                 std::make_index_sequence<std::tuple_size<decltype(fields)>::value>{});
}

template<typename F>
struct refl_recur_func;

template<typename T, typename F,
         std::enable_if_t<std::is_class<std::decay_t<T>>::value> * = nullptr>
void refl_recur_obj(T &&obj, F &&f, const char *fieldName, int depth) {
    f(fieldName, depth);
    refl_foreach(obj, refl_recur_func<F>(f, depth));
}

template<typename T, typename F,
         std::enable_if_t<!std::is_class<std::decay_t<T>>::value> * = nullptr>
void refl_recur_obj(T &&obj, F &&f, const char *fieldName, int depth) {
    f(fieldName, depth);
}

template<typename F>
struct refl_recur_func
{
public:
    refl_recur_func(const F &_f, int _depth) : f(_f), depth(_depth) {}

    template<typename Value>
    void operator()(const char *fieldName, Value &&value) {
        refl_recur_obj(value, f, fieldName, depth + 1);
    }

private:
    F f;
    int depth;
};

namespace tmp {

    // type list
    template<typename... TS>
    struct typelist
    {
        static constexpr auto size = sizeof...(TS);
    };

    template<typename T>
    struct is_typelist : std::false_type
    {
    };

    template<typename... TS>
    struct is_typelist<typelist<TS...>> : std::true_type
    {
    };

    // basic operations
    template<typename T, typename TL>
    struct push_back;
    template<typename T, typename TL>
    struct push_front;
    template<typename TL>
    struct pop_front;
    template<typename TL, size_t I>
    struct at;

    template<typename T, typename... TS>
    struct push_back<T, typelist<TS...>>
    {
        using type = typelist<TS..., T>;
    };

    template<typename T, typename... TS>
    struct push_front<T, typelist<TS...>>
    {
        using type = typelist<T, TS...>;
    };

    template<typename T, typename... TS>
    struct pop_front<typelist<T, TS...>>
    {
        using type = typelist<TS...>;
    };

    template<typename T, typename... TS>
    struct at<typelist<T, TS...>, 0>
    {
        using type = T;
    };

    template<typename T, typename... TS, size_t I>
    struct at<typelist<T, TS...>, I>
    {
        static_assert(I < (1 + sizeof...(TS)), "Out of bounds access");
        using type = typename at<typelist<TS...>, I - 1>::type;
    };

    // 'filter'
    template<typename TL, template<typename> class PRED>
    struct filter;

    template<template<typename> class PRED>
    struct filter<typelist<>, PRED>
    {
        using type = typelist<>;
    };

    template<typename T, typename... TS, template<typename> class PRED>
    struct filter<typelist<T, TS...>, PRED>
    {
        using remaining = typename filter<typelist<TS...>, PRED>::type;
        using type =
                typename std::conditional<PRED<T>::value, typename push_front<T, remaining>::type,
                                          remaining>::type;
    };

    // 'max' given a template binary predicate
    template<typename TL, template<typename, typename> class PRED>
    struct max;

    template<typename T, template<typename, typename> class PRED>
    struct max<typelist<T>, PRED>
    {
        using type = T;
    };

    template<typename... TS, template<typename, typename> class PRED>
    struct max<typelist<TS...>, PRED>
    {
        using first = typename at<typelist<TS...>, 0>::type;
        using remaining_max = typename max<typename pop_front<typelist<TS...>>::type, PRED>::type;
        using type = typename std::conditional<PRED<first, remaining_max>::value, first,
                                               remaining_max>::type;
    };

    // 'find_ancestors'
    namespace impl {

        template<typename SRCLIST, typename DESTLIST>
        struct find_ancestors
        {

            template<typename B>
            using negation = typename std::integral_constant<bool, !bool(B::value)>::type;

            template<typename T, typename U>
            using cmp = typename std::is_base_of<T, U>::type;
            using most_ancient = typename max<SRCLIST, cmp>::type;

            template<typename T>
            using not_most_ancient = typename negation<std::is_same<most_ancient, T>>::type;

            using type =
                    typename find_ancestors<typename filter<SRCLIST, not_most_ancient>::type,
                                            typename push_back<most_ancient, DESTLIST>::type>::type;
        };

        template<typename DESTLIST>
        struct find_ancestors<typelist<>, DESTLIST>
        {
            using type = DESTLIST;
        };

    }// namespace impl

    template<typename TL, typename T>
    struct find_ancestors
    {
        static_assert(is_typelist<TL>::value, "The first parameter is not a typelist");

        template<typename U>
        using base_of_T = typename std::is_base_of<U, T>::type;
        using src_list = typename filter<TL, base_of_T>::type;
        using type = typename impl::find_ancestors<src_list, typelist<>>::type;
    };

}// namespace tmp

using namespace tmp;

template<typename TL>
struct hierarchy_iterator
{
    static_assert(is_typelist<TL>::value, "Not a typelist");
    inline static void exec(void *_p) {
        using target_t = typename pop_front<TL>::type;
        if (auto ptr = static_cast<target_t *>(_p)) {
            printf("%s\n", typeid(typename at<TL, 0>::type).name());
            // LOG(INFO) << "hierarchy_iterator : " << typeid(typename at<TL, 0>::type).name();
            hierarchy_iterator<target_t>::exec(_p);
        }
    }
};

template<>
struct hierarchy_iterator<typelist<>>
{
    inline static void exec(void *) {}
};

//####################################################################################
//
// Description:     Reflect, C++ 11 Reflection Library
// Author:          Stephens Nunnally and Scidian Software
// License:         Distributed under the MIT License
// Source(s):       https://github.com/stevinz/reflect
//
// Copyright (c) 2021 Stephens Nunnally and Scidian Software
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
//####################################################################################
//
// INSTALLATION:
// - Copy 'reflect.h' to project
//
// - In ONE cpp file, define REGISTER_REFLECTION:
//      #define REGISTER_REFLECTION
//      #include "reflect.h"
//      #include "my_struct_1.h"
//      #include "my_struct_2.h"
//      #include etc...
//
// - Classes / Structs should be simple aggregate types (standard layout)
//      - No private or protected non-static data members
//      - No user-declared / user-provided constructors
//      - No virtual member functions
//      - No default member initializers (invalid in C++11, okay in C++14 and higher)
//      - See (https://en.cppreference.com/w/cpp/types/is_standard_layout) for more info
//
// - BEFORE using reflection, make one call to 'InitializeReflection()'
//
//####################################################################################
//
// USAGE:
//      EXAMPLE CLASS HEADER:
//      start header...
//
//          #include "reflect.h"
//          struct Transform2D {
//              int width;
//              int height;
//              std::vector <double> position;
//              REFLECT();
//          }
//          #ifdef REGISTER_REFLECTION
//              REFLECT_CLASS(Transform2D)
//              REFLECT_MEMBER(width)
//              REFLECT_MEMBER(height)
//              REFLECT_MEMBER(position)
//              REFLECT_END(Transform2D)
//          #endif
//
//      ...end header
//
//####################################################################################
//
// IN CODE:
//      ...
//      Transform2D t { };
//          t.width =    100;
//          t.height =   100;
//          t.position = std::vector<double>({1.0, 2.0, 3.0});
//      ...
//
//      TYPEDATA OBJECT
//      ----------------
//      - Class TypeData
//          TypeData data = ClassData<Transform2D>();           // By class type
//          TypeData data = ClassData(t);                       // By class instance
//          TypeData data = ClassData(type_hash);               // By class type hash
//          TypeData data = ClassData("Transform2D");           // By class name
//
//      - Member TypeData
//          TypeData data = MemberData<Transform2D>(0);         // By class type, member index
//          TypeData data = MemberData<Transform2D>("width");   // By class type, member name
//          TypeData data = MemberData(t, 0);                   // By class instance, member index
//          TypeData data = MemberData(t, "width");             // By class instance, member name
//          TypeData data = MemberData(type_hash, 0);           // By class type hash, member index
//          TypeData data = MemberData(type_hash, "width");     // By class type hash, member name
//
//
//      GET / SET MEMBER VARIABLE
//      -------------------------
//      Use the ClassMember<member_type>(class_instance, member_data) function to return
//      a reference to a member variable. This function requires the return type, a class
//      instance (can be void* or class type), and a member variable TypeData object.
//      Before calling ClassMember<>(), member variable type can be checked by comparing to
//      types using helper function TypeHashID<type_to_check>().
//
//      - Member Variable by Index
//          TypeData member = MemberData(t, 0);
//          if (member.type_hash == TypeHashID<int>()) {
//              // Create reference to member
//              int& width = ClassMember<int>(&t, member);
//              // Can now set member variable directly
//              width = 120;
//          }
//
//      - Member Variable by Name
//          TypeData member = MemberData(t, "position");
//          if (member.type_hash == TypeHashID<std::vector<double>>()) {
//              // Create reference to member
//              std::vector<double>& position = ClassMember<std::vector<double>>(&t, member);
//              // Can now set member variable directly
//              position = { 2.0, 4.0, 6.0 };
//          }
//
//      - Iterating Members / Properties
//          int member_count = ClassData("Transform2D").member_count;
//          for (int index = 0; index < member_count; ++index) {
//              TypeData member = MemberData(t, index);
//              std::cout << " Index: " << member.index << ", ";
//              std::cout << " Name: " << member.name << ",";
//              std::cout << " Value: ";
//              if (member.type_hash == TypeHashID<int>()) {
//                  std::cout << ClassMember<int>(&t, member);
//              } else if (member.type_hash == TypeHashID<std::vector<double>>()) {
//                  std::cout << ClassMember<std::vector<double>>(&t, member)[0];
//              }
//          }
//
//      - Data from Unknown Class Type
//          ...
//          TypeHash saved_hash = ClassData(t).type_hash;
//          void* class_pointer = (void*)(&t);
//          ...
//          TypeData member = MemberData(saved_hash, 1);
//          if (member.type_hash == TypeHashID<int>) {
//              std::cout << GetValue<int>(class_pointer, member);
//          }
//
//
//      USER META DATA
//      --------------
//      Meta data can by stored as std::string within a class or member type. Set user meta data
//      at compile time using CLASS_META_DATA and MEMBER_META_DATA in the class header file during
//      registration with (int, string) or (sting, string) pairs:
//
//          REFLECT_CLASS(Transform2D)
//              CLASS_META_DATA(META_DATA_DESCRIPTION, "Describes object in 2D space.")
//              CLASS_META_DATA("icon", "data/assets/transform.png")
//          REFLECT_MEMBER(width)
//              MEMBER_META_DATA(META_DATA_DESCRIPTION, "Width of this object.")
//
//      To get / set meta data at runtime, pass a TypeData object (class or member, this can be
//      retrieved many different ways as shown earlier) to the meta data functions. When setting or
//      retrieving meta data, the TypeData MUST BE PASSED BY REFERENCE!
//
//      - Get Reference to TypeData
//          TypeData& type_data = ClassData<Transform2D>();             // From class...
//          TypeData& type_data = MemberData<Transform2D>("width");     // or from member variable
//
//      - Get meta data
//          std::string description = GetClassMeta(type_data, META_DATA_DESCRIPTION);
//          std::string icon_file   = GetClassMeta(type_data, "icon");
//
//      - Set meta data
//          SetClassMeta(type_data, META_DATA_DESCRIPTION, description);
//          SetClassMeta(type_data, "icon", icon_file);
//
//
//
//####################################################################################
//
//      Visit (https://github.com/stevinz/reflect) for more detailed instructions
//
//####################################################################################
#ifndef SCID_REFLECT_H
#define SCID_REFLECT_H

// Includes
#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

//####################################################################################
//##    Sample Meta Data Enum
//############################
enum Meta_Data {
    META_DATA_DESCRIPTION,
    META_DATA_HIDDEN,
    META_DATA_TYPE,
    META_DATA_COLOR,
    META_DATA_ICON,
    META_DATA_TOOLTIP,
    // etc...
};

//####################################################################################
//##    Type Definitions
//############################
using TypeHash = size_t;                             // This comes from typeid().hash_code()
using Functions = std::vector<std::function<void()>>;// List of functions
using IntMap = std::unordered_map<int, std::string>; // Meta data int key map
using StringMap = std::map<std::string, std::string>;// Meta data string key map

//####################################################################################
//##    Class / Member Type Data
//############################
struct TypeData
{
    std::string name{"unknown"}; // Actual struct / class / member variable name
    std::string title{"unknown"};// Pretty (capitalized, spaced) name for displaying in gui
    TypeHash type_hash{0};       // Underlying typeid().hash_code of actual type
    IntMap meta_int_map{};       // Map to hold user meta data by int key
    StringMap meta_string_map{}; // Map to hold user meta data by string key
    // For Class Data
    int member_count{0};// Number of registered member variables of class
    // For Member Data
    int index{-1}; // Index of member variable within parent class / struct
    int offset{0}; // Char* offset of member variable within parent class / struct
    size_t size{0};// Size of actual type of member variable
};

// Empty TypeData to return by reference on GetTypeData() fail
static TypeData unknown_type{};

//####################################################################################
//##    SnReflect
//##        Singleton to hold Class / Member reflection and meta data
//############################
class SnReflect {
public:
    std::unordered_map<TypeHash, TypeData> classes{};// Holds data about classes / structs
    std::unordered_map<TypeHash, std::map<int, TypeData>>
            members{};// Holds data about member variables (of classes)

public:
    void AddClass(TypeData class_data) {
        assert(class_data.type_hash != 0 && "Class type hash is 0, error in registration?");
        classes[class_data.type_hash] = class_data;
    }
    void AddMember(TypeData class_data, TypeData member_data) {
        assert(class_data.type_hash != 0 && "Class type hash is 0, error in registration?");
        assert(classes.find(class_data.type_hash) != classes.end() &&
               "Class never registered with AddClass before calling AddMember!");
        members[class_data.type_hash][member_data.offset] = member_data;
        classes[class_data.type_hash].member_count = members[class_data.type_hash].size();
    }
};

//####################################################################################
//##    Global Variable Declarations
//############################
extern std::shared_ptr<SnReflect> g_reflect;// Meta data singleton
extern Functions g_register_list;           // Keeps list of registration functions

//####################################################################################
//##    General Functions
//############################
void InitializeReflection();// Creates SnReflect instance and registers classes and member variables
void CreateTitle(std::string &name);// Create nice display name from class / member variable names
void RegisterClass(TypeData class_data);                       // Update class TypeData
void RegisterMember(TypeData class_data, TypeData member_data);// Update member TypeData

// TypeHash helper function
template<typename T>
TypeHash TypeHashID() {
    return typeid(T).hash_code();
}

// Meta data
void SetMetaData(TypeData &type_data, int key, std::string data);
void SetMetaData(TypeData &type_data, std::string key, std::string data);
std::string GetMetaData(TypeData &type_data, int key);
std::string GetMetaData(TypeData &type_data, std::string key);

//####################################################################################
//##    Class / Member Registration
//############################
// Template wrapper to register type information with SnReflect from header files
template<typename T>
void InitiateClass(){};

// Call this to register class / struct type with reflection / meta data system
template<typename ClassType>
void RegisterClass(TypeData class_data) {
    //assert(std::is_standard_layout<ClassType>() && "Class is not standard layout!!");
    g_reflect->AddClass(class_data);
}

// Call this to register member variable with reflection / meta data system
template<typename MemberType>
void RegisterMember(TypeData class_data, TypeData member_data) {
    g_reflect->AddMember(class_data, member_data);
}

//####################################################################################
//##    Reflection TypeData Fetching
//############################
// #################### Class Data Fetching ####################
// Class TypeData fetching by actual class type
template<typename T>
TypeData &ClassData() {
    TypeHash class_hash = typeid(T).hash_code();
    if (g_reflect->classes.find(class_hash) != g_reflect->classes.end()) {
        return g_reflect->classes[class_hash];
    } else {
        return unknown_type;
    }
}
// Class TypeData fetching from passed in class instance
template<typename T>
TypeData &ClassData(T &class_instance) {
    return ClassData<T>();
}
// Class TypeData fetching from passed in class TypeHash
TypeData &ClassData(TypeHash class_hash);
// Class TypeData fetching from passed in class name
TypeData &ClassData(std::string class_name);
TypeData &ClassData(const char *class_name);

// #################### Member Data Fetching ####################
// -------------------------    By Index  -------------------------
// Member TypeData fetching by member variable index and class TypeHash
TypeData &MemberData(TypeHash class_hash, int member_index);
// Member TypeData fetching by member variable index and class name
template<typename T>
TypeData &MemberData(int member_index) {
    return MemberData(TypeHashID<T>(), member_index);
}
// Member TypeData fetching by member variable index and class instance
template<typename T>
TypeData &MemberData(T &class_instance, int member_index) {
    return MemberData<T>(member_index);
}

// -------------------------    By Name  -------------------------
// Member TypeData fetching by member variable Name and class TypeHash
TypeData &MemberData(TypeHash class_hash, std::string member_name);
// Member TypeData fetching by member variable Name and class name
template<typename T>
TypeData &MemberData(std::string member_name) {
    return MemberData(TypeHashID<T>(), member_name);
}
// Member TypeData fetching by member variable name and class instance
template<typename T>
TypeData &MemberData(T &class_instance, std::string member_name) {
    return MemberData<T>(member_name);
}

// #################### Member Variable Fetching ####################
// NOTES:
//  Internal Casting
//      /* Casting from void*, not fully standardized across compilers? */
//      SnVec3 rotation = *(SnVec3*)(class_ptr + member_data.offset);
//  Memcpy
//      SnVec3 value;
//      memcpy(&value, class_ptr + member_data.offset, member_data.size);
//  C++ Member Pointer
//      static constexpr auto offset_rotation = &Transform2D::rotation;
//      SnVec3 rotation = ((&et)->*off_rot);
template<typename ReturnType>
ReturnType &ClassMember(void *class_ptr, TypeData &member_data) {
    assert(member_data.name != "unknown" && "Could not find member variable!");
    assert(member_data.type_hash == TypeHashID<ReturnType>() &&
           "Did not request correct return type!");
    return *(reinterpret_cast<ReturnType *>(((char *) (class_ptr)) + member_data.offset));
}

//####################################################################################
//##    Macros for Reflection Registration
//####################################################################################
// Static variable added to class allows registration function to be added to list of classes to be registered
#define REFLECT()                                                                                  \
    static bool reflection;                                                                        \
    static bool initReflection();

// Define Registration Function
#define REFLECT_CLASS(TYPE)                                                                        \
    template<>                                                                                     \
    void InitiateClass<TYPE>() {                                                                   \
        using T = TYPE;                                                                            \
        TypeData class_data{};                                                                     \
        class_data.name = #TYPE;                                                                   \
        class_data.type_hash = typeid(TYPE).hash_code();                                           \
        class_data.title = #TYPE;                                                                  \
        CreateTitle(class_data.title);                                                             \
        RegisterClass<T>(class_data);                                                              \
        int member_index = -1;                                                                     \
        std::unordered_map<int, TypeData> mbrs{};

// Meta data functions
#define CLASS_META_TITLE(STRING)                                                                   \
    class_data.title = #STRING;                                                                    \
    RegisterClass(class_data);
#define CLASS_META_DATA(KEY, VALUE)                                                                \
    SetMetaData(class_data, KEY, VALUE);                                                           \
    RegisterClass(class_data);

// Member Registration
#define REFLECT_MEMBER(MEMBER)                                                                     \
    member_index++;                                                                                \
    mbrs[member_index] = TypeData();                                                               \
    mbrs[member_index].name = #MEMBER;                                                             \
    mbrs[member_index].index = member_index;                                                       \
    mbrs[member_index].type_hash = typeid(T::MEMBER).hash_code();                                  \
    mbrs[member_index].offset = offsetof(T, MEMBER);                                               \
    mbrs[member_index].size = sizeof(T::MEMBER);                                                   \
    mbrs[member_index].title = #MEMBER;                                                            \
    CreateTitle(mbrs[member_index].title);                                                         \
    RegisterMember<decltype(T::MEMBER)>(class_data, mbrs[member_index]);

// Meta data functions
#define MEMBER_META_TITLE(STRING)                                                                  \
    mbrs[member_index].title = #STRING;                                                            \
    RegisterMember(class_data, mbrs[member_index]);
#define MEMBER_META_DATA(KEY, VALUE)                                                               \
    SetMetaData(mbrs[member_index], KEY, VALUE);                                                   \
    RegisterMember(class_data, mbrs[member_index]);

// Static definitions add registration function to list of classes to be registered
#define REFLECT_END(TYPE)                                                                          \
    }                                                                                              \
    bool TYPE::reflection{initReflection()};                                                       \
    bool TYPE::initReflection() {                                                                  \
        g_register_list.push_back(std::bind(&InitiateClass<TYPE>));                                \
        return true;                                                                               \
    }


#endif// SCID_REFLECT_H

#include <functional>
#include <type_traits>
#include <utility>

namespace Meta {

    namespace detail {
        // 构造一个可以隐式转换为任意类型的类型
        template<class T>
        struct any_converter
        {
            // 不能convert至自身
            template<class U, class = typename std::enable_if<
                                      !std::is_same<typename std::decay<T>::type,
                                                    typename std::decay<U>::type>::value>::type>
            operator U() const noexcept;
        };

        template<class T, std::size_t I>
        struct any_converter_tagged : any_converter<T>
        {
        };

        // 判断T是否可以使用Args...进行聚合初始化：T{ std::declval<Args>()... }
        template<class T, class... Args>
        constexpr auto is_aggregate_constructible_impl(T &&, Args &&...args)
                -> decltype(T{{args}...}, std::true_type());
        // 多加一个重载可以去掉讨厌的clang warning
        template<class T, class Arg>
        constexpr auto is_aggregate_constructible_impl(T &&, Arg &&args)
                -> decltype(T{args}, std::true_type());
        // 这个函数千万别改成模板函数否则会死机的！
        constexpr auto is_aggregate_constructible_impl(...) -> std::false_type;

        template<class T, class... Args>
        struct is_aggregate_constructible
            : decltype(is_aggregate_constructible_impl(std::declval<T>(), std::declval<Args>()...))
        {
        };

        template<class T, class Seq>
        struct is_aggregate_constructible_with_n_args;
        template<class T, std::size_t... I>
        struct is_aggregate_constructible_with_n_args<T, std::index_sequence<I...>>
            : is_aggregate_constructible<T, any_converter_tagged<T, I>...>
        {
        };
        // 这里添加一个元函数是用来支持沙雕GCC的，不知道为什么GCC里面变长模板参数不能作为嵌套的实参展开……
        template<class T, std::size_t N>
        struct is_aggregate_constructible_with_n_args_ex
            : is_aggregate_constructible_with_n_args<T, std::make_index_sequence<N>>
        {
        };

        // （原）线性查找法
        template<class T, class Seq = std::make_index_sequence<sizeof(T)>>
        struct struct_member_count_impl1;
        template<class T, std::size_t... I>
        struct struct_member_count_impl1<T, std::index_sequence<I...>>
            : std::integral_constant<
                      std::size_t,
                      (... + !!(is_aggregate_constructible_with_n_args_ex<T, I + 1>::value))>
        {
        };

        template<class B, class T, class U>
        struct lazy_conditional : lazy_conditional<typename B::type, T, U>
        {
        };
        template<class T, class U>
        struct lazy_conditional<std::true_type, T, U> : T
        {
        };
        template<class T, class U>
        struct lazy_conditional<std::false_type, T, U> : U
        {
        };

        // 二分查找法
        template<class T, class Seq = std::index_sequence<0>>
        struct struct_member_count_impl2;
        template<class T, std::size_t... I>
        struct struct_member_count_impl2<T, std::index_sequence<I...>>
            : lazy_conditional<
                      std::conjunction<is_aggregate_constructible_with_n_args_ex<T, I + 1>...>,
                      struct_member_count_impl2<T,
                                                std::index_sequence<I..., (I + sizeof...(I))...>>,
                      std::integral_constant<std::size_t,
                                             (... + !!(is_aggregate_constructible_with_n_args_ex<
                                                            T, I + 1>::value))>>::type
        {
        };

    }// namespace detail

    template<class T>
    struct StructMemberCount : detail::struct_member_count_impl2<T>
    {
    };

#include "PreprocesserFlat.hpp"

    namespace detail {

#define APPLYER_DEF(N)                                                                             \
    template<class T, class ApplyFunc>                                                             \
    auto StructApply_impl(T &&my_struct, ApplyFunc f, std::integral_constant<std::size_t, N>) {    \
        auto &&[ENUM_PARAMS(x, N)] = std::forward<T>(my_struct);                                   \
        return std::invoke(f, ENUM_PARAMS(x, N));                                                  \
    }

        ENUM_FOR_EACH(APPLYER_DEF, 128)
#undef APPLYER_DEF
    }// namespace detail

    // StructApply : 把结构体解包为变长参数调用可调用对象f
    template<class T, class ApplyFunc>
    auto StructApply(T &&my_struct, ApplyFunc f) {
        return detail::StructApply_impl(std::forward<T>(my_struct), f,
                                        StructMemberCount<typename std::decay<T>::type>());
    }

    // StructTransformMeta : 把结构体各成员的类型作为变长参数调用元函数MetaFunc
    template<class T, template<class...> class MetaFunc>
    struct StructTransformMeta
    {
        struct FakeApplyer
        {
            template<class... Args>
            auto operator()(Args... args) -> MetaFunc<decltype(args)...>;
        };
        using type = decltype(StructApply(std::declval<T>(), FakeApplyer()));
    };
}// namespace Meta

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace reflect {

    //--------------------------------------------------------
    // Base class of all type descriptors
    //--------------------------------------------------------

    struct TypeDescriptor
    {
        const char *name;
        size_t size;

        TypeDescriptor(const char *name, size_t size) : name{name}, size{size} {}
        virtual ~TypeDescriptor() {}
        virtual std::string getFullName() const { return name; }
        virtual void dump(const void *obj, int indentLevel = 0) const = 0;
    };

    //--------------------------------------------------------
    // Finding type descriptors
    //--------------------------------------------------------

    // Declare the function template that handles primitive types such as int, std::string, etc.:
    template<typename T>
    TypeDescriptor *getPrimitiveDescriptor();

    // A helper class to find TypeDescriptors in different ways:
    struct DefaultResolver
    {
        template<typename T>
        static char func(decltype(&T::Reflection));
        template<typename T>
        static int func(...);
        template<typename T>
        struct IsReflected
        {
            enum {
                value = (sizeof(func<T>(nullptr)) == sizeof(char))
            };
        };

        // This version is called if T has a static member named "Reflection":
        template<typename T, typename std::enable_if<IsReflected<T>::value, int>::type = 0>
        static TypeDescriptor *get() {
            return &T::Reflection;
        }

        // This version is called otherwise:
        template<typename T, typename std::enable_if<!IsReflected<T>::value, int>::type = 0>
        static TypeDescriptor *get() {
            return getPrimitiveDescriptor<T>();
        }
    };

    // This is the primary class template for finding all TypeDescriptors:
    template<typename T>
    struct TypeResolver
    {
        static TypeDescriptor *get() { return DefaultResolver::get<T>(); }
    };

    //--------------------------------------------------------
    // Type descriptors for user-defined structs/classes
    //--------------------------------------------------------

    struct TypeDescriptor_Struct : TypeDescriptor
    {
        struct Member
        {
            const char *name;
            size_t offset;
            TypeDescriptor *type;
        };

        std::vector<Member> members;

        TypeDescriptor_Struct(void (*init)(TypeDescriptor_Struct *)) : TypeDescriptor{nullptr, 0} {
            init(this);
        }
        TypeDescriptor_Struct(const char *name, size_t size,
                              const std::initializer_list<Member> &init)
            : TypeDescriptor{nullptr, 0}, members{init} {}
        virtual void dump(const void *obj, int indentLevel) const override {
            std::cout << name << " {" << std::endl;
            for (const Member &member: members) {
                std::cout << std::string(4 * (indentLevel + 1), ' ') << member.name << " = ";
                member.type->dump((char *) obj + member.offset, indentLevel + 1);
                std::cout << std::endl;
            }
            std::cout << std::string(4 * indentLevel, ' ') << "}";
        }
    };

#define REFLECT_STRUCT()                                                                           \
    friend struct reflect::DefaultResolver;                                                        \
    static reflect::TypeDescriptor_Struct Reflection;                                              \
    static void initReflection(reflect::TypeDescriptor_Struct *);

#define REFLECT_STRUCT_BEGIN(type)                                                                 \
    reflect::TypeDescriptor_Struct type::Reflection{type::initReflection};                         \
    void type::initReflection(reflect::TypeDescriptor_Struct *typeDesc) {                          \
        using T = type;                                                                            \
        typeDesc->name = #type;                                                                    \
        typeDesc->size = sizeof(T);                                                                \
        typeDesc->members = {

#define REFLECT_STRUCT_MEMBER(name)                                                                \
    {#name, offsetof(T, name), reflect::TypeResolver<decltype(T::name)>::get()},

#define REFLECT_STRUCT_END()                                                                       \
    }                                                                                              \
    ;                                                                                              \
    }

    //--------------------------------------------------------
    // Type descriptors for std::vector
    //--------------------------------------------------------

    struct TypeDescriptor_StdVector : TypeDescriptor
    {
        TypeDescriptor *itemType;
        size_t (*getSize)(const void *);
        const void *(*getItem)(const void *, size_t);

        template<typename ItemType>
        TypeDescriptor_StdVector(ItemType *)
            : TypeDescriptor{"std::vector<>", sizeof(std::vector<ItemType>)},
              itemType{TypeResolver<ItemType>::get()} {
            getSize = [](const void *vecPtr) -> size_t {
                const auto &vec = *(const std::vector<ItemType> *) vecPtr;
                return vec.size();
            };
            getItem = [](const void *vecPtr, size_t index) -> const void * {
                const auto &vec = *(const std::vector<ItemType> *) vecPtr;
                return &vec[index];
            };
        }
        virtual std::string getFullName() const override {
            return std::string("std::vector<") + itemType->getFullName() + ">";
        }
        virtual void dump(const void *obj, int indentLevel) const override {
            size_t numItems = getSize(obj);
            std::cout << getFullName();
            if (numItems == 0) {
                std::cout << "{}";
            } else {
                std::cout << "{" << std::endl;
                for (size_t index = 0; index < numItems; index++) {
                    std::cout << std::string(4 * (indentLevel + 1), ' ') << "[" << index << "] ";
                    itemType->dump(getItem(obj, index), indentLevel + 1);
                    std::cout << std::endl;
                }
                std::cout << std::string(4 * indentLevel, ' ') << "}";
            }
        }
    };

    // Partially specialize TypeResolver<> for std::vectors:
    template<typename T>
    class TypeResolver<std::vector<T>> {
    public:
        static TypeDescriptor *get() {
            static TypeDescriptor_StdVector typeDesc{(T *) nullptr};
            return &typeDesc;
        }
    };

}// namespace reflect

#endif