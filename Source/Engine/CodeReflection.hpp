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

//####################################################################################
//####################################################################################
//####################################################################################
//##    BEGIN REFLECT IMPLEMENTATION
//####################################################################################
//####################################################################################
//####################################################################################
#ifdef REGISTER_REFLECTION

// Gloabls
std::shared_ptr<SnReflect> g_reflect{nullptr};// Meta data singleton
Functions g_register_list{};                  // Keeps list of registration functions

// ########## General Registration ##########
// Initializes global reflection object, registers classes with reflection system
void InitializeReflection() {
    // Create Singleton
    g_reflect = std::make_shared<SnReflect>();

    // Register Structs / Classes
    for (int func = 0; func < g_register_list.size(); ++func) { g_register_list[func](); }
    g_register_list.clear();// Clean up
}

// Used in registration macros to automatically create nice display name from class / member variable names
void CreateTitle(std::string &name) {
    // Replace underscores, capitalize first letters
    std::replace(name.begin(), name.end(), '_', ' ');
    name[0] = toupper(name[0]);
    for (int c = 1; c < name.length(); c++) {
        if (name[c - 1] == ' ') name[c] = toupper(name[c]);
    }

    // Add spaces to seperate words
    std::string title = "";
    title += name[0];
    for (int c = 1; c < name.length(); c++) {
        if (islower(name[c - 1]) && isupper(name[c])) {
            title += std::string(" ");
        } else if ((isalpha(name[c - 1]) && isdigit(name[c]))) {
            title += std::string(" ");
        }
        title += name[c];
    }
    name = title;
}

// ########## Class / Member Registration ##########
// Update class TypeData
void RegisterClass(TypeData class_data) { g_reflect->AddClass(class_data); }

// Update member TypeData
void RegisterMember(TypeData class_data, TypeData member_data) {
    g_reflect->AddMember(class_data, member_data);
}

//####################################################################################
//##    TypeData Fetching
//####################################################################################
// ########## Class Data Fetching ##########
// Class TypeData fetching from passed in class TypeHash
TypeData &ClassData(TypeHash class_hash) {
    for (auto &pair: g_reflect->classes) {
        if (pair.first == class_hash) return pair.second;
    }
    return unknown_type;
}
// Class TypeData fetching from passed in class name
TypeData &ClassData(std::string class_name) {
    for (auto &pair: g_reflect->classes) {
        if (pair.second.name == class_name) return pair.second;
    }
    return unknown_type;
}
// Class TypeData fetching from passed in class name
TypeData &ClassData(const char *class_name) { return ClassData(std::string(class_name)); }

// ########## Member Data Fetching ##########
// Member TypeData fetching by member variable index and class TypeHash
TypeData &MemberData(TypeHash class_hash, int member_index) {
    int count = 0;
    for (auto &member: g_reflect->members[class_hash]) {
        if (count == member_index) return member.second;
        ++count;
    }
    return unknown_type;
}
// Member TypeData fetching by member variable name and class TypeHash
TypeData &MemberData(TypeHash class_hash, std::string member_name) {
    for (auto &member: g_reflect->members[class_hash]) {
        if (member.second.name == member_name) return member.second;
    }
    return unknown_type;
}

//####################################################################################
//##    Meta Data (User Info)
//####################################################################################
void SetMetaData(TypeData &type_data, int key, std::string data) {
    if (type_data.type_hash != 0) type_data.meta_int_map[key] = data;
}
void SetMetaData(TypeData &type_data, std::string key, std::string data) {
    if (type_data.type_hash != 0) type_data.meta_string_map[key] = data;
}
std::string GetMetaData(TypeData &type_data, int key) {
    if (type_data.type_hash != 0) {
        if (type_data.meta_int_map.find(key) != type_data.meta_int_map.end())
            return type_data.meta_int_map[key];
    }
    return "";
}
std::string GetMetaData(TypeData &type_data, std::string key) {
    if (type_data.type_hash != 0) {
        if (type_data.meta_string_map.find(key) != type_data.meta_string_map.end())
            return type_data.meta_string_map[key];
    }
    return "";
}

#endif// REGISTER_REFLECTION
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

    // 横向迭代专用，ENUM_PARAMS(x, 3) => x1, x2, x3

#define ENUM_PARAMS_0(x)
#define ENUM_PARAMS_1(x) x##1
#define ENUM_PARAMS_2(x) ENUM_PARAMS_1(x), x##2
#define ENUM_PARAMS_3(x) ENUM_PARAMS_2(x), x##3
#define ENUM_PARAMS_4(x) ENUM_PARAMS_3(x), x##4
#define ENUM_PARAMS_5(x) ENUM_PARAMS_4(x), x##5
#define ENUM_PARAMS_6(x) ENUM_PARAMS_5(x), x##6
#define ENUM_PARAMS_7(x) ENUM_PARAMS_6(x), x##7
#define ENUM_PARAMS_8(x) ENUM_PARAMS_7(x), x##8
#define ENUM_PARAMS_9(x) ENUM_PARAMS_8(x), x##9
#define ENUM_PARAMS_10(x) ENUM_PARAMS_9(x), x##10
#define ENUM_PARAMS_11(x) ENUM_PARAMS_10(x), x##11
#define ENUM_PARAMS_12(x) ENUM_PARAMS_11(x), x##12
#define ENUM_PARAMS_13(x) ENUM_PARAMS_12(x), x##13
#define ENUM_PARAMS_14(x) ENUM_PARAMS_13(x), x##14
#define ENUM_PARAMS_15(x) ENUM_PARAMS_14(x), x##15
#define ENUM_PARAMS_16(x) ENUM_PARAMS_15(x), x##16
#define ENUM_PARAMS_17(x) ENUM_PARAMS_16(x), x##17
#define ENUM_PARAMS_18(x) ENUM_PARAMS_17(x), x##18
#define ENUM_PARAMS_19(x) ENUM_PARAMS_18(x), x##19
#define ENUM_PARAMS_20(x) ENUM_PARAMS_19(x), x##20
#define ENUM_PARAMS_21(x) ENUM_PARAMS_20(x), x##21
#define ENUM_PARAMS_22(x) ENUM_PARAMS_21(x), x##22
#define ENUM_PARAMS_23(x) ENUM_PARAMS_22(x), x##23
#define ENUM_PARAMS_24(x) ENUM_PARAMS_23(x), x##24
#define ENUM_PARAMS_25(x) ENUM_PARAMS_24(x), x##25
#define ENUM_PARAMS_26(x) ENUM_PARAMS_25(x), x##26
#define ENUM_PARAMS_27(x) ENUM_PARAMS_26(x), x##27
#define ENUM_PARAMS_28(x) ENUM_PARAMS_27(x), x##28
#define ENUM_PARAMS_29(x) ENUM_PARAMS_28(x), x##29
#define ENUM_PARAMS_30(x) ENUM_PARAMS_29(x), x##30
#define ENUM_PARAMS_31(x) ENUM_PARAMS_30(x), x##31
#define ENUM_PARAMS_32(x) ENUM_PARAMS_31(x), x##32
#define ENUM_PARAMS_33(x) ENUM_PARAMS_32(x), x##33
#define ENUM_PARAMS_34(x) ENUM_PARAMS_33(x), x##34
#define ENUM_PARAMS_35(x) ENUM_PARAMS_34(x), x##35
#define ENUM_PARAMS_36(x) ENUM_PARAMS_35(x), x##36
#define ENUM_PARAMS_37(x) ENUM_PARAMS_36(x), x##37
#define ENUM_PARAMS_38(x) ENUM_PARAMS_37(x), x##38
#define ENUM_PARAMS_39(x) ENUM_PARAMS_38(x), x##39
#define ENUM_PARAMS_40(x) ENUM_PARAMS_39(x), x##40
#define ENUM_PARAMS_41(x) ENUM_PARAMS_40(x), x##41
#define ENUM_PARAMS_42(x) ENUM_PARAMS_41(x), x##42
#define ENUM_PARAMS_43(x) ENUM_PARAMS_42(x), x##43
#define ENUM_PARAMS_44(x) ENUM_PARAMS_43(x), x##44
#define ENUM_PARAMS_45(x) ENUM_PARAMS_44(x), x##45
#define ENUM_PARAMS_46(x) ENUM_PARAMS_45(x), x##46
#define ENUM_PARAMS_47(x) ENUM_PARAMS_46(x), x##47
#define ENUM_PARAMS_48(x) ENUM_PARAMS_47(x), x##48
#define ENUM_PARAMS_49(x) ENUM_PARAMS_48(x), x##49
#define ENUM_PARAMS_50(x) ENUM_PARAMS_49(x), x##50
#define ENUM_PARAMS_51(x) ENUM_PARAMS_50(x), x##51
#define ENUM_PARAMS_52(x) ENUM_PARAMS_51(x), x##52
#define ENUM_PARAMS_53(x) ENUM_PARAMS_52(x), x##53
#define ENUM_PARAMS_54(x) ENUM_PARAMS_53(x), x##54
#define ENUM_PARAMS_55(x) ENUM_PARAMS_54(x), x##55
#define ENUM_PARAMS_56(x) ENUM_PARAMS_55(x), x##56
#define ENUM_PARAMS_57(x) ENUM_PARAMS_56(x), x##57
#define ENUM_PARAMS_58(x) ENUM_PARAMS_57(x), x##58
#define ENUM_PARAMS_59(x) ENUM_PARAMS_58(x), x##59
#define ENUM_PARAMS_60(x) ENUM_PARAMS_59(x), x##60
#define ENUM_PARAMS_61(x) ENUM_PARAMS_60(x), x##61
#define ENUM_PARAMS_62(x) ENUM_PARAMS_61(x), x##62
#define ENUM_PARAMS_63(x) ENUM_PARAMS_62(x), x##63
#define ENUM_PARAMS_64(x) ENUM_PARAMS_63(x), x##64
#define ENUM_PARAMS_65(x) ENUM_PARAMS_64(x), x##65
#define ENUM_PARAMS_66(x) ENUM_PARAMS_65(x), x##66
#define ENUM_PARAMS_67(x) ENUM_PARAMS_66(x), x##67
#define ENUM_PARAMS_68(x) ENUM_PARAMS_67(x), x##68
#define ENUM_PARAMS_69(x) ENUM_PARAMS_68(x), x##69
#define ENUM_PARAMS_70(x) ENUM_PARAMS_69(x), x##70
#define ENUM_PARAMS_71(x) ENUM_PARAMS_70(x), x##71
#define ENUM_PARAMS_72(x) ENUM_PARAMS_71(x), x##72
#define ENUM_PARAMS_73(x) ENUM_PARAMS_72(x), x##73
#define ENUM_PARAMS_74(x) ENUM_PARAMS_73(x), x##74
#define ENUM_PARAMS_75(x) ENUM_PARAMS_74(x), x##75
#define ENUM_PARAMS_76(x) ENUM_PARAMS_75(x), x##76
#define ENUM_PARAMS_77(x) ENUM_PARAMS_76(x), x##77
#define ENUM_PARAMS_78(x) ENUM_PARAMS_77(x), x##78
#define ENUM_PARAMS_79(x) ENUM_PARAMS_78(x), x##79
#define ENUM_PARAMS_80(x) ENUM_PARAMS_79(x), x##80
#define ENUM_PARAMS_81(x) ENUM_PARAMS_80(x), x##81
#define ENUM_PARAMS_82(x) ENUM_PARAMS_81(x), x##82
#define ENUM_PARAMS_83(x) ENUM_PARAMS_82(x), x##83
#define ENUM_PARAMS_84(x) ENUM_PARAMS_83(x), x##84
#define ENUM_PARAMS_85(x) ENUM_PARAMS_84(x), x##85
#define ENUM_PARAMS_86(x) ENUM_PARAMS_85(x), x##86
#define ENUM_PARAMS_87(x) ENUM_PARAMS_86(x), x##87
#define ENUM_PARAMS_88(x) ENUM_PARAMS_87(x), x##88
#define ENUM_PARAMS_89(x) ENUM_PARAMS_88(x), x##89
#define ENUM_PARAMS_90(x) ENUM_PARAMS_89(x), x##90
#define ENUM_PARAMS_91(x) ENUM_PARAMS_90(x), x##91
#define ENUM_PARAMS_92(x) ENUM_PARAMS_91(x), x##92
#define ENUM_PARAMS_93(x) ENUM_PARAMS_92(x), x##93
#define ENUM_PARAMS_94(x) ENUM_PARAMS_93(x), x##94
#define ENUM_PARAMS_95(x) ENUM_PARAMS_94(x), x##95
#define ENUM_PARAMS_96(x) ENUM_PARAMS_95(x), x##96
#define ENUM_PARAMS_97(x) ENUM_PARAMS_96(x), x##97
#define ENUM_PARAMS_98(x) ENUM_PARAMS_97(x), x##98
#define ENUM_PARAMS_99(x) ENUM_PARAMS_98(x), x##99
#define ENUM_PARAMS_100(x) ENUM_PARAMS_99(x), x##100
#define ENUM_PARAMS_101(x) ENUM_PARAMS_100(x), x##101
#define ENUM_PARAMS_102(x) ENUM_PARAMS_101(x), x##102
#define ENUM_PARAMS_103(x) ENUM_PARAMS_102(x), x##103
#define ENUM_PARAMS_104(x) ENUM_PARAMS_103(x), x##104
#define ENUM_PARAMS_105(x) ENUM_PARAMS_104(x), x##105
#define ENUM_PARAMS_106(x) ENUM_PARAMS_105(x), x##106
#define ENUM_PARAMS_107(x) ENUM_PARAMS_106(x), x##107
#define ENUM_PARAMS_108(x) ENUM_PARAMS_107(x), x##108
#define ENUM_PARAMS_109(x) ENUM_PARAMS_108(x), x##109
#define ENUM_PARAMS_110(x) ENUM_PARAMS_109(x), x##110
#define ENUM_PARAMS_111(x) ENUM_PARAMS_110(x), x##111
#define ENUM_PARAMS_112(x) ENUM_PARAMS_111(x), x##112
#define ENUM_PARAMS_113(x) ENUM_PARAMS_112(x), x##113
#define ENUM_PARAMS_114(x) ENUM_PARAMS_113(x), x##114
#define ENUM_PARAMS_115(x) ENUM_PARAMS_114(x), x##115
#define ENUM_PARAMS_116(x) ENUM_PARAMS_115(x), x##116
#define ENUM_PARAMS_117(x) ENUM_PARAMS_116(x), x##117
#define ENUM_PARAMS_118(x) ENUM_PARAMS_117(x), x##118
#define ENUM_PARAMS_119(x) ENUM_PARAMS_118(x), x##119
#define ENUM_PARAMS_120(x) ENUM_PARAMS_119(x), x##120
#define ENUM_PARAMS_121(x) ENUM_PARAMS_120(x), x##121
#define ENUM_PARAMS_122(x) ENUM_PARAMS_121(x), x##122
#define ENUM_PARAMS_123(x) ENUM_PARAMS_122(x), x##123
#define ENUM_PARAMS_124(x) ENUM_PARAMS_123(x), x##124
#define ENUM_PARAMS_125(x) ENUM_PARAMS_124(x), x##125
#define ENUM_PARAMS_126(x) ENUM_PARAMS_125(x), x##126
#define ENUM_PARAMS_127(x) ENUM_PARAMS_126(x), x##127
#define ENUM_PARAMS_128(x) ENUM_PARAMS_127(x), x##128
#define ENUM_PARAMS_129(x) ENUM_PARAMS_128(x), x##129
#define ENUM_PARAMS_130(x) ENUM_PARAMS_129(x), x##130
#define ENUM_PARAMS_131(x) ENUM_PARAMS_130(x), x##131
#define ENUM_PARAMS_132(x) ENUM_PARAMS_131(x), x##132
#define ENUM_PARAMS_133(x) ENUM_PARAMS_132(x), x##133
#define ENUM_PARAMS_134(x) ENUM_PARAMS_133(x), x##134
#define ENUM_PARAMS_135(x) ENUM_PARAMS_134(x), x##135
#define ENUM_PARAMS_136(x) ENUM_PARAMS_135(x), x##136
#define ENUM_PARAMS_137(x) ENUM_PARAMS_136(x), x##137
#define ENUM_PARAMS_138(x) ENUM_PARAMS_137(x), x##138
#define ENUM_PARAMS_139(x) ENUM_PARAMS_138(x), x##139
#define ENUM_PARAMS_140(x) ENUM_PARAMS_139(x), x##140
#define ENUM_PARAMS_141(x) ENUM_PARAMS_140(x), x##141
#define ENUM_PARAMS_142(x) ENUM_PARAMS_141(x), x##142
#define ENUM_PARAMS_143(x) ENUM_PARAMS_142(x), x##143
#define ENUM_PARAMS_144(x) ENUM_PARAMS_143(x), x##144
#define ENUM_PARAMS_145(x) ENUM_PARAMS_144(x), x##145
#define ENUM_PARAMS_146(x) ENUM_PARAMS_145(x), x##146
#define ENUM_PARAMS_147(x) ENUM_PARAMS_146(x), x##147
#define ENUM_PARAMS_148(x) ENUM_PARAMS_147(x), x##148
#define ENUM_PARAMS_149(x) ENUM_PARAMS_148(x), x##149
#define ENUM_PARAMS_150(x) ENUM_PARAMS_149(x), x##150
#define ENUM_PARAMS_151(x) ENUM_PARAMS_150(x), x##151
#define ENUM_PARAMS_152(x) ENUM_PARAMS_151(x), x##152
#define ENUM_PARAMS_153(x) ENUM_PARAMS_152(x), x##153
#define ENUM_PARAMS_154(x) ENUM_PARAMS_153(x), x##154
#define ENUM_PARAMS_155(x) ENUM_PARAMS_154(x), x##155
#define ENUM_PARAMS_156(x) ENUM_PARAMS_155(x), x##156
#define ENUM_PARAMS_157(x) ENUM_PARAMS_156(x), x##157
#define ENUM_PARAMS_158(x) ENUM_PARAMS_157(x), x##158
#define ENUM_PARAMS_159(x) ENUM_PARAMS_158(x), x##159
#define ENUM_PARAMS_160(x) ENUM_PARAMS_159(x), x##160
#define ENUM_PARAMS_161(x) ENUM_PARAMS_160(x), x##161
#define ENUM_PARAMS_162(x) ENUM_PARAMS_161(x), x##162
#define ENUM_PARAMS_163(x) ENUM_PARAMS_162(x), x##163
#define ENUM_PARAMS_164(x) ENUM_PARAMS_163(x), x##164
#define ENUM_PARAMS_165(x) ENUM_PARAMS_164(x), x##165
#define ENUM_PARAMS_166(x) ENUM_PARAMS_165(x), x##166
#define ENUM_PARAMS_167(x) ENUM_PARAMS_166(x), x##167
#define ENUM_PARAMS_168(x) ENUM_PARAMS_167(x), x##168
#define ENUM_PARAMS_169(x) ENUM_PARAMS_168(x), x##169
#define ENUM_PARAMS_170(x) ENUM_PARAMS_169(x), x##170
#define ENUM_PARAMS_171(x) ENUM_PARAMS_170(x), x##171
#define ENUM_PARAMS_172(x) ENUM_PARAMS_171(x), x##172
#define ENUM_PARAMS_173(x) ENUM_PARAMS_172(x), x##173
#define ENUM_PARAMS_174(x) ENUM_PARAMS_173(x), x##174
#define ENUM_PARAMS_175(x) ENUM_PARAMS_174(x), x##175
#define ENUM_PARAMS_176(x) ENUM_PARAMS_175(x), x##176
#define ENUM_PARAMS_177(x) ENUM_PARAMS_176(x), x##177
#define ENUM_PARAMS_178(x) ENUM_PARAMS_177(x), x##178
#define ENUM_PARAMS_179(x) ENUM_PARAMS_178(x), x##179
#define ENUM_PARAMS_180(x) ENUM_PARAMS_179(x), x##180
#define ENUM_PARAMS_181(x) ENUM_PARAMS_180(x), x##181
#define ENUM_PARAMS_182(x) ENUM_PARAMS_181(x), x##182
#define ENUM_PARAMS_183(x) ENUM_PARAMS_182(x), x##183
#define ENUM_PARAMS_184(x) ENUM_PARAMS_183(x), x##184
#define ENUM_PARAMS_185(x) ENUM_PARAMS_184(x), x##185
#define ENUM_PARAMS_186(x) ENUM_PARAMS_185(x), x##186
#define ENUM_PARAMS_187(x) ENUM_PARAMS_186(x), x##187
#define ENUM_PARAMS_188(x) ENUM_PARAMS_187(x), x##188
#define ENUM_PARAMS_189(x) ENUM_PARAMS_188(x), x##189
#define ENUM_PARAMS_190(x) ENUM_PARAMS_189(x), x##190
#define ENUM_PARAMS_191(x) ENUM_PARAMS_190(x), x##191
#define ENUM_PARAMS_192(x) ENUM_PARAMS_191(x), x##192
#define ENUM_PARAMS_193(x) ENUM_PARAMS_192(x), x##193
#define ENUM_PARAMS_194(x) ENUM_PARAMS_193(x), x##194
#define ENUM_PARAMS_195(x) ENUM_PARAMS_194(x), x##195
#define ENUM_PARAMS_196(x) ENUM_PARAMS_195(x), x##196
#define ENUM_PARAMS_197(x) ENUM_PARAMS_196(x), x##197
#define ENUM_PARAMS_198(x) ENUM_PARAMS_197(x), x##198
#define ENUM_PARAMS_199(x) ENUM_PARAMS_198(x), x##199
#define ENUM_PARAMS_200(x) ENUM_PARAMS_199(x), x##200
#define ENUM_PARAMS_201(x) ENUM_PARAMS_200(x), x##201
#define ENUM_PARAMS_202(x) ENUM_PARAMS_201(x), x##202
#define ENUM_PARAMS_203(x) ENUM_PARAMS_202(x), x##203
#define ENUM_PARAMS_204(x) ENUM_PARAMS_203(x), x##204
#define ENUM_PARAMS_205(x) ENUM_PARAMS_204(x), x##205
#define ENUM_PARAMS_206(x) ENUM_PARAMS_205(x), x##206
#define ENUM_PARAMS_207(x) ENUM_PARAMS_206(x), x##207
#define ENUM_PARAMS_208(x) ENUM_PARAMS_207(x), x##208
#define ENUM_PARAMS_209(x) ENUM_PARAMS_208(x), x##209
#define ENUM_PARAMS_210(x) ENUM_PARAMS_209(x), x##210
#define ENUM_PARAMS_211(x) ENUM_PARAMS_210(x), x##211
#define ENUM_PARAMS_212(x) ENUM_PARAMS_211(x), x##212
#define ENUM_PARAMS_213(x) ENUM_PARAMS_212(x), x##213
#define ENUM_PARAMS_214(x) ENUM_PARAMS_213(x), x##214
#define ENUM_PARAMS_215(x) ENUM_PARAMS_214(x), x##215
#define ENUM_PARAMS_216(x) ENUM_PARAMS_215(x), x##216
#define ENUM_PARAMS_217(x) ENUM_PARAMS_216(x), x##217
#define ENUM_PARAMS_218(x) ENUM_PARAMS_217(x), x##218
#define ENUM_PARAMS_219(x) ENUM_PARAMS_218(x), x##219
#define ENUM_PARAMS_220(x) ENUM_PARAMS_219(x), x##220
#define ENUM_PARAMS_221(x) ENUM_PARAMS_220(x), x##221
#define ENUM_PARAMS_222(x) ENUM_PARAMS_221(x), x##222
#define ENUM_PARAMS_223(x) ENUM_PARAMS_222(x), x##223
#define ENUM_PARAMS_224(x) ENUM_PARAMS_223(x), x##224
#define ENUM_PARAMS_225(x) ENUM_PARAMS_224(x), x##225
#define ENUM_PARAMS_226(x) ENUM_PARAMS_225(x), x##226
#define ENUM_PARAMS_227(x) ENUM_PARAMS_226(x), x##227
#define ENUM_PARAMS_228(x) ENUM_PARAMS_227(x), x##228
#define ENUM_PARAMS_229(x) ENUM_PARAMS_228(x), x##229
#define ENUM_PARAMS_230(x) ENUM_PARAMS_229(x), x##230
#define ENUM_PARAMS_231(x) ENUM_PARAMS_230(x), x##231
#define ENUM_PARAMS_232(x) ENUM_PARAMS_231(x), x##232
#define ENUM_PARAMS_233(x) ENUM_PARAMS_232(x), x##233
#define ENUM_PARAMS_234(x) ENUM_PARAMS_233(x), x##234
#define ENUM_PARAMS_235(x) ENUM_PARAMS_234(x), x##235
#define ENUM_PARAMS_236(x) ENUM_PARAMS_235(x), x##236
#define ENUM_PARAMS_237(x) ENUM_PARAMS_236(x), x##237
#define ENUM_PARAMS_238(x) ENUM_PARAMS_237(x), x##238
#define ENUM_PARAMS_239(x) ENUM_PARAMS_238(x), x##239
#define ENUM_PARAMS_240(x) ENUM_PARAMS_239(x), x##240
#define ENUM_PARAMS_241(x) ENUM_PARAMS_240(x), x##241
#define ENUM_PARAMS_242(x) ENUM_PARAMS_241(x), x##242
#define ENUM_PARAMS_243(x) ENUM_PARAMS_242(x), x##243
#define ENUM_PARAMS_244(x) ENUM_PARAMS_243(x), x##244
#define ENUM_PARAMS_245(x) ENUM_PARAMS_244(x), x##245
#define ENUM_PARAMS_246(x) ENUM_PARAMS_245(x), x##246
#define ENUM_PARAMS_247(x) ENUM_PARAMS_246(x), x##247
#define ENUM_PARAMS_248(x) ENUM_PARAMS_247(x), x##248
#define ENUM_PARAMS_249(x) ENUM_PARAMS_248(x), x##249
#define ENUM_PARAMS_250(x) ENUM_PARAMS_249(x), x##250
#define ENUM_PARAMS_251(x) ENUM_PARAMS_250(x), x##251
#define ENUM_PARAMS_252(x) ENUM_PARAMS_251(x), x##252
#define ENUM_PARAMS_253(x) ENUM_PARAMS_252(x), x##253
#define ENUM_PARAMS_254(x) ENUM_PARAMS_253(x), x##254
#define ENUM_PARAMS_255(x) ENUM_PARAMS_254(x), x##255

#define ENUM_PARAMS(x, N) ENUM_PARAMS_##N(x)

    // 纵向迭代专用

#define ENUM_FOR_EACH_0(x)
#define ENUM_FOR_EACH_1(x) ENUM_FOR_EACH_0(x) x(1)
#define ENUM_FOR_EACH_2(x) ENUM_FOR_EACH_1(x) x(2)
#define ENUM_FOR_EACH_3(x) ENUM_FOR_EACH_2(x) x(3)
#define ENUM_FOR_EACH_4(x) ENUM_FOR_EACH_3(x) x(4)
#define ENUM_FOR_EACH_5(x) ENUM_FOR_EACH_4(x) x(5)
#define ENUM_FOR_EACH_6(x) ENUM_FOR_EACH_5(x) x(6)
#define ENUM_FOR_EACH_7(x) ENUM_FOR_EACH_6(x) x(7)
#define ENUM_FOR_EACH_8(x) ENUM_FOR_EACH_7(x) x(8)
#define ENUM_FOR_EACH_9(x) ENUM_FOR_EACH_8(x) x(9)
#define ENUM_FOR_EACH_10(x) ENUM_FOR_EACH_9(x) x(10)
#define ENUM_FOR_EACH_11(x) ENUM_FOR_EACH_10(x) x(11)
#define ENUM_FOR_EACH_12(x) ENUM_FOR_EACH_11(x) x(12)
#define ENUM_FOR_EACH_13(x) ENUM_FOR_EACH_12(x) x(13)
#define ENUM_FOR_EACH_14(x) ENUM_FOR_EACH_13(x) x(14)
#define ENUM_FOR_EACH_15(x) ENUM_FOR_EACH_14(x) x(15)
#define ENUM_FOR_EACH_16(x) ENUM_FOR_EACH_15(x) x(16)
#define ENUM_FOR_EACH_17(x) ENUM_FOR_EACH_16(x) x(17)
#define ENUM_FOR_EACH_18(x) ENUM_FOR_EACH_17(x) x(18)
#define ENUM_FOR_EACH_19(x) ENUM_FOR_EACH_18(x) x(19)
#define ENUM_FOR_EACH_20(x) ENUM_FOR_EACH_19(x) x(20)
#define ENUM_FOR_EACH_21(x) ENUM_FOR_EACH_20(x) x(21)
#define ENUM_FOR_EACH_22(x) ENUM_FOR_EACH_21(x) x(22)
#define ENUM_FOR_EACH_23(x) ENUM_FOR_EACH_22(x) x(23)
#define ENUM_FOR_EACH_24(x) ENUM_FOR_EACH_23(x) x(24)
#define ENUM_FOR_EACH_25(x) ENUM_FOR_EACH_24(x) x(25)
#define ENUM_FOR_EACH_26(x) ENUM_FOR_EACH_25(x) x(26)
#define ENUM_FOR_EACH_27(x) ENUM_FOR_EACH_26(x) x(27)
#define ENUM_FOR_EACH_28(x) ENUM_FOR_EACH_27(x) x(28)
#define ENUM_FOR_EACH_29(x) ENUM_FOR_EACH_28(x) x(29)
#define ENUM_FOR_EACH_30(x) ENUM_FOR_EACH_29(x) x(30)
#define ENUM_FOR_EACH_31(x) ENUM_FOR_EACH_30(x) x(31)
#define ENUM_FOR_EACH_32(x) ENUM_FOR_EACH_31(x) x(32)
#define ENUM_FOR_EACH_33(x) ENUM_FOR_EACH_32(x) x(33)
#define ENUM_FOR_EACH_34(x) ENUM_FOR_EACH_33(x) x(34)
#define ENUM_FOR_EACH_35(x) ENUM_FOR_EACH_34(x) x(35)
#define ENUM_FOR_EACH_36(x) ENUM_FOR_EACH_35(x) x(36)
#define ENUM_FOR_EACH_37(x) ENUM_FOR_EACH_36(x) x(37)
#define ENUM_FOR_EACH_38(x) ENUM_FOR_EACH_37(x) x(38)
#define ENUM_FOR_EACH_39(x) ENUM_FOR_EACH_38(x) x(39)
#define ENUM_FOR_EACH_40(x) ENUM_FOR_EACH_39(x) x(40)
#define ENUM_FOR_EACH_41(x) ENUM_FOR_EACH_40(x) x(41)
#define ENUM_FOR_EACH_42(x) ENUM_FOR_EACH_41(x) x(42)
#define ENUM_FOR_EACH_43(x) ENUM_FOR_EACH_42(x) x(43)
#define ENUM_FOR_EACH_44(x) ENUM_FOR_EACH_43(x) x(44)
#define ENUM_FOR_EACH_45(x) ENUM_FOR_EACH_44(x) x(45)
#define ENUM_FOR_EACH_46(x) ENUM_FOR_EACH_45(x) x(46)
#define ENUM_FOR_EACH_47(x) ENUM_FOR_EACH_46(x) x(47)
#define ENUM_FOR_EACH_48(x) ENUM_FOR_EACH_47(x) x(48)
#define ENUM_FOR_EACH_49(x) ENUM_FOR_EACH_48(x) x(49)
#define ENUM_FOR_EACH_50(x) ENUM_FOR_EACH_49(x) x(50)
#define ENUM_FOR_EACH_51(x) ENUM_FOR_EACH_50(x) x(51)
#define ENUM_FOR_EACH_52(x) ENUM_FOR_EACH_51(x) x(52)
#define ENUM_FOR_EACH_53(x) ENUM_FOR_EACH_52(x) x(53)
#define ENUM_FOR_EACH_54(x) ENUM_FOR_EACH_53(x) x(54)
#define ENUM_FOR_EACH_55(x) ENUM_FOR_EACH_54(x) x(55)
#define ENUM_FOR_EACH_56(x) ENUM_FOR_EACH_55(x) x(56)
#define ENUM_FOR_EACH_57(x) ENUM_FOR_EACH_56(x) x(57)
#define ENUM_FOR_EACH_58(x) ENUM_FOR_EACH_57(x) x(58)
#define ENUM_FOR_EACH_59(x) ENUM_FOR_EACH_58(x) x(59)
#define ENUM_FOR_EACH_60(x) ENUM_FOR_EACH_59(x) x(60)
#define ENUM_FOR_EACH_61(x) ENUM_FOR_EACH_60(x) x(61)
#define ENUM_FOR_EACH_62(x) ENUM_FOR_EACH_61(x) x(62)
#define ENUM_FOR_EACH_63(x) ENUM_FOR_EACH_62(x) x(63)
#define ENUM_FOR_EACH_64(x) ENUM_FOR_EACH_63(x) x(64)
#define ENUM_FOR_EACH_65(x) ENUM_FOR_EACH_64(x) x(65)
#define ENUM_FOR_EACH_66(x) ENUM_FOR_EACH_65(x) x(66)
#define ENUM_FOR_EACH_67(x) ENUM_FOR_EACH_66(x) x(67)
#define ENUM_FOR_EACH_68(x) ENUM_FOR_EACH_67(x) x(68)
#define ENUM_FOR_EACH_69(x) ENUM_FOR_EACH_68(x) x(69)
#define ENUM_FOR_EACH_70(x) ENUM_FOR_EACH_69(x) x(70)
#define ENUM_FOR_EACH_71(x) ENUM_FOR_EACH_70(x) x(71)
#define ENUM_FOR_EACH_72(x) ENUM_FOR_EACH_71(x) x(72)
#define ENUM_FOR_EACH_73(x) ENUM_FOR_EACH_72(x) x(73)
#define ENUM_FOR_EACH_74(x) ENUM_FOR_EACH_73(x) x(74)
#define ENUM_FOR_EACH_75(x) ENUM_FOR_EACH_74(x) x(75)
#define ENUM_FOR_EACH_76(x) ENUM_FOR_EACH_75(x) x(76)
#define ENUM_FOR_EACH_77(x) ENUM_FOR_EACH_76(x) x(77)
#define ENUM_FOR_EACH_78(x) ENUM_FOR_EACH_77(x) x(78)
#define ENUM_FOR_EACH_79(x) ENUM_FOR_EACH_78(x) x(79)
#define ENUM_FOR_EACH_80(x) ENUM_FOR_EACH_79(x) x(80)
#define ENUM_FOR_EACH_81(x) ENUM_FOR_EACH_80(x) x(81)
#define ENUM_FOR_EACH_82(x) ENUM_FOR_EACH_81(x) x(82)
#define ENUM_FOR_EACH_83(x) ENUM_FOR_EACH_82(x) x(83)
#define ENUM_FOR_EACH_84(x) ENUM_FOR_EACH_83(x) x(84)
#define ENUM_FOR_EACH_85(x) ENUM_FOR_EACH_84(x) x(85)
#define ENUM_FOR_EACH_86(x) ENUM_FOR_EACH_85(x) x(86)
#define ENUM_FOR_EACH_87(x) ENUM_FOR_EACH_86(x) x(87)
#define ENUM_FOR_EACH_88(x) ENUM_FOR_EACH_87(x) x(88)
#define ENUM_FOR_EACH_89(x) ENUM_FOR_EACH_88(x) x(89)
#define ENUM_FOR_EACH_90(x) ENUM_FOR_EACH_89(x) x(90)
#define ENUM_FOR_EACH_91(x) ENUM_FOR_EACH_90(x) x(91)
#define ENUM_FOR_EACH_92(x) ENUM_FOR_EACH_91(x) x(92)
#define ENUM_FOR_EACH_93(x) ENUM_FOR_EACH_92(x) x(93)
#define ENUM_FOR_EACH_94(x) ENUM_FOR_EACH_93(x) x(94)
#define ENUM_FOR_EACH_95(x) ENUM_FOR_EACH_94(x) x(95)
#define ENUM_FOR_EACH_96(x) ENUM_FOR_EACH_95(x) x(96)
#define ENUM_FOR_EACH_97(x) ENUM_FOR_EACH_96(x) x(97)
#define ENUM_FOR_EACH_98(x) ENUM_FOR_EACH_97(x) x(98)
#define ENUM_FOR_EACH_99(x) ENUM_FOR_EACH_98(x) x(99)
#define ENUM_FOR_EACH_100(x) ENUM_FOR_EACH_99(x) x(100)
#define ENUM_FOR_EACH_101(x) ENUM_FOR_EACH_100(x) x(101)
#define ENUM_FOR_EACH_102(x) ENUM_FOR_EACH_101(x) x(102)
#define ENUM_FOR_EACH_103(x) ENUM_FOR_EACH_102(x) x(103)
#define ENUM_FOR_EACH_104(x) ENUM_FOR_EACH_103(x) x(104)
#define ENUM_FOR_EACH_105(x) ENUM_FOR_EACH_104(x) x(105)
#define ENUM_FOR_EACH_106(x) ENUM_FOR_EACH_105(x) x(106)
#define ENUM_FOR_EACH_107(x) ENUM_FOR_EACH_106(x) x(107)
#define ENUM_FOR_EACH_108(x) ENUM_FOR_EACH_107(x) x(108)
#define ENUM_FOR_EACH_109(x) ENUM_FOR_EACH_108(x) x(109)
#define ENUM_FOR_EACH_110(x) ENUM_FOR_EACH_109(x) x(110)
#define ENUM_FOR_EACH_111(x) ENUM_FOR_EACH_110(x) x(111)
#define ENUM_FOR_EACH_112(x) ENUM_FOR_EACH_111(x) x(112)
#define ENUM_FOR_EACH_113(x) ENUM_FOR_EACH_112(x) x(113)
#define ENUM_FOR_EACH_114(x) ENUM_FOR_EACH_113(x) x(114)
#define ENUM_FOR_EACH_115(x) ENUM_FOR_EACH_114(x) x(115)
#define ENUM_FOR_EACH_116(x) ENUM_FOR_EACH_115(x) x(116)
#define ENUM_FOR_EACH_117(x) ENUM_FOR_EACH_116(x) x(117)
#define ENUM_FOR_EACH_118(x) ENUM_FOR_EACH_117(x) x(118)
#define ENUM_FOR_EACH_119(x) ENUM_FOR_EACH_118(x) x(119)
#define ENUM_FOR_EACH_120(x) ENUM_FOR_EACH_119(x) x(120)
#define ENUM_FOR_EACH_121(x) ENUM_FOR_EACH_120(x) x(121)
#define ENUM_FOR_EACH_122(x) ENUM_FOR_EACH_121(x) x(122)
#define ENUM_FOR_EACH_123(x) ENUM_FOR_EACH_122(x) x(123)
#define ENUM_FOR_EACH_124(x) ENUM_FOR_EACH_123(x) x(124)
#define ENUM_FOR_EACH_125(x) ENUM_FOR_EACH_124(x) x(125)
#define ENUM_FOR_EACH_126(x) ENUM_FOR_EACH_125(x) x(126)
#define ENUM_FOR_EACH_127(x) ENUM_FOR_EACH_126(x) x(127)
#define ENUM_FOR_EACH_128(x) ENUM_FOR_EACH_127(x) x(128)
#define ENUM_FOR_EACH_129(x) ENUM_FOR_EACH_128(x) x(129)
#define ENUM_FOR_EACH_130(x) ENUM_FOR_EACH_129(x) x(130)
#define ENUM_FOR_EACH_131(x) ENUM_FOR_EACH_130(x) x(131)
#define ENUM_FOR_EACH_132(x) ENUM_FOR_EACH_131(x) x(132)
#define ENUM_FOR_EACH_133(x) ENUM_FOR_EACH_132(x) x(133)
#define ENUM_FOR_EACH_134(x) ENUM_FOR_EACH_133(x) x(134)
#define ENUM_FOR_EACH_135(x) ENUM_FOR_EACH_134(x) x(135)
#define ENUM_FOR_EACH_136(x) ENUM_FOR_EACH_135(x) x(136)
#define ENUM_FOR_EACH_137(x) ENUM_FOR_EACH_136(x) x(137)
#define ENUM_FOR_EACH_138(x) ENUM_FOR_EACH_137(x) x(138)
#define ENUM_FOR_EACH_139(x) ENUM_FOR_EACH_138(x) x(139)
#define ENUM_FOR_EACH_140(x) ENUM_FOR_EACH_139(x) x(140)
#define ENUM_FOR_EACH_141(x) ENUM_FOR_EACH_140(x) x(141)
#define ENUM_FOR_EACH_142(x) ENUM_FOR_EACH_141(x) x(142)
#define ENUM_FOR_EACH_143(x) ENUM_FOR_EACH_142(x) x(143)
#define ENUM_FOR_EACH_144(x) ENUM_FOR_EACH_143(x) x(144)
#define ENUM_FOR_EACH_145(x) ENUM_FOR_EACH_144(x) x(145)
#define ENUM_FOR_EACH_146(x) ENUM_FOR_EACH_145(x) x(146)
#define ENUM_FOR_EACH_147(x) ENUM_FOR_EACH_146(x) x(147)
#define ENUM_FOR_EACH_148(x) ENUM_FOR_EACH_147(x) x(148)
#define ENUM_FOR_EACH_149(x) ENUM_FOR_EACH_148(x) x(149)
#define ENUM_FOR_EACH_150(x) ENUM_FOR_EACH_149(x) x(150)
#define ENUM_FOR_EACH_151(x) ENUM_FOR_EACH_150(x) x(151)
#define ENUM_FOR_EACH_152(x) ENUM_FOR_EACH_151(x) x(152)
#define ENUM_FOR_EACH_153(x) ENUM_FOR_EACH_152(x) x(153)
#define ENUM_FOR_EACH_154(x) ENUM_FOR_EACH_153(x) x(154)
#define ENUM_FOR_EACH_155(x) ENUM_FOR_EACH_154(x) x(155)
#define ENUM_FOR_EACH_156(x) ENUM_FOR_EACH_155(x) x(156)
#define ENUM_FOR_EACH_157(x) ENUM_FOR_EACH_156(x) x(157)
#define ENUM_FOR_EACH_158(x) ENUM_FOR_EACH_157(x) x(158)
#define ENUM_FOR_EACH_159(x) ENUM_FOR_EACH_158(x) x(159)
#define ENUM_FOR_EACH_160(x) ENUM_FOR_EACH_159(x) x(160)
#define ENUM_FOR_EACH_161(x) ENUM_FOR_EACH_160(x) x(161)
#define ENUM_FOR_EACH_162(x) ENUM_FOR_EACH_161(x) x(162)
#define ENUM_FOR_EACH_163(x) ENUM_FOR_EACH_162(x) x(163)
#define ENUM_FOR_EACH_164(x) ENUM_FOR_EACH_163(x) x(164)
#define ENUM_FOR_EACH_165(x) ENUM_FOR_EACH_164(x) x(165)
#define ENUM_FOR_EACH_166(x) ENUM_FOR_EACH_165(x) x(166)
#define ENUM_FOR_EACH_167(x) ENUM_FOR_EACH_166(x) x(167)
#define ENUM_FOR_EACH_168(x) ENUM_FOR_EACH_167(x) x(168)
#define ENUM_FOR_EACH_169(x) ENUM_FOR_EACH_168(x) x(169)
#define ENUM_FOR_EACH_170(x) ENUM_FOR_EACH_169(x) x(170)
#define ENUM_FOR_EACH_171(x) ENUM_FOR_EACH_170(x) x(171)
#define ENUM_FOR_EACH_172(x) ENUM_FOR_EACH_171(x) x(172)
#define ENUM_FOR_EACH_173(x) ENUM_FOR_EACH_172(x) x(173)
#define ENUM_FOR_EACH_174(x) ENUM_FOR_EACH_173(x) x(174)
#define ENUM_FOR_EACH_175(x) ENUM_FOR_EACH_174(x) x(175)
#define ENUM_FOR_EACH_176(x) ENUM_FOR_EACH_175(x) x(176)
#define ENUM_FOR_EACH_177(x) ENUM_FOR_EACH_176(x) x(177)
#define ENUM_FOR_EACH_178(x) ENUM_FOR_EACH_177(x) x(178)
#define ENUM_FOR_EACH_179(x) ENUM_FOR_EACH_178(x) x(179)
#define ENUM_FOR_EACH_180(x) ENUM_FOR_EACH_179(x) x(180)
#define ENUM_FOR_EACH_181(x) ENUM_FOR_EACH_180(x) x(181)
#define ENUM_FOR_EACH_182(x) ENUM_FOR_EACH_181(x) x(182)
#define ENUM_FOR_EACH_183(x) ENUM_FOR_EACH_182(x) x(183)
#define ENUM_FOR_EACH_184(x) ENUM_FOR_EACH_183(x) x(184)
#define ENUM_FOR_EACH_185(x) ENUM_FOR_EACH_184(x) x(185)
#define ENUM_FOR_EACH_186(x) ENUM_FOR_EACH_185(x) x(186)
#define ENUM_FOR_EACH_187(x) ENUM_FOR_EACH_186(x) x(187)
#define ENUM_FOR_EACH_188(x) ENUM_FOR_EACH_187(x) x(188)
#define ENUM_FOR_EACH_189(x) ENUM_FOR_EACH_188(x) x(189)
#define ENUM_FOR_EACH_190(x) ENUM_FOR_EACH_189(x) x(190)
#define ENUM_FOR_EACH_191(x) ENUM_FOR_EACH_190(x) x(191)
#define ENUM_FOR_EACH_192(x) ENUM_FOR_EACH_191(x) x(192)
#define ENUM_FOR_EACH_193(x) ENUM_FOR_EACH_192(x) x(193)
#define ENUM_FOR_EACH_194(x) ENUM_FOR_EACH_193(x) x(194)
#define ENUM_FOR_EACH_195(x) ENUM_FOR_EACH_194(x) x(195)
#define ENUM_FOR_EACH_196(x) ENUM_FOR_EACH_195(x) x(196)
#define ENUM_FOR_EACH_197(x) ENUM_FOR_EACH_196(x) x(197)
#define ENUM_FOR_EACH_198(x) ENUM_FOR_EACH_197(x) x(198)
#define ENUM_FOR_EACH_199(x) ENUM_FOR_EACH_198(x) x(199)
#define ENUM_FOR_EACH_200(x) ENUM_FOR_EACH_199(x) x(200)
#define ENUM_FOR_EACH_201(x) ENUM_FOR_EACH_200(x) x(201)
#define ENUM_FOR_EACH_202(x) ENUM_FOR_EACH_201(x) x(202)
#define ENUM_FOR_EACH_203(x) ENUM_FOR_EACH_202(x) x(203)
#define ENUM_FOR_EACH_204(x) ENUM_FOR_EACH_203(x) x(204)
#define ENUM_FOR_EACH_205(x) ENUM_FOR_EACH_204(x) x(205)
#define ENUM_FOR_EACH_206(x) ENUM_FOR_EACH_205(x) x(206)
#define ENUM_FOR_EACH_207(x) ENUM_FOR_EACH_206(x) x(207)
#define ENUM_FOR_EACH_208(x) ENUM_FOR_EACH_207(x) x(208)
#define ENUM_FOR_EACH_209(x) ENUM_FOR_EACH_208(x) x(209)
#define ENUM_FOR_EACH_210(x) ENUM_FOR_EACH_209(x) x(210)
#define ENUM_FOR_EACH_211(x) ENUM_FOR_EACH_210(x) x(211)
#define ENUM_FOR_EACH_212(x) ENUM_FOR_EACH_211(x) x(212)
#define ENUM_FOR_EACH_213(x) ENUM_FOR_EACH_212(x) x(213)
#define ENUM_FOR_EACH_214(x) ENUM_FOR_EACH_213(x) x(214)
#define ENUM_FOR_EACH_215(x) ENUM_FOR_EACH_214(x) x(215)
#define ENUM_FOR_EACH_216(x) ENUM_FOR_EACH_215(x) x(216)
#define ENUM_FOR_EACH_217(x) ENUM_FOR_EACH_216(x) x(217)
#define ENUM_FOR_EACH_218(x) ENUM_FOR_EACH_217(x) x(218)
#define ENUM_FOR_EACH_219(x) ENUM_FOR_EACH_218(x) x(219)
#define ENUM_FOR_EACH_220(x) ENUM_FOR_EACH_219(x) x(220)
#define ENUM_FOR_EACH_221(x) ENUM_FOR_EACH_220(x) x(221)
#define ENUM_FOR_EACH_222(x) ENUM_FOR_EACH_221(x) x(222)
#define ENUM_FOR_EACH_223(x) ENUM_FOR_EACH_222(x) x(223)
#define ENUM_FOR_EACH_224(x) ENUM_FOR_EACH_223(x) x(224)
#define ENUM_FOR_EACH_225(x) ENUM_FOR_EACH_224(x) x(225)
#define ENUM_FOR_EACH_226(x) ENUM_FOR_EACH_225(x) x(226)
#define ENUM_FOR_EACH_227(x) ENUM_FOR_EACH_226(x) x(227)
#define ENUM_FOR_EACH_228(x) ENUM_FOR_EACH_227(x) x(228)
#define ENUM_FOR_EACH_229(x) ENUM_FOR_EACH_228(x) x(229)
#define ENUM_FOR_EACH_230(x) ENUM_FOR_EACH_229(x) x(230)
#define ENUM_FOR_EACH_231(x) ENUM_FOR_EACH_230(x) x(231)
#define ENUM_FOR_EACH_232(x) ENUM_FOR_EACH_231(x) x(232)
#define ENUM_FOR_EACH_233(x) ENUM_FOR_EACH_232(x) x(233)
#define ENUM_FOR_EACH_234(x) ENUM_FOR_EACH_233(x) x(234)
#define ENUM_FOR_EACH_235(x) ENUM_FOR_EACH_234(x) x(235)
#define ENUM_FOR_EACH_236(x) ENUM_FOR_EACH_235(x) x(236)
#define ENUM_FOR_EACH_237(x) ENUM_FOR_EACH_236(x) x(237)
#define ENUM_FOR_EACH_238(x) ENUM_FOR_EACH_237(x) x(238)
#define ENUM_FOR_EACH_239(x) ENUM_FOR_EACH_238(x) x(239)
#define ENUM_FOR_EACH_240(x) ENUM_FOR_EACH_239(x) x(240)
#define ENUM_FOR_EACH_241(x) ENUM_FOR_EACH_240(x) x(241)
#define ENUM_FOR_EACH_242(x) ENUM_FOR_EACH_241(x) x(242)
#define ENUM_FOR_EACH_243(x) ENUM_FOR_EACH_242(x) x(243)
#define ENUM_FOR_EACH_244(x) ENUM_FOR_EACH_243(x) x(244)
#define ENUM_FOR_EACH_245(x) ENUM_FOR_EACH_244(x) x(245)
#define ENUM_FOR_EACH_246(x) ENUM_FOR_EACH_245(x) x(246)
#define ENUM_FOR_EACH_247(x) ENUM_FOR_EACH_246(x) x(247)
#define ENUM_FOR_EACH_248(x) ENUM_FOR_EACH_247(x) x(248)
#define ENUM_FOR_EACH_249(x) ENUM_FOR_EACH_248(x) x(249)
#define ENUM_FOR_EACH_250(x) ENUM_FOR_EACH_249(x) x(250)
#define ENUM_FOR_EACH_251(x) ENUM_FOR_EACH_250(x) x(251)
#define ENUM_FOR_EACH_252(x) ENUM_FOR_EACH_251(x) x(252)
#define ENUM_FOR_EACH_253(x) ENUM_FOR_EACH_252(x) x(253)
#define ENUM_FOR_EACH_254(x) ENUM_FOR_EACH_253(x) x(254)
#define ENUM_FOR_EACH_255(x) ENUM_FOR_EACH_254(x) x(255)

#define ENUM_FOR_EACH(x, N) ENUM_FOR_EACH_##N(x)

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