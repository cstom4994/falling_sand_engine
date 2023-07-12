#ifndef ME_REFLECT_H
#define ME_REFLECT_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <deque>
#include <forward_list>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "engine/core/basic_types.h"
#include "engine/core/core.hpp"
#include "engine/utils/templatelist.hpp"
#include "engine/meta/static_relfection.hpp"

#define meta_offset(T, E) ((size_t)(&(((T *)(0))->E)))
#define meta_to_str(T) ((const char *)#T)

typedef enum meta_property_type {
    META_PROPERTY_TYPE_U8 = 0x00,
    META_PROPERTY_TYPE_U16,
    META_PROPERTY_TYPE_U32,
    META_PROPERTY_TYPE_U64,
    META_PROPERTY_TYPE_S8,
    META_PROPERTY_TYPE_S16,
    META_PROPERTY_TYPE_S32,
    META_PROPERTY_TYPE_S64,
    META_PROPERTY_TYPE_F32,
    META_PROPERTY_TYPE_F64,
    META_PROPERTY_TYPE_SIZE_T,
    META_PROPERTY_TYPE_STR,
    META_PROPERTY_TYPE_COUNT
} meta_property_type;

typedef struct meta_property_type_info_t {
    const char *name;  // Display name
    u32 id;            // Matches up to property type, used for lookups and switch statements
} meta_property_type_info_t;

extern meta_property_type_info_t _meta_property_type_info_decl_impl(const char *name, u32 id);

#define meta_property_type_info_decl(T, PROP_TYPE) _meta_property_type_info_decl_impl(meta_to_str(T), (PROP_TYPE))

#define META_PROPERTY_TYPE_INFO_U8 meta_property_type_info_decl(u8, META_PROPERTY_TYPE_U8)
#define META_PROPERTY_TYPE_INFO_S8 meta_property_type_info_decl(i8, META_PROPERTY_TYPE_S8)
#define META_PROPERTY_TYPE_INFO_U16 meta_property_type_info_decl(u16, META_PROPERTY_TYPE_U16)
#define META_PROPERTY_TYPE_INFO_S16 meta_property_type_info_decl(i16, META_PROPERTY_TYPE_S16)
#define META_PROPERTY_TYPE_INFO_U32 meta_property_type_info_decl(u32, META_PROPERTY_TYPE_U32)
#define META_PROPERTY_TYPE_INFO_S32 meta_property_type_info_decl(i32, META_PROPERTY_TYPE_S32)
#define META_PROPERTY_TYPE_INFO_U64 meta_property_type_info_decl(u64, META_PROPERTY_TYPE_U64)
#define META_PROPERTY_TYPE_INFO_S64 meta_property_type_info_decl(i64, META_PROPERTY_TYPE_S64)
#define META_PROPERTY_TYPE_INFO_F32 meta_property_type_info_decl(f32, META_PROPERTY_TYPE_F32)
#define META_PROPERTY_TYPE_INFO_F64 meta_property_type_info_decl(f64, META_PROPERTY_TYPE_F64)

#define META_PROPERTY_TYPE_INFO_SIZE_T meta_property_type_info_decl(size_t, META_PROPERTY_TYPE_SIZE_T)
#define META_PROPERTY_TYPE_INFO_STR meta_property_type_info_decl(char *, META_PROPERTY_TYPE_STR)

typedef struct meta_property_t {
    const char *name;                // Display name of field
    size_t offset;                   // Offset in bytes to struct
    meta_property_type_info_t type;  // Type info
} meta_property_t;

extern meta_property_t _meta_property_impl(const char *name, size_t offset, meta_property_type_info_t type);

#define meta_property(CLS, FIELD, TYPE) _meta_property_impl(meta_to_str(FIELD), meta_offset(CLS, FIELD), TYPE)

typedef struct meta_class_t {
    const char *name;             // Display name
    u32 property_count;           // Count of all properties in list
    meta_property_t *properties;  // List of all properties
} meta_class_t;

typedef struct meta_registry_t {
    struct {
        u64 key;
        meta_class_t value;
    } *classes;
} meta_registry_t;

typedef struct meta_class_decl_t {
    meta_property_t *properties;  // Array of properties
    size_t size;                  // Size of array in bytes
} meta_class_decl_t;

// Functions
extern meta_registry_t meta_registry_new();
extern void meta_registry_free(meta_registry_t *meta);
extern u64 _meta_registry_register_class_impl(meta_registry_t *meta, const char *name, const meta_class_decl_t *decl);
extern meta_class_t *_meta_class_getp_impl(meta_registry_t *meta, const char *name);

#define meta_registry_register_class(META, T, DECL) _meta_registry_register_class_impl((META), meta_to_str(T), (DECL))

#define meta_registry_class_get(META, T) _meta_class_getp_impl(META, meta_to_str(T))

#define meta_registry_getvp(OBJ, T, PROP) ((T *)((u8 *)(OBJ) + (PROP)->offset))

#define meta_registry_getv(OBJ, T, PROP) (*((T *)((u8 *)(OBJ) + (PROP)->offset)))

typedef struct c_meta {
    // cJSON *cjson;
} c_meta;

#pragma region DR

#ifndef enum_t
/// enum_t "enum type (scoped)" assumes the property of enum classes that encloses the enum values within a particular scope
/// e.g. MyClass::MyEnum::Value cannot be accessed via MyClass::Value (as it could with regular enums) and potentially cause redefinition errors
/// while avoiding the property of enum classes that removes the one-to-one relationship with the underlying type (which forces excessive type-casting)
///
/// Usage:
/// enum_t(name, type, {
///     enumerator = constexpr,
///     enumerator = constexpr,
///     ...
/// });
/* enum_t "enum type (scoped)" documentation minimized for expansion visibility, see definition for description and usage */
#define enum_t(name, type, ...)          \
    struct name##_ {                     \
        enum type##_ : type __VA_ARGS__; \
    };                                   \
    using name = typename name##_::type##_;
#pragma warning(disable : 26812)  // In the context of using enum_t, enum class is definitely not preferred, disable the warning in visual studios
#endif

/// Contains everything neccessary to loop over varadic macro arguments
namespace MacroLoops {
/// ArgMax: 125 (derived from the C spec limiting macros to 126 arguments and the COUNT_ARGUMENTS helper macro "ML_M" requiring ArgMax+1 arguments)

/// MacroLoop_Expand
#define ML_E(x) x

/// MacroLoop_Concatenate
#define ML_C(x, y) x##y

/// MacroLoop_ForEach[1, ..., ArgMax]
#define ML_1(f, a, ...) f(a)
#define ML_2(f, a, ...) f(a) ML_E(ML_1(f, __VA_ARGS__))
#define ML_3(f, a, ...) f(a) ML_E(ML_2(f, __VA_ARGS__))
#define ML_4(f, a, ...) f(a) ML_E(ML_3(f, __VA_ARGS__))
#define ML_5(f, a, ...) f(a) ML_E(ML_4(f, __VA_ARGS__))
#define ML_6(f, a, ...) f(a) ML_E(ML_5(f, __VA_ARGS__))
#define ML_7(f, a, ...) f(a) ML_E(ML_6(f, __VA_ARGS__))
#define ML_8(f, a, ...) f(a) ML_E(ML_7(f, __VA_ARGS__))
#define ML_9(f, a, ...) f(a) ML_E(ML_8(f, __VA_ARGS__))
#define ML_10(f, a, ...) f(a) ML_E(ML_9(f, __VA_ARGS__))
#define ML_11(f, a, ...) f(a) ML_E(ML_10(f, __VA_ARGS__))
#define ML_12(f, a, ...) f(a) ML_E(ML_11(f, __VA_ARGS__))
#define ML_13(f, a, ...) f(a) ML_E(ML_12(f, __VA_ARGS__))
#define ML_14(f, a, ...) f(a) ML_E(ML_13(f, __VA_ARGS__))
#define ML_15(f, a, ...) f(a) ML_E(ML_14(f, __VA_ARGS__))
#define ML_16(f, a, ...) f(a) ML_E(ML_15(f, __VA_ARGS__))
#define ML_17(f, a, ...) f(a) ML_E(ML_16(f, __VA_ARGS__))
#define ML_18(f, a, ...) f(a) ML_E(ML_17(f, __VA_ARGS__))
#define ML_19(f, a, ...) f(a) ML_E(ML_18(f, __VA_ARGS__))
#define ML_20(f, a, ...) f(a) ML_E(ML_19(f, __VA_ARGS__))
#define ML_21(f, a, ...) f(a) ML_E(ML_20(f, __VA_ARGS__))
#define ML_22(f, a, ...) f(a) ML_E(ML_21(f, __VA_ARGS__))
#define ML_23(f, a, ...) f(a) ML_E(ML_22(f, __VA_ARGS__))
#define ML_24(f, a, ...) f(a) ML_E(ML_23(f, __VA_ARGS__))
#define ML_25(f, a, ...) f(a) ML_E(ML_24(f, __VA_ARGS__))
#define ML_26(f, a, ...) f(a) ML_E(ML_25(f, __VA_ARGS__))
#define ML_27(f, a, ...) f(a) ML_E(ML_26(f, __VA_ARGS__))
#define ML_28(f, a, ...) f(a) ML_E(ML_27(f, __VA_ARGS__))
#define ML_29(f, a, ...) f(a) ML_E(ML_28(f, __VA_ARGS__))
#define ML_30(f, a, ...) f(a) ML_E(ML_29(f, __VA_ARGS__))
#define ML_31(f, a, ...) f(a) ML_E(ML_30(f, __VA_ARGS__))
#define ML_32(f, a, ...) f(a) ML_E(ML_31(f, __VA_ARGS__))
#define ML_33(f, a, ...) f(a) ML_E(ML_32(f, __VA_ARGS__))
#define ML_34(f, a, ...) f(a) ML_E(ML_33(f, __VA_ARGS__))
#define ML_35(f, a, ...) f(a) ML_E(ML_34(f, __VA_ARGS__))
#define ML_36(f, a, ...) f(a) ML_E(ML_35(f, __VA_ARGS__))
#define ML_37(f, a, ...) f(a) ML_E(ML_36(f, __VA_ARGS__))
#define ML_38(f, a, ...) f(a) ML_E(ML_37(f, __VA_ARGS__))
#define ML_39(f, a, ...) f(a) ML_E(ML_38(f, __VA_ARGS__))
#define ML_40(f, a, ...) f(a) ML_E(ML_39(f, __VA_ARGS__))
#define ML_41(f, a, ...) f(a) ML_E(ML_40(f, __VA_ARGS__))
#define ML_42(f, a, ...) f(a) ML_E(ML_41(f, __VA_ARGS__))
#define ML_43(f, a, ...) f(a) ML_E(ML_42(f, __VA_ARGS__))
#define ML_44(f, a, ...) f(a) ML_E(ML_43(f, __VA_ARGS__))
#define ML_45(f, a, ...) f(a) ML_E(ML_44(f, __VA_ARGS__))
#define ML_46(f, a, ...) f(a) ML_E(ML_45(f, __VA_ARGS__))
#define ML_47(f, a, ...) f(a) ML_E(ML_46(f, __VA_ARGS__))
#define ML_48(f, a, ...) f(a) ML_E(ML_47(f, __VA_ARGS__))
#define ML_49(f, a, ...) f(a) ML_E(ML_48(f, __VA_ARGS__))
#define ML_50(f, a, ...) f(a) ML_E(ML_49(f, __VA_ARGS__))
#define ML_51(f, a, ...) f(a) ML_E(ML_50(f, __VA_ARGS__))
#define ML_52(f, a, ...) f(a) ML_E(ML_51(f, __VA_ARGS__))
#define ML_53(f, a, ...) f(a) ML_E(ML_52(f, __VA_ARGS__))
#define ML_54(f, a, ...) f(a) ML_E(ML_53(f, __VA_ARGS__))
#define ML_55(f, a, ...) f(a) ML_E(ML_54(f, __VA_ARGS__))
#define ML_56(f, a, ...) f(a) ML_E(ML_55(f, __VA_ARGS__))
#define ML_57(f, a, ...) f(a) ML_E(ML_56(f, __VA_ARGS__))
#define ML_58(f, a, ...) f(a) ML_E(ML_57(f, __VA_ARGS__))
#define ML_59(f, a, ...) f(a) ML_E(ML_58(f, __VA_ARGS__))
#define ML_60(f, a, ...) f(a) ML_E(ML_59(f, __VA_ARGS__))
#define ML_61(f, a, ...) f(a) ML_E(ML_60(f, __VA_ARGS__))
#define ML_62(f, a, ...) f(a) ML_E(ML_61(f, __VA_ARGS__))
#define ML_63(f, a, ...) f(a) ML_E(ML_62(f, __VA_ARGS__))
#define ML_64(f, a, ...) f(a) ML_E(ML_63(f, __VA_ARGS__))
#define ML_65(f, a, ...) f(a) ML_E(ML_64(f, __VA_ARGS__))
#define ML_66(f, a, ...) f(a) ML_E(ML_65(f, __VA_ARGS__))
#define ML_67(f, a, ...) f(a) ML_E(ML_66(f, __VA_ARGS__))
#define ML_68(f, a, ...) f(a) ML_E(ML_67(f, __VA_ARGS__))
#define ML_69(f, a, ...) f(a) ML_E(ML_68(f, __VA_ARGS__))
#define ML_70(f, a, ...) f(a) ML_E(ML_69(f, __VA_ARGS__))
#define ML_71(f, a, ...) f(a) ML_E(ML_70(f, __VA_ARGS__))
#define ML_72(f, a, ...) f(a) ML_E(ML_71(f, __VA_ARGS__))
#define ML_73(f, a, ...) f(a) ML_E(ML_72(f, __VA_ARGS__))
#define ML_74(f, a, ...) f(a) ML_E(ML_73(f, __VA_ARGS__))
#define ML_75(f, a, ...) f(a) ML_E(ML_74(f, __VA_ARGS__))
#define ML_76(f, a, ...) f(a) ML_E(ML_75(f, __VA_ARGS__))
#define ML_77(f, a, ...) f(a) ML_E(ML_76(f, __VA_ARGS__))
#define ML_78(f, a, ...) f(a) ML_E(ML_77(f, __VA_ARGS__))
#define ML_79(f, a, ...) f(a) ML_E(ML_78(f, __VA_ARGS__))
#define ML_80(f, a, ...) f(a) ML_E(ML_79(f, __VA_ARGS__))
#define ML_81(f, a, ...) f(a) ML_E(ML_80(f, __VA_ARGS__))
#define ML_82(f, a, ...) f(a) ML_E(ML_81(f, __VA_ARGS__))
#define ML_83(f, a, ...) f(a) ML_E(ML_82(f, __VA_ARGS__))
#define ML_84(f, a, ...) f(a) ML_E(ML_83(f, __VA_ARGS__))
#define ML_85(f, a, ...) f(a) ML_E(ML_84(f, __VA_ARGS__))
#define ML_86(f, a, ...) f(a) ML_E(ML_85(f, __VA_ARGS__))
#define ML_87(f, a, ...) f(a) ML_E(ML_86(f, __VA_ARGS__))
#define ML_88(f, a, ...) f(a) ML_E(ML_87(f, __VA_ARGS__))
#define ML_89(f, a, ...) f(a) ML_E(ML_88(f, __VA_ARGS__))
#define ML_90(f, a, ...) f(a) ML_E(ML_89(f, __VA_ARGS__))
#define ML_91(f, a, ...) f(a) ML_E(ML_90(f, __VA_ARGS__))
#define ML_92(f, a, ...) f(a) ML_E(ML_91(f, __VA_ARGS__))
#define ML_93(f, a, ...) f(a) ML_E(ML_92(f, __VA_ARGS__))
#define ML_94(f, a, ...) f(a) ML_E(ML_93(f, __VA_ARGS__))
#define ML_95(f, a, ...) f(a) ML_E(ML_94(f, __VA_ARGS__))
#define ML_96(f, a, ...) f(a) ML_E(ML_95(f, __VA_ARGS__))
#define ML_97(f, a, ...) f(a) ML_E(ML_96(f, __VA_ARGS__))
#define ML_98(f, a, ...) f(a) ML_E(ML_97(f, __VA_ARGS__))
#define ML_99(f, a, ...) f(a) ML_E(ML_98(f, __VA_ARGS__))
#define ML_100(f, a, ...) f(a) ML_E(ML_99(f, __VA_ARGS__))
#define ML_101(f, a, ...) f(a) ML_E(ML_100(f, __VA_ARGS__))
#define ML_102(f, a, ...) f(a) ML_E(ML_101(f, __VA_ARGS__))
#define ML_103(f, a, ...) f(a) ML_E(ML_102(f, __VA_ARGS__))
#define ML_104(f, a, ...) f(a) ML_E(ML_103(f, __VA_ARGS__))
#define ML_105(f, a, ...) f(a) ML_E(ML_104(f, __VA_ARGS__))
#define ML_106(f, a, ...) f(a) ML_E(ML_105(f, __VA_ARGS__))
#define ML_107(f, a, ...) f(a) ML_E(ML_106(f, __VA_ARGS__))
#define ML_108(f, a, ...) f(a) ML_E(ML_107(f, __VA_ARGS__))
#define ML_109(f, a, ...) f(a) ML_E(ML_108(f, __VA_ARGS__))
#define ML_110(f, a, ...) f(a) ML_E(ML_109(f, __VA_ARGS__))
#define ML_111(f, a, ...) f(a) ML_E(ML_110(f, __VA_ARGS__))
#define ML_112(f, a, ...) f(a) ML_E(ML_111(f, __VA_ARGS__))
#define ML_113(f, a, ...) f(a) ML_E(ML_112(f, __VA_ARGS__))
#define ML_114(f, a, ...) f(a) ML_E(ML_113(f, __VA_ARGS__))
#define ML_115(f, a, ...) f(a) ML_E(ML_114(f, __VA_ARGS__))
#define ML_116(f, a, ...) f(a) ML_E(ML_115(f, __VA_ARGS__))
#define ML_117(f, a, ...) f(a) ML_E(ML_116(f, __VA_ARGS__))
#define ML_118(f, a, ...) f(a) ML_E(ML_117(f, __VA_ARGS__))
#define ML_119(f, a, ...) f(a) ML_E(ML_118(f, __VA_ARGS__))
#define ML_120(f, a, ...) f(a) ML_E(ML_119(f, __VA_ARGS__))
#define ML_121(f, a, ...) f(a) ML_E(ML_120(f, __VA_ARGS__))
#define ML_122(f, a, ...) f(a) ML_E(ML_121(f, __VA_ARGS__))
#define ML_123(f, a, ...) f(a) ML_E(ML_122(f, __VA_ARGS__))
#define ML_124(f, a, ...) f(a) ML_E(ML_123(f, __VA_ARGS__))
#define ML_125(f, a, ...) f(a) ML_E(ML_124(f, __VA_ARGS__))

/// MacroLoop_ArgumentCounts [ArgMax ... 0]
#define ML_G()                                                                                                                                                                                        \
    125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, \
            83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38,   \
            37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

/// MacroLoop_Select_Argument_At_Argument_Max [a0, ..., a(ArgMax-1), argAtArgMax]
#define ML_M(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, c0, c1, c2, c3, c4, c5, c6, \
             c7, c8, c9, d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, g0, g1, g2, g3, g4, g5, g6, g7, g8, g9, h0, h1, h2,   \
             h3, h4, h5, h6, h7, h8, h9, i0, i1, i2, i3, i4, i5, i6, i7, i8, i9, j0, j1, j2, j3, j4, j5, j6, j7, j8, argAtArgMax, ...)                                                                 \
    argAtArgMax

/// MacroLoop_Expand_Select_Argument_At_Argument_Max (necessary due to bugs in VS macro handling)
#define ML_S(...) ML_E(ML_M(__VA_ARGS__))

/// MacroLoop_ForEach_N
#define ML_N(N, f, ...) ML_E(ML_C(ML_, N)(f, __VA_ARGS__))

/// Selects the count of varadic arguments
#define COUNT_ARGUMENTS(...) ML_S(__VA_ARGS__, ML_G())

/// Call "f" for each argument
#define FOR_EACH(f, ...) ML_N(COUNT_ARGUMENTS(__VA_ARGS__), f, __VA_ARGS__)

};  // namespace MacroLoops

namespace ExtendedTypeSupport {
template <typename T>
struct promote_char {
    using type = T;
};
template <>
struct promote_char<char> {
    using type = int;
};
template <>
struct promote_char<signed char> {
    using type = int;
};
template <>
struct promote_char<unsigned char> {
    using type = int;
};
template <>
struct promote_char<const char> {
    using type = const int;
};
template <>
struct promote_char<const signed char> {
    using type = const int;
};
template <>
struct promote_char<const unsigned char> {
    using type = const int;
};

template <typename T>
struct pair_lhs {
    using type = void;
};
template <typename L, typename R>
struct pair_lhs<std::pair<L, R>> {
    using type = L;
};
template <typename L, typename R>
struct pair_lhs<const std::pair<L, R>> {
    using type = L;
};

template <typename T>
struct pair_rhs {
    using type = void;
};
template <typename L, typename R>
struct pair_rhs<std::pair<L, R>> {
    using type = R;
};
template <typename L, typename R>
struct pair_rhs<const std::pair<L, R>> {
    using type = R;
};

template <typename T>
struct element_type {
    using type = void;
};
template <typename T>
struct element_type<const T> {
    using type = typename element_type<T>::type;
};
template <typename T, size_t N>
struct element_type<T[N]> {
    using type = T;
};
template <typename T, size_t N>
struct element_type<const T[N]> {
    using type = T;
};
template <typename T, size_t N>
struct element_type<std::array<T, N>> {
    using type = T;
};
template <typename T, typename A>
struct element_type<std::vector<T, A>> {
    using type = T;
};
template <typename T, typename A>
struct element_type<std::deque<T, A>> {
    using type = T;
};
template <typename T, typename A>
struct element_type<std::list<T, A>> {
    using type = T;
};
template <typename T, typename A>
struct element_type<std::forward_list<T, A>> {
    using type = T;
};
template <typename T, typename C>
struct element_type<std::stack<T, C>> {
    using type = T;
};
template <typename T, typename C>
struct element_type<std::queue<T, C>> {
    using type = T;
};
template <typename T, typename C, typename P>
struct element_type<std::priority_queue<T, C, P>> {
    using type = T;
};
template <typename K, typename C, typename A>
struct element_type<std::set<K, C, A>> {
    using type = K;
};
template <typename K, typename C, typename A>
struct element_type<std::multiset<K, C, A>> {
    using type = K;
};
template <typename K, typename H, typename E, typename A>
struct element_type<std::unordered_set<K, H, E, A>> {
    using type = K;
};
template <typename K, typename H, typename E, typename A>
struct element_type<std::unordered_multiset<K, H, E, A>> {
    using type = K;
};
template <typename K, typename T, typename C, typename A>
struct element_type<std::map<K, T, C, A>> {
    using type = typename std::pair<K, T>;
};
template <typename K, typename T, typename C, typename A>
struct element_type<std::multimap<K, T, C, A>> {
    using type = typename std::pair<K, T>;
};
template <typename K, typename T, typename H, typename E, typename A>
struct element_type<std::unordered_map<K, T, H, E, A>> {
    using type = typename std::pair<K, T>;
};
template <typename K, typename T, typename H, typename E, typename A>
struct element_type<std::unordered_multimap<K, T, H, E, A>> {
    using type = typename std::pair<K, T>;
};

template <typename T>
struct is_pointable {
    static constexpr bool value = std::is_pointer<T>::value;
};
template <typename T>
struct is_pointable<std::unique_ptr<T>> {
    static constexpr bool value = true;
};
template <typename T>
struct is_pointable<std::shared_ptr<T>> {
    static constexpr bool value = true;
};
template <typename T>
struct is_pointable<std::weak_ptr<T>> {
    static constexpr bool value = true;
};
template <typename T>
struct is_pointable<const T> {
    static constexpr bool value = is_pointable<T>::value;
};

template <typename T>
struct remove_pointer {
    using type = typename std::remove_pointer<T>::type;
};
template <typename T>
struct remove_pointer<std::unique_ptr<T>> {
    using type = T;
};
template <typename T>
struct remove_pointer<std::shared_ptr<T>> {
    using type = T;
};
template <typename T>
struct remove_pointer<std::weak_ptr<T>> {
    using type = T;
};
template <typename T>
struct remove_pointer<const T> {
    using type = std::conditional_t<is_pointable<T>::value, typename remove_pointer<T>::type, const T>;
};

template <typename T>
struct static_array_size {
    static constexpr size_t value = 0;
};
template <typename T, size_t N>
struct static_array_size<T[N]> {
    static constexpr size_t value = N;
};
template <typename T, size_t N>
struct static_array_size<const T[N]> {
    static constexpr size_t value = N;
};
template <typename T, size_t N>
struct static_array_size<std::array<T, N>> {
    static constexpr size_t value = N;
};
template <typename T, size_t N>
struct static_array_size<const std::array<T, N>> {
    static constexpr size_t value = N;
};

template <typename T>
struct is_static_array {
    static constexpr bool value = false;
};
template <typename T, size_t N>
struct is_static_array<T[N]> {
    static constexpr bool value = true;
};
template <typename T, size_t N>
struct is_static_array<const T[N]> {
    static constexpr bool value = true;
};
template <typename T, size_t N>
struct is_static_array<std::array<T, N>> {
    static constexpr bool value = true;
};
template <typename T, size_t N>
struct is_static_array<const std::array<T, N>> {
    static constexpr bool value = true;
};

template <typename T>
struct is_iterable {
    static constexpr bool value = !std::is_same<void, typename element_type<T>::type>::value;
};

template <typename T>
struct is_map {
    static constexpr bool value = false;
};
template <typename T>
struct is_map<const T> {
    static constexpr bool value = is_map<T>::value;
};
template <typename K, typename T, typename C, typename A>
struct is_map<std::map<K, T, C, A>> {
    static constexpr bool value = true;
};
template <typename K, typename T, typename C, typename A>
struct is_map<std::multimap<K, T, C, A>> {
    static constexpr bool value = true;
};
template <typename K, typename T, typename H, typename E, typename A>
struct is_map<std::unordered_map<K, T, H, E, A>> {
    static constexpr bool value = true;
};
template <typename K, typename T, typename H, typename E, typename A>
struct is_map<std::unordered_multimap<K, T, H, E, A>> {
    static constexpr bool value = true;
};

template <typename T>
struct is_stl_iterable {
    static constexpr bool value = false;
};
template <typename T>
struct is_stl_iterable<const T> {
    static constexpr bool value = is_stl_iterable<T>::value;
};
template <typename T, size_t N>
struct is_stl_iterable<std::array<T, N>> {
    static constexpr bool value = true;
};
template <typename T, typename A>
struct is_stl_iterable<std::vector<T, A>> {
    static constexpr bool value = true;
};
template <typename T, typename A>
struct is_stl_iterable<std::deque<T, A>> {
    static constexpr bool value = true;
};
template <typename T, typename A>
struct is_stl_iterable<std::list<T, A>> {
    static constexpr bool value = true;
};
template <typename T, typename A>
struct is_stl_iterable<std::forward_list<T, A>> {
    static constexpr bool value = true;
};
template <typename K, typename C, typename A>
struct is_stl_iterable<std::set<K, C, A>> {
    static constexpr bool value = true;
};
template <typename K, typename C, typename A>
struct is_stl_iterable<std::multiset<K, C, A>> {
    static constexpr bool value = true;
};
template <typename K, typename H, typename E, typename A>
struct is_stl_iterable<std::unordered_set<K, H, E, A>> {
    static constexpr bool value = true;
};
template <typename K, typename H, typename E, typename A>
struct is_stl_iterable<std::unordered_multiset<K, H, E, A>> {
    static constexpr bool value = true;
};
template <typename K, typename T, typename C, typename A>
struct is_stl_iterable<std::map<K, T, C, A>> {
    static constexpr bool value = true;
};
template <typename K, typename T, typename C, typename A>
struct is_stl_iterable<std::multimap<K, T, C, A>> {
    static constexpr bool value = true;
};
template <typename K, typename T, typename H, typename E, typename A>
struct is_stl_iterable<std::unordered_map<K, T, H, E, A>> {
    static constexpr bool value = true;
};
template <typename K, typename T, typename H, typename E, typename A>
struct is_stl_iterable<std::unordered_multimap<K, T, H, E, A>> {
    static constexpr bool value = true;
};

template <typename T>
struct is_adaptor {
    static constexpr bool value = false;
};
template <typename T>
struct is_adaptor<const T> {
    static constexpr bool value = is_adaptor<T>::value;
};
template <typename T, typename C>
struct is_adaptor<std::stack<T, C>> {
    static constexpr bool value = true;
};
template <typename T, typename C>
struct is_adaptor<std::queue<T, C>> {
    static constexpr bool value = true;
};
template <typename T, typename C, typename P>
struct is_adaptor<std::priority_queue<T, C, P>> {
    static constexpr bool value = true;
};

template <typename T>
struct is_forward_list {
    static constexpr bool value = false;
};
template <typename T, typename A>
struct is_forward_list<std::forward_list<T, A>> {
    static constexpr bool value = true;
};
template <typename T, typename A>
struct is_forward_list<const std::forward_list<T, A>> {
    static constexpr bool value = true;
};

template <typename T>
struct is_pair {
    static constexpr bool value = false;
};
template <typename L, typename R>
struct is_pair<std::pair<L, R>> {
    static constexpr bool value = true;
};
template <typename L, typename R>
struct is_pair<const std::pair<L, R>> {
    static constexpr bool value = true;
};

template <typename T>
struct is_tuple {
    static constexpr bool value = false;
};
template <typename... Ts>
struct is_tuple<std::tuple<Ts...>> {
    static constexpr bool value = true;
};
template <typename... Ts>
struct is_tuple<const std::tuple<Ts...>> {
    static constexpr bool value = true;
};

template <typename T>
struct is_bool {
    static constexpr bool value = std::is_same<bool, typename std::remove_const<T>::type>::value;
};

template <typename T>
struct is_string {
    static constexpr bool value = std::is_same<std::string, typename std::remove_const<T>::type>::value;
};

template <typename T>
struct has_push_back {
    static constexpr bool value = false;
};
template <typename T>
struct has_push_back<const T> {
    static constexpr bool value = has_push_back<T>::value;
};
template <typename T, typename A>
struct has_push_back<std::vector<T, A>> {
    static constexpr bool value = true;
};
template <typename T, typename A>
struct has_push_back<std::deque<T, A>> {
    static constexpr bool value = true;
};
template <typename T, typename A>
struct has_push_back<std::list<T, A>> {
    static constexpr bool value = true;
};

template <typename T>
struct has_push {
    static constexpr bool value = false;
};
template <typename T>
struct has_push<const T> {
    static constexpr bool value = has_push<T>::value;
};
template <typename T, typename C>
struct has_push<std::stack<T, C>> {
    static constexpr bool value = true;
};
template <typename T, typename C>
struct has_push<std::queue<T, C>> {
    static constexpr bool value = true;
};
template <typename T, typename C, typename P>
struct has_push<std::priority_queue<T, C, P>> {
    static constexpr bool value = true;
};

template <typename T>
struct has_insert {
    static constexpr bool value = false;
};
template <typename T>
struct has_insert<const T> {
    static constexpr bool value = has_insert<T>::value;
};
template <typename K, typename C, typename A>
struct has_insert<std::set<K, C, A>> {
    static constexpr bool value = true;
};
template <typename K, typename C, typename A>
struct has_insert<std::multiset<K, C, A>> {
    static constexpr bool value = true;
};
template <typename K, typename H, typename E, typename A>
struct has_insert<std::unordered_set<K, H, E, A>> {
    static constexpr bool value = true;
};
template <typename K, typename H, typename E, typename A>
struct has_insert<std::unordered_multiset<K, H, E, A>> {
    static constexpr bool value = true;
};
template <typename K, typename T, typename C, typename A>
struct has_insert<std::map<K, T, C, A>> {
    static constexpr bool value = true;
};
template <typename K, typename T, typename C, typename A>
struct has_insert<std::multimap<K, T, C, A>> {
    static constexpr bool value = true;
};
template <typename K, typename T, typename H, typename E, typename A>
struct has_insert<std::unordered_map<K, T, H, E, A>> {
    static constexpr bool value = true;
};
template <typename K, typename T, typename H, typename E, typename A>
struct has_insert<std::unordered_multimap<K, T, H, E, A>> {
    static constexpr bool value = true;
};

template <typename T>
struct has_clear {
    static constexpr bool value = false;
};
template <typename T>
struct has_clear<const T> {
    static constexpr bool value = has_clear<T>::value;
};
template <typename T, typename A>
struct has_clear<std::vector<T, A>> {
    static constexpr bool value = true;
};
template <typename T, typename A>
struct has_clear<std::deque<T, A>> {
    static constexpr bool value = true;
};
template <typename T, typename A>
struct has_clear<std::list<T, A>> {
    static constexpr bool value = true;
};
template <typename T, typename A>
struct has_clear<std::forward_list<T, A>> {
    static constexpr bool value = true;
};
template <typename K, typename C, typename A>
struct has_clear<std::set<K, C, A>> {
    static constexpr bool value = true;
};
template <typename K, typename C, typename A>
struct has_clear<std::multiset<K, C, A>> {
    static constexpr bool value = true;
};
template <typename K, typename H, typename E, typename A>
struct has_clear<std::unordered_set<K, H, E, A>> {
    static constexpr bool value = true;
};
template <typename K, typename H, typename E, typename A>
struct has_clear<std::unordered_multiset<K, H, E, A>> {
    static constexpr bool value = true;
};
template <typename K, typename T, typename C, typename A>
struct has_clear<std::map<K, T, C, A>> {
    static constexpr bool value = true;
};
template <typename K, typename T, typename C, typename A>
struct has_clear<std::multimap<K, T, C, A>> {
    static constexpr bool value = true;
};
template <typename K, typename T, typename H, typename E, typename A>
struct has_clear<std::unordered_map<K, T, H, E, A>> {
    static constexpr bool value = true;
};
template <typename K, typename T, typename H, typename E, typename A>
struct has_clear<std::unordered_multimap<K, T, H, E, A>> {
    static constexpr bool value = true;
};

template <typename Iterable, typename Element>
static constexpr void Append(Iterable &iterable, Element &element) {
    if constexpr (!std::is_const<Iterable>::value) {
        if constexpr (has_push_back<Iterable>::value)
            iterable.push_back(element);
        else if constexpr (is_forward_list<Iterable>::value) {
            auto last = iterable.before_begin();
            for (auto curr = last; curr != iterable.end(); last = curr++)
                ;
            iterable.insert_after(last, element);
        } else if constexpr (has_push<Iterable>::value)
            iterable.push(element);
        else if constexpr (has_insert<Iterable>::value)
            iterable.insert(element);
    }
}

template <typename Iterable>
static constexpr bool IterIsEmpty(const Iterable &iterable) {
    if constexpr (std::is_array<Iterable>::value)
        return std::extent<Iterable>::value == 0;
    else
        return iterable.empty();
}

template <typename Iterable>
static constexpr void Clear(Iterable &iterable) {
    if constexpr (!std::is_const<Iterable>::value) {
        if constexpr (has_clear<Iterable>::value)
            iterable.clear();
        else if constexpr (is_adaptor<Iterable>::value) {
            while (!iterable.empty()) iterable.pop();
        }
    }
}

inline namespace OpDetection {
namespace OpDetectImpl {
template <class AlwaysVoid, template <class...> class Op, class... Args>
struct OpExists {
    static constexpr bool value = false;
};
template <template <class...> class Op, class... Args>
struct OpExists<std::void_t<Op<Args...>>, Op, Args...> {
    static constexpr bool value = true;
};

template <typename L, typename R>
using AssignmentOp = decltype(std::declval<L>() = std::declval<const R &>());
template <typename L, typename R>
using StaticCastAssignmentOp = decltype(std::declval<L>() = static_cast<std::remove_reference_t<L>>(std::declval<const R &>()));
template <typename L, typename R>
using MapToOp = decltype(std::declval<L>().map_to(std::declval<R &>()));
template <typename L, typename R>
using MapFromOp = decltype(std::declval<L>().map_from(std::declval<const R &>()));
}  // namespace OpDetectImpl

template <typename L, typename R>
static constexpr bool IsAssignable = OpDetectImpl::OpExists<void, OpDetectImpl::AssignmentOp, L, R>::value;
template <typename L, typename R>
static constexpr bool IsStaticCastAssignable = OpDetectImpl::OpExists<void, OpDetectImpl::StaticCastAssignmentOp, L, R>::value;
template <typename L, typename R>
static constexpr bool HasMapTo = OpDetectImpl::OpExists<void, OpDetectImpl::MapToOp, L, R>::value;
template <typename L, typename R>
static constexpr bool HasMapFrom = OpDetectImpl::OpExists<void, OpDetectImpl::MapFromOp, L, R>::value;
};  // namespace OpDetection

template <typename L, typename R>
struct TypePair {
    using Left = L;
    using Right = R;
};

template <typename T, template <typename...> class Of>
struct is_specialization : std::false_type {};
template <template <typename...> class Of, typename... Ts>
struct is_specialization<Of<Ts...>, Of> : std::true_type {};
template <typename T, template <typename...> class Of>
inline constexpr bool is_specialization_v = is_specialization<T, Of>::value;

template <typename... Ts>
struct type_list {
    template <typename T>
    struct has : std::bool_constant<(std::is_same_v<Ts, T> || ...)> {};
    template <typename T>
    static inline constexpr bool has_v = has<T>::value;
    template <template <typename...> class Of>
    struct has_specialization : std::bool_constant<(is_specialization_v<Ts, Of> || ...)> {};
    template <template <typename...> class Of>
    static inline constexpr bool has_specialization_v = has_specialization<Of>::value;
    template <template <typename...> class Of>
    class get_specialization {
        template <typename... Us>
        struct get {
            using type = void;
        };
        template <typename U, typename... Us>
        struct get<U, Us...> {
            using type = std::conditional_t<is_specialization_v<U, Of>, U, typename get<Us...>::type>;
        };

    public:
        using type = typename get<Ts...>::type;
    };
    template <template <typename...> class Of>
    using get_specialization_t = typename get_specialization<Of>::type;
};
template <typename... Ts>
struct type_list<const std::tuple<Ts...>> : type_list<Ts...> {};
template <typename... Ts>
struct type_list<std::tuple<Ts...>> : type_list<Ts...> {};

struct Unspecialized {};  // Primary templates (and not specializations), should inherit from Unspecialized
template <typename T>
struct is_specialized {
    static constexpr bool value = !std::is_base_of<Unspecialized, T>::value;
};

template <typename T, typename TypeIfVoid>
struct if_void {
    using type = T;
};
template <typename TypeIfVoid>
struct if_void<void, TypeIfVoid> {
    using type = TypeIfVoid;
};
template <typename T, typename TypeIfVoid>
using if_void_t = typename if_void<T, TypeIfVoid>::type;

template <typename T>
constexpr bool HasTypeRecursion() {
    return false;
}

template <typename T, typename CurrentType, typename... NextTypes>
constexpr bool HasTypeRecursion() {
    if constexpr (std::is_same<T, CurrentType>::value)
        return true;
    else
        return HasTypeRecursion<T, NextTypes...>();
}

template <typename T, typename... Ts>
struct has_type {
    static constexpr bool value = HasTypeRecursion<T, Ts...>();
};
template <typename T, typename... Ts>
struct has_type<T, std::tuple<Ts...>> {
    static constexpr bool value = HasTypeRecursion<T, Ts...>();
};
template <typename T, typename... Ts>
struct has_type<T, const std::tuple<Ts...>> {
    static constexpr bool value = HasTypeRecursion<T, Ts...>();
};

template <typename TypeToGet>
class get {
private:
    template <typename... Ts>
    struct get_impl {
        template <size_t Index>
        static constexpr TypeToGet GetElementRecursion(const std::tuple<Ts...> &elements) {
#pragma warning(suppress : 26444)  // Warning incorrectly raised for unnamed local variable
            return {};
        }

        template <size_t Index, typename CurrentType, typename... NextTypes>
        static constexpr TypeToGet GetElementRecursion(const std::tuple<Ts...> &elements) {
            if constexpr (std::is_same<TypeToGet, CurrentType>::value)
                return std::get<Index>(elements);
            else
                return GetElementRecursion<Index + 1, NextTypes...>(elements);
        }

        static constexpr TypeToGet GetElement(const std::tuple<Ts...> &elements) { return GetElementRecursion<0, Ts...>(elements); }
    };

public:
    template <typename... Ts>
    static constexpr TypeToGet from(const std::tuple<Ts...> &elements) {
        return get_impl<Ts...>::GetElement(elements);
    }
};

template <typename TypeToGet>
class for_each {
private:
    template <typename... Ts>
    struct for_each_impl {
        template <size_t Index, typename Function>
        static constexpr void ForEachElementRecursion(const std::tuple<Ts...> &elements, Function function) {}

        template <size_t Index, typename Function, typename CurrentType, typename... NextTypes>
        static constexpr void ForEachElementRecursion(const std::tuple<Ts...> &elements, Function function) {
            if constexpr (std::is_same<TypeToGet, CurrentType>::value) function(std::get<Index>(elements));

            ForEachElementRecursion<Index + 1, Function, NextTypes...>(elements, function);
        }

        template <typename Function>
        static constexpr void ForEachElement(const std::tuple<Ts...> &elements, Function function) {
            return ForEachElementRecursion<0, Function, Ts...>(elements, function);
        }
    };

public:
    template <typename Function, typename... Ts>
    static constexpr void in(const std::tuple<Ts...> &elements, Function function) {
        for_each_impl<Ts...>::ForEachElement(elements, function);
    }
};

template <typename... Ts>
struct ForEachIn {
    template <size_t Index, typename Function>
    static constexpr void ForEachElementRecursion(const std::tuple<Ts...> &elements, Function function) {}

    template <size_t Index, typename Function, typename CurrentType, typename... NextTypes>
    static constexpr void ForEachElementRecursion(const std::tuple<Ts...> &elements, Function function) {
        function(std::get<Index>(elements));
        ForEachElementRecursion<Index + 1, Function, NextTypes...>(elements, function);
    }

    template <typename Function>
    static constexpr void ForEachElement(const std::tuple<Ts...> &elements, Function function) {
        ForEachElementRecursion<0, Function, Ts...>(elements, function);
    }
};

template <typename Function, typename... Ts>
static constexpr void for_each_in(const std::tuple<Ts...> &elements, Function function) {
    ForEachIn<Ts...>::ForEachElement(elements, function);
}

template <typename T>
constexpr auto getTypeView() {
    std::string_view view;
#ifdef _MSC_VER
#ifndef __clang__
    view = __FUNCSIG__;
    view.remove_prefix(view.find_first_of("<") + 1);
    view.remove_suffix(view.size() - view.find_last_of(">"));
#else
    view = __PRETTY_FUNCTION__;
    view.remove_prefix(view.find_first_of("=") + 1);
    view.remove_prefix(view.find_first_not_of(" "));
    view.remove_suffix(view.size() - view.find_last_of("]"));
#endif
#else
#ifdef __clang__
    view = __PRETTY_FUNCTION__;
    view.remove_prefix(view.find_first_of("=") + 1);
    view.remove_prefix(view.find_first_not_of(" "));
    view.remove_suffix(view.size() - view.find_last_of("]"));
#else
#ifdef __GNUC__
    view = __PRETTY_FUNCTION__;
    view.remove_prefix(view.find_first_of("=") + 1);
    view.remove_prefix(view.find_first_not_of(" "));
    view.remove_suffix(view.size() - view.find_last_of("]"));
#else
    view = "unknown";
#endif
#endif
#endif
    return view;
}

template <typename T>
struct TypeName {
    constexpr TypeName() : value() {
        auto view = getTypeView<T>();
        for (size_t i = 0; i < view.size(); i++) value[i] = view[i];

        value[view.size()] = '\0';
    }
    char value[getTypeView<T>().size() + 1];
};

template <typename T>
std::string TypeToStr() {
    return std::string(TypeName<T>().value);
}

template <class Adaptor>
const typename Adaptor::container_type &get_underlying_container(const Adaptor &adaptor) {
    struct AdaptorSubClass : Adaptor {
        static const typename Adaptor::container_type &get(const Adaptor &adaptor) { return adaptor.*(&AdaptorSubClass::c); }
    };
    return AdaptorSubClass::get(adaptor);
}
}  // namespace ExtendedTypeSupport

/// Contains support for working with reflected fields, the definition for the REFLECT macro and non-generic supporting macros
namespace reflection {
#define NOTE(field, ...) static constexpr auto field##_note{std::tuple{__VA_ARGS__}};
using NoAnnotation = std::tuple<>;

inline namespace Inheritance {
template <typename T, size_t SuperIndex, typename SuperAnnotations>
struct SuperInfo {
    using Type = T;
    using Annotations = SuperAnnotations;
    const Annotations &annotations;
    static constexpr size_t Index = SuperIndex;

    template <typename Annotation>
    static constexpr bool HasAnnotation = ExtendedTypeSupport::has_type<Annotation, Annotations>::value;

    template <typename Annotation>
    constexpr Annotation getAnnotation() const {
        return ExtendedTypeSupport::get<Annotation>::from(annotations);
    }

    template <typename Annotation, typename Function>
    constexpr void forEach(Function function) const {
        return ExtendedTypeSupport::for_each<Annotation>::in(annotations, function);
    }

    template <typename Function>
    constexpr void forEachAnnotation(Function function) const {
        return ExtendedTypeSupport::for_each_in(annotations, function);
    }
};

template <typename T, typename... Ts>
struct NotedSuper {
    const std::tuple<Ts...> annotations;
    using Annotations = decltype(annotations);
};

template <typename T>
struct SuperClass {
    constexpr static NoAnnotation annotations{};
    using Annotations = decltype(annotations);

    template <typename... Ts>
    constexpr NotedSuper<T, Ts...> operator()(const Ts &...args) const {
        return NotedSuper<T, Ts...>{std::tuple<Ts...>(args...)};
    }
};

template <typename T>
static constexpr SuperClass<T> Super{};

template <typename T>
struct is_super {
    static constexpr bool value = false;
};
template <typename T>
struct is_super<const T> {
    static constexpr bool value = is_super<T>::value;
};
template <typename T>
struct is_super<SuperClass<T>> {
    static constexpr bool value = true;
};
template <typename T, typename... Ts>
struct is_super<NotedSuper<T, Ts...>> {
    static constexpr bool value = true;
};

template <typename T>
struct super_type {
    using type = void;
};
template <typename T>
struct super_type<const T> {
    using type = typename super_type<T>::type;
};
template <typename T>
struct super_type<SuperClass<T>> {
    using type = T;
};
template <typename T, typename... Ts>
struct super_type<NotedSuper<T, Ts...>> {
    using type = T;
};

template <size_t Count>
static constexpr size_t CountSupersRecursion() {
    return Count;
}

template <size_t Count, typename CurrentType, typename... NextTypes>
static constexpr size_t CountSupersRecursion() {
    if constexpr (is_super<CurrentType>::value)
        return CountSupersRecursion<1 + Count, NextTypes...>();
    else
        return CountSupersRecursion<Count, NextTypes...>();
}

template <typename... Ts>
struct count_supers {
    static constexpr size_t value = CountSupersRecursion<0, Ts...>();
};

template <typename SubClass, typename... Ts>
struct Inherit;

template <typename SubClass, typename... Ts>
struct Inherit<SubClass, const std::tuple<Ts...>> {
    template <size_t Index, size_t SuperIndex, typename Function>
    static void ForEachRecursion(const SubClass &, Function function) {}  // Base case for recursion

    template <size_t Index, size_t SuperIndex, typename Function, typename CurrentAnnotation, typename... NextAnnotations>
    static void ForEachRecursion(SubClass &object, Function function) {
        if constexpr (is_super<CurrentAnnotation>::value) {
            using SuperType = typename super_type<CurrentAnnotation>::type;
            function(SuperInfo<SuperType, SuperIndex, typename CurrentAnnotation::Annotations>{std::get<Index>(SubClass::Class::annotations).annotations}, (SuperType &)object);
            ForEachRecursion<Index + 1, SuperIndex + 1, Function, NextAnnotations...>(object, function);
        } else
            ForEachRecursion<Index + 1, SuperIndex, Function, NextAnnotations...>(object, function);
    }

    template <size_t Index, size_t SuperIndex, typename Function, typename CurrentAnnotation, typename... NextAnnotations>
    static void ForEachRecursion(const SubClass &object, Function function) {
        if constexpr (is_super<CurrentAnnotation>::value) {
            using SuperType = typename super_type<CurrentAnnotation>::type;
            function(SuperInfo<SuperType, SuperIndex, typename CurrentAnnotation::Annotations>{std::get<Index>(SubClass::Class::annotations).annotations}, (SuperType &)object);
            ForEachRecursion<Index + 1, SuperIndex + 1, Function, NextAnnotations...>(object, function);
        } else
            ForEachRecursion<Index + 1, SuperIndex, Function, NextAnnotations...>(object, function);
    }

    template <typename Function>
    static void ForEach(SubClass &object, Function function) {
        ForEachRecursion<0, 0, Function, Ts...>(object, function);
    }

    template <typename Function>
    static void ForEach(const SubClass &object, Function function) {
        ForEachRecursion<0, 0, Function, Ts...>(object, function);
    }

    template <size_t Index, size_t SuperIndex, typename Function>
    static constexpr void ForEachRecursion(Function function) {}  // Base case for recursion

    template <size_t Index, size_t SuperIndex, typename Function, typename CurrentAnnotation, typename... NextAnnotations>
    static constexpr void ForEachRecursion(Function function) {
        if constexpr (is_super<CurrentAnnotation>::value) {
            using SuperType = typename super_type<CurrentAnnotation>::type;
            function(SuperInfo<SuperType, SuperIndex, typename CurrentAnnotation::Annotations>{std::get<Index>(SubClass::Class::annotations).annotations});
            ForEachRecursion<Index + 1, SuperIndex + 1, Function, NextAnnotations...>(function);
        } else
            ForEachRecursion<Index + 1, SuperIndex, Function, NextAnnotations...>(function);
    }

    template <typename Function>
    static constexpr void ForEach(Function function) {
        ForEachRecursion<0, 0, Function, Ts...>(function);
    }

    template <size_t Index, size_t SuperIndex, typename Function>
    static void AtRecursion(const SubClass &object, size_t superIndex, Function function) {}  // Base case for recursion

    template <size_t Index, size_t SuperIndex, typename Function, typename CurrentAnnotation, typename... NextAnnotations>
    static void AtRecursion(SubClass &object, size_t superIndex, Function function) {
        if constexpr (is_super<CurrentAnnotation>::value) {
            if (SuperIndex == superIndex) {
                using SuperType = typename super_type<CurrentAnnotation>::type;
                function(SuperInfo<SuperType, SuperIndex, typename CurrentAnnotation::Annotations>{std::get<Index>(SubClass::Class::annotations).annotations}, (SuperType &)object);
            } else
                AtRecursion<Index + 1, SuperIndex + 1, Function, NextAnnotations...>(object, superIndex, function);
        } else
            AtRecursion<Index + 1, SuperIndex, Function, NextAnnotations...>(object, superIndex, function);
    }

    template <size_t Index, size_t SuperIndex, typename Function, typename CurrentAnnotation, typename... NextAnnotations>
    static void AtRecursion(const SubClass &object, size_t superIndex, Function function) {
        if constexpr (is_super<CurrentAnnotation>::value) {
            if (SuperIndex == superIndex) {
                using SuperType = typename super_type<CurrentAnnotation>::type;
                function(SuperInfo<SuperType, SuperIndex, typename CurrentAnnotation::Annotations>{std::get<Index>(SubClass::Class::annotations).annotations}, (SuperType &)object);
            } else
                AtRecursion<Index + 1, SuperIndex + 1, Function, NextAnnotations...>(object, superIndex, function);
        } else
            AtRecursion<Index + 1, SuperIndex, Function, NextAnnotations...>(object, superIndex, function);
    }

    template <typename Function>
    static void At(SubClass &object, size_t superIndex, Function function) {
        AtRecursion<0, 0, Function, Ts...>(object, superIndex, function);
    }

    template <typename Function>
    static void At(const SubClass &object, size_t superIndex, Function function) {
        AtRecursion<0, 0, Function, Ts...>(object, superIndex, function);
    }

    template <size_t Index, size_t SuperIndex, typename Function>
    static constexpr void AtRecursion(size_t superIndex, Function function) {}  // Base case for recursion

    template <size_t Index, size_t SuperIndex, typename Function, typename CurrentAnnotation, typename... NextAnnotations>
    static constexpr void AtRecursion(size_t superIndex, Function function) {
        if constexpr (is_super<CurrentAnnotation>::value) {
            if (SuperIndex == superIndex) {
                using SuperType = typename super_type<CurrentAnnotation>::type;
                function(SuperInfo<SuperType, SuperIndex, typename CurrentAnnotation::Annotations>{std::get<Index>(SubClass::Class::annotations).annotations});
            } else
                AtRecursion<Index + 1, SuperIndex + 1, Function, NextAnnotations...>(superIndex, function);
        } else
            AtRecursion<Index + 1, SuperIndex, Function, NextAnnotations...>(superIndex, function);
    }

    template <typename Function>
    static constexpr void At(size_t superIndex, Function function) {
        AtRecursion<0, 0, Function, Ts...>(superIndex, function);
    }

    static constexpr size_t TotalSupers = count_supers<Ts...>::value;
};

}  // namespace Inheritance

namespace Fields {
template <typename T = void, typename FieldDescr = void, typename FieldPointer = std::nullptr_t, size_t FieldIndex = 0, typename Annotations = NoAnnotation, const char *FieldName = nullptr>
struct Field;

template <>
struct Field<void, void, std::nullptr_t, 0, NoAnnotation, nullptr> {
    const char *name;
    const char *typeStr;
};

template <typename T, typename FieldDescr, typename FieldPointer, size_t FieldIndex, typename FieldAnnotations, const char *FieldName>
struct Field {
    static constexpr const char *Name = FieldName;
    const char *name;
    const char *typeStr;

    using Type = T;
    using Pointer = std::conditional_t<std::is_reference_v<T>, std::nullptr_t, FieldPointer>;
    using Annotations = FieldAnnotations;

    Pointer p;
    const Annotations &annotations;

    static constexpr size_t Index = FieldIndex;
    static constexpr bool IsStatic = !std::is_member_pointer<FieldPointer>::value && !std::is_same<std::nullptr_t, FieldPointer>::value;
    static constexpr bool IsFunction = std::is_function_v<T> || std::is_same_v<T, FieldPointer>;
    static constexpr bool HasOffset = !IsStatic && !IsFunction && !std::is_reference_v<T>;
    static constexpr size_t getOffset() { return FieldDescr::template Get<HasOffset>::offset(); }  // Returns 0 if HasOffset is false

    template <typename Annotation>
    static constexpr bool HasAnnotation = ExtendedTypeSupport::has_type<Annotation, Annotations>::value;

    template <typename Annotation>
    constexpr Annotation getAnnotation() const {
        return ExtendedTypeSupport::get<Annotation>::from(annotations);
    }

    template <typename Annotation, typename Function>
    constexpr void forEach(Function function) const {
        return ExtendedTypeSupport::for_each<Annotation>::in(annotations, function);
    }

    template <typename Function>
    constexpr void forEachAnnotation(Function function) const {
        return ExtendedTypeSupport::for_each_in(annotations, function);
    }
};
}  // namespace Fields

struct Unproxied {};

template <typename T>
struct Proxy : public Unproxied {};

template <typename T>
struct is_proxied {
    static constexpr bool value = !std::is_base_of<Unproxied, Proxy<T>>::value;
};

template <typename Type>
struct unproxy {
    using T = Type;
};
template <typename Type>
struct unproxy<Proxy<Type>> {
    using T = Type;
};
template <typename Type>
using unproxy_t = typename unproxy<Type>::T;

#define GET_FIELD_NAME(x) x,

#ifdef __clang__
#define CLANG_ONLY(x)                                              \
    template <typename T_, typename F, typename = decltype(T_::x)> \
    static constexpr void useInst(F f, T_ &t) {                    \
        f(field, t.x);                                             \
    }                                                              \
    template <typename T_, typename F, typename = decltype(T_::x)> \
    static constexpr void useInst(F f, const T_ &t) {              \
        f(field, t.x);                                             \
    }                                                              \
    template <typename T_, typename F>                             \
    static constexpr inline void useInst(F f, ...) {}
#define USE_FIELD_VALUE(x) \
    if constexpr (!x##_::Field::IsFunction) x##_::template useInst<ClassType>(function, object);
#define USE_FIELD_VALUE_AT(x)                                                                        \
    case IndexOf::x:                                                                                 \
        if constexpr (!x##_::Field::IsFunction) x##_::template useInst<ClassType>(function, object); \
        break;
#else
#define CLANG_ONLY(x)
#define USE_FIELD_VALUE(x) \
    if constexpr (!x##_::Field::IsFunction) function(x##_::field, object.x);
#define USE_FIELD_VALUE_AT(x)                                                    \
    case IndexOf::x:                                                             \
        if constexpr (!x##_::Field::IsFunction) function(x##_::field, object.x); \
        break;
#endif

#define DESCRIBE_FIELD(x)                                                                                                    \
    struct x##_ {                                                                                                            \
        template <typename T_>                                                                                               \
        static constexpr ExtendedTypeSupport::TypePair<decltype(T_::x), decltype(&T_::x)> identify(int);                     \
        template <typename T_>                                                                                               \
        static constexpr ExtendedTypeSupport::TypePair<decltype(T_::x), std::nullptr_t> identify(unsigned int);              \
        template <typename T_>                                                                                               \
        static constexpr ExtendedTypeSupport::TypePair<decltype(&T_::x), decltype(&T_::x)> identify(...);                    \
        using Type = typename decltype(identify<ProxyType>(0))::Left;                                                        \
        using Pointer = typename decltype(identify<ProxyType>(0))::Right;                                                    \
        template <typename T_, bool IsReference>                                                                             \
        struct GetPointer {                                                                                                  \
            static constexpr auto value = &T_::x;                                                                            \
        };                                                                                                                   \
        template <typename T_>                                                                                               \
        struct GetPointer<T_, true> {                                                                                        \
            static constexpr std::nullptr_t value = nullptr;                                                                 \
        };                                                                                                                   \
        static constexpr const char nameStr[] = #x;                                                                          \
        static constexpr auto typeStr = ExtendedTypeSupport::TypeName<Type>();                                               \
        template <typename T_>                                                                                               \
        static constexpr decltype(T_::x##_note) idNote(int);                                                                 \
        template <typename T_>                                                                                               \
        static constexpr decltype(Class::NoNote) idNote(...);                                                                \
        template <typename T_, bool NoNote>                                                                                  \
        struct GetNote {                                                                                                     \
            static constexpr auto &value = Class::NoNote;                                                                    \
        };                                                                                                                   \
        template <typename T_>                                                                                               \
        struct GetNote<T_, false> {                                                                                          \
            static constexpr auto &value = T_::x##_note;                                                                     \
        };                                                                                                                   \
        using NoteType = decltype(idNote<ProxyType>(0));                                                                     \
        template <bool HasOffset, typename T_ = ClassType>                                                                   \
        struct Get {                                                                                                         \
            static constexpr size_t offset() { return offsetof(T_, x); }                                                     \
        };                                                                                                                   \
        template <typename T_>                                                                                               \
        struct Get<false, T_> {                                                                                              \
            static constexpr size_t offset() { return size_t(0); }                                                           \
        };                                                                                                                   \
        using Field = reflection::Fields::Field<Type, x##_, Pointer, IndexOf::x, NoteType, nameStr>;                         \
        static constexpr Field field = {nameStr, &typeStr.value[0], GetPointer<ProxyType, std::is_reference_v<Type>>::value, \
                                        GetNote<ProxyType, std::is_same_v<decltype(Class::NoNote), NoteType>>::value};       \
        CLANG_ONLY(x)                                                                                                        \
    };

#define ADD_IF_STATIC(x) +(x##_::Field::IsStatic ? 1 : 0)
#define GET_FIELD(x) {Class::x##_::field.name, Class::x##_::field.typeStr},
#define USE_FIELD(x) function(x##_::field);
#define USE_FIELD_AT(x)        \
    case IndexOf::x:           \
        function(x##_::field); \
        break;

#pragma warning(disable : 4003)  // Not enough arguments warning generated despite macros working perfectly

/// After the objectType there needs to be at least 1 and at most 125 field names
/// e.g. REFLECT(MyObj, myInt, myString, myOtherObj)
#define REFLECT(objectType, ...)                                                                               \
    struct Class {                                                                                             \
        using ProxyType = objectType;                                                                          \
        using ClassType = reflection::unproxy_t<ProxyType>;                                                    \
        enum_t(IndexOf, size_t, {FOR_EACH(GET_FIELD_NAME, __VA_ARGS__)});                                      \
        static constexpr reflection::NoAnnotation NoNote{};                                                    \
        using Annotations = decltype(NoNote);                                                                  \
        static constexpr Annotations &annotations = NoNote;                                                    \
        FOR_EACH(DESCRIBE_FIELD, __VA_ARGS__)                                                                  \
        static constexpr size_t TotalFields = COUNT_ARGUMENTS(__VA_ARGS__);                                    \
        static constexpr size_t TotalStaticFields = 0 FOR_EACH(ADD_IF_STATIC, __VA_ARGS__);                    \
        static constexpr reflection::Fields::Field<> Fields[TotalFields] = {FOR_EACH(GET_FIELD, __VA_ARGS__)}; \
        template <typename Function>                                                                           \
        constexpr static void ForEachField(Function function) {                                                \
            FOR_EACH(USE_FIELD, __VA_ARGS__)                                                                   \
        }                                                                                                      \
        template <typename Function>                                                                           \
        static void ForEachField(ClassType &object, Function function) {                                       \
            FOR_EACH(USE_FIELD_VALUE, __VA_ARGS__)                                                             \
        }                                                                                                      \
        template <typename Function>                                                                           \
        static void ForEachField(const ClassType &object, Function function) {                                 \
            FOR_EACH(USE_FIELD_VALUE, __VA_ARGS__)                                                             \
        }                                                                                                      \
        template <typename Function>                                                                           \
        constexpr static void FieldAt(size_t fieldIndex, Function function) {                                  \
            switch (fieldIndex) { FOR_EACH(USE_FIELD_AT, __VA_ARGS__) }                                        \
        }                                                                                                      \
        template <typename Function>                                                                           \
        static void FieldAt(ClassType &object, size_t fieldIndex, Function function) {                         \
            switch (fieldIndex) { FOR_EACH(USE_FIELD_VALUE_AT, __VA_ARGS__) }                                  \
        }                                                                                                      \
    };                                                                                                         \
    using Supers = reflection::Inherit<typename Class::ClassType, typename Class::Annotations>;

/// REFLECT_NOTED is exactly the same as REFLECT except this signals that objectType itself is annotated
#define REFLECT_NOTED(objectType, ...)                                                                         \
    struct Class {                                                                                             \
        using ProxyType = objectType;                                                                          \
        using ClassType = reflection::unproxy_t<ProxyType>;                                                    \
        enum_t(IndexOf, size_t, {FOR_EACH(GET_FIELD_NAME, __VA_ARGS__)});                                      \
        static constexpr reflection::NoAnnotation NoNote{};                                                    \
        using Annotations = decltype(objectType##_note);                                                       \
        static constexpr Annotations &annotations = objectType##_note;                                         \
        FOR_EACH(DESCRIBE_FIELD, __VA_ARGS__)                                                                  \
        static constexpr size_t TotalFields = COUNT_ARGUMENTS(__VA_ARGS__);                                    \
        static constexpr size_t TotalStaticFields = 0 FOR_EACH(ADD_IF_STATIC, __VA_ARGS__);                    \
        static constexpr reflection::Fields::Field<> Fields[TotalFields] = {FOR_EACH(GET_FIELD, __VA_ARGS__)}; \
        template <typename Function>                                                                           \
        constexpr static void ForEachField(Function function) {                                                \
            FOR_EACH(USE_FIELD, __VA_ARGS__)                                                                   \
        }                                                                                                      \
        template <typename Function>                                                                           \
        static void ForEachField(ClassType &object, Function function) {                                       \
            FOR_EACH(USE_FIELD_VALUE, __VA_ARGS__)                                                             \
        }                                                                                                      \
        template <typename Function>                                                                           \
        static void ForEachField(const ClassType &object, Function function) {                                 \
            FOR_EACH(USE_FIELD_VALUE, __VA_ARGS__)                                                             \
        }                                                                                                      \
        template <typename Function>                                                                           \
        constexpr static void FieldAt(size_t fieldIndex, Function function) {                                  \
            switch (fieldIndex) { FOR_EACH(USE_FIELD_AT, __VA_ARGS__) }                                        \
        }                                                                                                      \
        template <typename Function>                                                                           \
        static void FieldAt(ClassType &object, size_t fieldIndex, Function function) {                         \
            switch (fieldIndex) { FOR_EACH(USE_FIELD_VALUE_AT, __VA_ARGS__) }                                  \
        }                                                                                                      \
    };                                                                                                         \
    using Supers = reflection::Inherit<typename Class::ClassType, typename Class::Annotations>;

/// REFLECT_EMPTY is used to reflect an annotated class with no reflected fields
#define REFLECT_EMPTY(objectType)                                                       \
    struct Class {                                                                      \
        using ProxyType = objectType;                                                   \
        using ClassType = reflection::unproxy_t<ProxyType>;                             \
        using Annotations = decltype(objectType##_note);                                \
        static constexpr Annotations &annotations = objectType##_note;                  \
        static constexpr size_t TotalFields = 0;                                        \
        static constexpr size_t TotalStaticFields = 0;                                  \
        static constexpr reflection::Fields::Field<> Fields[1] = {{"", ""}};            \
        template <typename Function>                                                    \
        constexpr static void ForEachField(Function function) {}                        \
        template <typename Function>                                                    \
        static void ForEachField(ClassType &object, Function function) {}               \
        template <typename Function>                                                    \
        static void ForEachField(const ClassType &object, Function function) {}         \
        template <typename Function>                                                    \
        constexpr static void FieldAt(size_t fieldIndex, Function function) {}          \
        template <typename Function>                                                    \
        static void FieldAt(ClassType &object, size_t fieldIndex, Function function) {} \
    };                                                                                  \
    using Supers = reflection::Inherit<typename Class::ClassType, typename Class::Annotations>;

template <typename T, typename = decltype(T::Class::TotalFields)>
static constexpr std::true_type typeHasReflection(int);
template <typename T>
static constexpr std::false_type typeHasReflection(unsigned int);

template <typename T>
struct is_reflected {
    static constexpr bool value = is_proxied<T>::value || decltype(typeHasReflection<T>(0))::value;
};
template <typename T>
struct is_reflected<const T> {
    static constexpr bool value = is_reflected<T>::value;
};
template <typename T>
inline constexpr bool is_reflected_v = is_reflected<T>::value;

template <typename T>
struct reflected_type {
    using type = typename std::conditional_t<is_proxied<T>::value, Proxy<T>, T>;
};
template <typename T>
using reflected_type_t = typename reflected_type<T>::type;

template <typename Type>
struct clazz {
    using type = typename reflected_type_t<Type>::Class;
};
template <typename Type>
using class_t = typename clazz<Type>::type;

template <typename Type>
struct supers {
    using type = typename reflected_type_t<Type>::Supers;
};
template <typename Type>
using supers_t = typename supers<Type>::type;

template <typename T, typename Enable = void>
struct class_notes {
    using type = reflection::NoAnnotation;
};
template <typename T>
struct class_notes<T, std::enable_if_t<is_reflected_v<T>>> {
    using type = typename class_t<T>::Annotations;
};
template <typename T>
using class_notes_t = typename class_notes<T>::type;
}  // namespace reflection

namespace ObjectMapper {
#define OM_R(l, r) ObjectMapper::map(o.r, this->l);
#define OM_S(l, r) ObjectMapper::map(this->l, o.r);
#define OM_O(a) OM_R a
#define OM_T(a) OM_S a

/// Defines a mapping from "this" to objectType
/// After the objectType there needs to be at least 1 and at most 124 parenthesized field mappings ("this" fields on left, objectType fields on right)
/// e.g. MAP_TO(target, (a, a), (b, c), (d, d))
#define MAP_TO(objectType, ...) \
    void map_to(objectType &o) const { FOR_EACH(OM_O, __VA_ARGS__) }

/// Defines a mapping from objectType to "this"
/// After the objectType there needs to be at least 1 and at most 124 parenthesized field mappings ("this" fields on left, objectType fields on right)
/// e.g. MAP_FROM(target, (a, a), (b, c), (d, d))
#define MAP_FROM(objectType, ...) \
    void map_from(const objectType &o) { FOR_EACH(OM_T, __VA_ARGS__) }

/// Defines a bi-directional mapping between "this" and objectType
/// After the objectType there needs to be at least 1 and at most 124 parenthesized field mappings ("this" fields on left, objectType fields on right)
/// e.g. MAP_WITH(target, (a, a), (b, c), (d, d))
#define MAP_WITH(objectType, ...) MAP_TO(objectType, __VA_ARGS__) MAP_FROM(objectType, __VA_ARGS__)

/// Default mapping implementation, safe to call from ObjectMapper::map specializations, assignment & conversion operators, and map_to/map_from methods
/// Do not specialize this method
template <typename To, typename From>
constexpr inline void map_default(To &, const From &);

/// Default mapping helper, safe to call from ObjectMapper::map specializations, assignment & conversion operators, and map_to/map_from methods
/// Do not specialize this method
template <typename To, typename From>
constexpr inline To map_default(const From &from) {
    To to;
    ObjectMapper::map_default(to, from);
    return to;
}

/// If any mapping exists from "from" to "to", "to" is assigned mapped values from "from", if no mapping exists, "to" is unchanged and nothing is thrown
/// A mapping may exist if...
/// - Both "To" and "From" are reflected objects and have fields with identical names and compatible types
/// - Both "To" and "From" are compatible types ("to = from" or "to = static_cast<To>(from)" is valid)
/// - Both "To" and "From" are compatible pairs, tuples, array, or STL containers
/// - An assignment operator, converting constructor, or conversion operator exists
/// - A map_to or map_from member method exists in "to" or "from" (which may or may not have been generated by the MAP_TO/MAP_FROM/MAP_WITH macros)
/// - This method was specialized
///
/// To avoid infinite recursion, call ObjectMapper::map_default instead of ObjectMapper::map when you want default mappings in...
/// ObjectMapper::map specializations, assignment operators, conversion operators, converting constructors, map_to, or map_from methods with the same types
template <typename To, typename From>
constexpr inline void map(To &to, const From &from) {
    if constexpr (std::is_const_v<To>)
        return;
    else if constexpr (ExtendedTypeSupport::HasMapFrom<To, From>)
        to.map_from(from);
    else if constexpr (ExtendedTypeSupport::HasMapTo<From, To>)
        from.map_to(to);
    else if constexpr (ExtendedTypeSupport::IsAssignable<decltype(to), decltype(from)>)
        to = from;
    else if constexpr (ExtendedTypeSupport::IsStaticCastAssignable<decltype(to), decltype(from)>)
        to = static_cast<To>(from);
    else
        ObjectMapper::map_default(to, from);
}

/// Helper method for ObjectMapper::map(To &, From &); do not specialize this method
template <typename To, typename From>
constexpr inline To map(const From &from) {
    To to;
    ObjectMapper::map(to, from);
    return to;
}

/// Helper method for ObjectMapper::map_default(To &, From &); do not specialize this method
template <size_t Index, typename... To, typename... From>
constexpr inline void map_tuple(std::tuple<To...> &to, const std::tuple<From...> &from) {
    if constexpr (Index < sizeof...(To) && Index < sizeof...(From)) {
        ObjectMapper::map(std::get<Index>(to), std::get<Index>(from));
        ObjectMapper::map_tuple<Index + 1>(to, from);
    }
}

template <typename To, typename From>
constexpr inline void map_default(To &to, const From &from) {
    if constexpr (std::is_const_v<To>)
        return;
    else if constexpr (ExtendedTypeSupport::is_pointable<To>::value) {
        using ToDereferenced = typename ExtendedTypeSupport::remove_pointer<To>::type;
        if (to == nullptr) {
            if constexpr (ExtendedTypeSupport::is_pointable<From>::value) {
                if (from == nullptr)
                    return;
                else if constexpr (std::is_same_v<std::shared_ptr<ToDereferenced>, To>) {
                    if constexpr (std::is_same_v<From, To>)
                        to = from;  // Share shared pointer
                    else {
                        to = ME::create_ref<ToDereferenced>();
                        ObjectMapper::map(*to, *from);
                    }
                } else if constexpr (std::is_same_v<std::unique_ptr<ToDereferenced>, To>) {
                    to = std::make_unique<ToDereferenced>();
                    ObjectMapper::map(*to, *from);
                }
            } else if constexpr (std::is_same_v<std::shared_ptr<ToDereferenced>, To>) {
                to = ME::create_ref<ToDereferenced>();
                ObjectMapper::map(*to, from);
            } else if constexpr (std::is_same_v<std::unique_ptr<ToDereferenced>, To>) {
                to = std::make_unique<ToDereferenced>();
                ObjectMapper::map(*to, from);
            }
        } else  // to != nullptr
        {
            if constexpr (ExtendedTypeSupport::is_pointable<From>::value) {
                if (from == nullptr)
                    to = nullptr;
                else
                    ObjectMapper::map(*to, *from);
            } else
                ObjectMapper::map(*to, from);
        }
    } else if constexpr (ExtendedTypeSupport::is_pointable<From>::value) {
        if (from != nullptr) ObjectMapper::map(to, *from);
    } else if constexpr (ExtendedTypeSupport::is_pair<To>::value && ExtendedTypeSupport::is_pair<From>::value) {
        ObjectMapper::map(to.first, from.first);
        ObjectMapper::map(to.second, from.second);
    } else if constexpr (ExtendedTypeSupport::is_tuple<To>::value && ExtendedTypeSupport::is_tuple<From>::value) {
        ObjectMapper::map_tuple<0>(to, from);
    } else if constexpr (ExtendedTypeSupport::is_iterable<To>::value && ExtendedTypeSupport::is_iterable<From>::value) {
        using ToElementType = typename ExtendedTypeSupport::element_type<To>::type;
        using FromElementType = typename ExtendedTypeSupport::element_type<From>::type;
        if constexpr ((ExtendedTypeSupport::is_stl_iterable<To>::value || ExtendedTypeSupport::is_adaptor<To>::value) &&
                      (ExtendedTypeSupport::is_stl_iterable<From>::value || ExtendedTypeSupport::is_adaptor<From>::value)) {
            ExtendedTypeSupport::Clear(to);
            for (auto &fromElement : from) {
                ToElementType toElement;
                ObjectMapper::map(toElement, fromElement);
                ExtendedTypeSupport::Append(to, toElement);
            }
        } else if constexpr (ExtendedTypeSupport::is_static_array<From>::value && ExtendedTypeSupport::static_array_size<From>::value > 0) {
            if constexpr (ExtendedTypeSupport::is_static_array<To>::value && ExtendedTypeSupport::static_array_size<To>::value > 0) {
                constexpr size_t limit = std::min(ExtendedTypeSupport::static_array_size<To>::value, ExtendedTypeSupport::static_array_size<From>::value);
                for (size_t i = 0; i < limit; i++) ObjectMapper::map(to[i], from[i]);
            } else {
                ExtendedTypeSupport::Clear(to);
                for (size_t i = 0; i < ExtendedTypeSupport::static_array_size<From>::value; i++) {
                    ToElementType toElement;
                    ObjectMapper::map(toElement, from[i]);
                    ExtendedTypeSupport::Append(to, toElement);
                }
            }
        } else if constexpr (ExtendedTypeSupport::is_static_array<To>::value && ExtendedTypeSupport::static_array_size<To>::value > 0) {
            size_t i = 0;
            for (auto &element : from) {
                ObjectMapper::map(to[i], element);
                if (++i == ExtendedTypeSupport::static_array_size<To>::value) break;
            }
        }
    } else if constexpr (reflection::is_reflected<To>::value && reflection::is_reflected<From>::value) {
        reflection::class_t<To>::ForEachField(to, [&](auto &toField, auto &toValue) {
            reflection::class_t<From>::ForEachField(from, [&](auto &fromField, auto &fromValue) {
                if (std::string_view(toField.Name) == std::string_view(fromField.Name)) ObjectMapper::map(toValue, fromValue);
            });
        });
    }
}

inline namespace Annotations {
template <typename MappedBy, typename Type = void>
struct MappedByType {
    using Object = Type;
    using DefaultMapping = MappedBy;
};
template <typename MappedBy>
struct MappedByType<MappedBy, void> {
    using DefaultMapping = MappedBy;
};

/// Field or class-level annotation saying a field or class should be mapped to "T" for activities like serialization
template <typename T>
static constexpr MappedByType<T, void> MappedBy{};

/// Operation annotation saying type "T" should be mapped to type "MappedBy" for activities like serialization
template <typename T, typename MappedBy>
using UseMapping = MappedByType<MappedBy, T>;

template <typename T>
inline constexpr bool IsMappedByNotes = ExtendedTypeSupport::type_list<T>::template has_specialization_v<MappedByType>;
template <typename T>
inline constexpr bool IsMappedByClassNote = IsMappedByNotes<reflection::class_notes_t<T>>;

template <typename T, typename Enable = void>
struct GetMappingFromNotes {
    using type = void;
};
template <typename T>
struct GetMappingFromNotes<T, std::enable_if_t<IsMappedByNotes<T>>> {
    using type = typename ExtendedTypeSupport::type_list<T>::template get_specialization_t<MappedByType>::DefaultMapping;
};

template <typename T, typename Enable = void>
struct GetMappingByClassNote {
    using type = void;
};
template <typename T>
struct GetMappingByClassNote<T, std::enable_if_t<IsMappedByClassNote<T>>> {
    using type = typename GetMappingFromNotes<reflection::class_notes_t<T>>::type;
};

template <typename T>
struct SetTags {
    using DefaultMapping = void;
};  // e.g. struct ObjectMapper::SetTags<Foo> : IsMappedBy<Bar> {};
template <typename T>
using GetTags = SetTags<T>;  // e.g. ObjectMapper::GetProperty<Foo>::MappedBy::DefaultMapping
template <typename T>
using IsMappedBy = MappedByType<T>;  // Tags a type "T" to be mapped to for activities like serialization
/// Sets default type "mappedBy" which this object "object" should be mapped to for activities like serialization
#define SET_DEFAULT_OBJECT_MAPPING(object, mappedBy) \
    template <>                                      \
    struct ObjectMapper::SetTags<object> : IsMappedBy<mappedBy> {};

template <typename T>
inline constexpr bool IsMappedByTags = !std::is_void_v<typename GetTags<T>::DefaultMapping>;

template <typename T, typename Note, typename Enable = void>
struct IsUseMappingNote {
    static constexpr bool value = false;
};
template <typename T, typename Note>
struct IsUseMappingNote<T, Note, std::enable_if_t<ExtendedTypeSupport::is_specialization_v<Note, MappedByType>>> {
    static constexpr bool value = std::is_same_v<T, typename Note::Object>;
};

template <typename T, typename Notes>
struct HasUseMappingNote {
    static constexpr bool value = false;
};
template <typename T, typename... Ts>
struct HasUseMappingNote<T, std::tuple<Ts...>> {
    static constexpr bool value = (IsUseMappingNote<T, Ts>::value || ...);
};

template <typename T, typename Notes, typename Enable = void>
struct GetOpNoteMapping {
    using type = void;
};
template <typename T, typename Notes>
struct GetOpNoteMapping<T, Notes, std::enable_if_t<HasUseMappingNote<T, Notes>::value>> {
    using type = typename ExtendedTypeSupport::type_list<Notes>::template get_specialization_t<MappedByType>::DefaultMapping;
};

/// Checks whether the type "T" has a type to use for default mappings for activies like serialization
template <typename T, typename FieldNotes = void, typename OpNotes = void>
inline constexpr bool HasDefaultMapping = HasUseMappingNote<T, OpNotes>::value || IsMappedByNotes<FieldNotes> || IsMappedByTags<T> || IsMappedByClassNote<T>;

/// Gets the type which "T" should be mapped to for activities like serialization (or void if no default exists)
template <typename T, typename FieldNotes = void, typename OpNotes = void>
using GetDefaultMapping = ExtendedTypeSupport::if_void_t<
        typename GetOpNoteMapping<T, OpNotes>::type,
        ExtendedTypeSupport::if_void_t<typename GetMappingFromNotes<FieldNotes>::type, ExtendedTypeSupport::if_void_t<typename GetTags<T>::DefaultMapping, typename GetMappingByClassNote<T>::type>>>;
}  // namespace Annotations
};  // namespace ObjectMapper

#pragma endregion DR

#pragma region ME::meta

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ME_CREFLECT_TYPES_STRUCT = 1,
    ME_CREFLECT_TYPES_STRING = 2,
    ME_CREFLECT_TYPES_INTEGER = 3,
    ME_CREFLECT_TYPES_FLOAT = 4,
    ME_CREFLECT_TYPES_DOUBLE = 5,
    ME_CREFLECT_TYPES_POINTER = 6
} ME_CREFLECT_Types;

struct _ME_CREFLECT_FieldInfo {
    const char *field_type;
    const char *field_name;
    size_t size;
    size_t offset;
    int is_signed;
    int array_size;
    ME_CREFLECT_Types data_type;
};

typedef struct _ME_CREFLECT_FieldInfo ME_CREFLECT_FieldInfo;

struct _ME_CREFLECT_TypeInfo {
    const char *name;
    size_t fields_count;
    size_t size;
    size_t packed_size;
    ME_CREFLECT_FieldInfo *fields;
};

typedef struct _ME_CREFLECT_TypeInfo ME_CREFLECT_TypeInfo;

#define ME_CREFLECT_EXPAND_(X) X
#define ME_CREFLECT_EXPAND_VA_(...) __VA_ARGS__
#define ME_CREFLECT_FOREACH_1_(FNC, USER_DATA, ARG) FNC(ARG, USER_DATA)
#define ME_CREFLECT_FOREACH_2_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                  \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_1_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_3_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                  \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_2_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_4_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                  \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_3_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_5_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                  \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_4_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_6_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                  \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_5_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_7_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                  \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_6_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_8_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                  \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_7_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_9_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                  \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_8_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_10_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                   \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_9_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_11_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                   \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_10_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_12_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                   \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_11_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_13_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                   \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_12_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_14_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                   \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_13_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_15_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                   \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_14_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_16_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                   \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_15_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_17_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                   \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_16_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_18_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                   \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_17_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_19_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                   \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_18_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_20_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                   \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_19_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_21_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                   \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_20_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_22_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                   \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_21_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_23_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                   \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_22_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_24_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                   \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_23_(FNC, USER_DATA, __VA_ARGS__))
#define ME_CREFLECT_FOREACH_25_(FNC, USER_DATA, ARG, ...) \
    FNC(ARG, USER_DATA)                                   \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_FOREACH_24_(FNC, USER_DATA, __VA_ARGS__))

#define ME_CREFLECT_OVERRIDE_4(_1, _2, _3, _4, FNC, ...) FNC
#define ME_CREFLECT_OVERRIDE_4_PLACEHOLDER 1, 2, 3, 4
#define ME_CREFLECT_OVERRIDE_5(_1, _2, _3, _4, _5, FNC, ...) FNC
#define ME_CREFLECT_OVERRIDE_5_PLACEHOLDER ME_CREFLECT_OVERRIDE_4_PLACEHOLDER, 5
#define ME_CREFLECT_OVERRIDE_14(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, FNC, ...) FNC
#define ME_CREFLECT_OVERRIDE_14_PLACEHOLDER ME_CREFLECT_OVERRIDE_5_PLACEHOLDER, 6, 7, 8, 9, 10, 11, 12, 13, 14
#define ME_CREFLECT_OVERRIDE_20(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, FNC, ...) FNC
#define ME_CREFLECT_OVERRIDE_20_PLACEHOLDER ME_CREFLECT_OVERRIDE_14_PLACEHOLDER, 15, 16, 17, 18, 19, 20
#define ME_CREFLECT_OVERRIDE_25(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, FNC, ...) FNC
#define ME_CREFLECT_OVERRIDE_25_PLACEHOLDER ME_CREFLECT_OVERRIDE_20_PLACEHOLDER, 21, 22, 23, 24, 25

#define ME_CREFLECT_FOREACH(FNC, USER_DATA, ...)                                                                                                                                                      \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_OVERRIDE_25(__VA_ARGS__, ME_CREFLECT_FOREACH_25_, ME_CREFLECT_FOREACH_24_, ME_CREFLECT_FOREACH_23_, ME_CREFLECT_FOREACH_22_, ME_CREFLECT_FOREACH_21_,             \
                                                ME_CREFLECT_FOREACH_20_, ME_CREFLECT_FOREACH_19_, ME_CREFLECT_FOREACH_18_, ME_CREFLECT_FOREACH_17_, ME_CREFLECT_FOREACH_16_, ME_CREFLECT_FOREACH_15_, \
                                                ME_CREFLECT_FOREACH_14_, ME_CREFLECT_FOREACH_13_, ME_CREFLECT_FOREACH_12_, ME_CREFLECT_FOREACH_11_, ME_CREFLECT_FOREACH_10_, ME_CREFLECT_FOREACH_9_,  \
                                                ME_CREFLECT_FOREACH_8_, ME_CREFLECT_FOREACH_7_, ME_CREFLECT_FOREACH_6_, ME_CREFLECT_FOREACH_5_, ME_CREFLECT_FOREACH_4_, ME_CREFLECT_FOREACH_3_,       \
                                                ME_CREFLECT_FOREACH_2_, ME_CREFLECT_FOREACH_1_)(FNC, USER_DATA, __VA_ARGS__))

#define ME_CREFLECT_DECLARE_SIMPLE_FIELD_(IGNORE, TYPE, FIELD_NAME) TYPE FIELD_NAME;
#define ME_CREFLECT_DECLARE_ARRAY_FIELD_(IGNORE, TYPE, FIELD_NAME, ARRAY_SIZE) TYPE FIELD_NAME[ARRAY_SIZE];

#define ME_CREFLECT_DECLARE_FIELD_(...) \
    ME_CREFLECT_EXPAND_(ME_CREFLECT_OVERRIDE_4(__VA_ARGS__, ME_CREFLECT_DECLARE_ARRAY_FIELD_, ME_CREFLECT_DECLARE_SIMPLE_FIELD_, ME_CREFLECT_OVERRIDE_4_PLACEHOLDER)(__VA_ARGS__))

#define ME_CREFLECT_DECLARE_FIELD(X, USER_DATA) ME_CREFLECT_DECLARE_FIELD_ X

#define ME_CREFLECT_SIZEOF_(IGNORE, C_TYPE, ...) +sizeof(C_TYPE)
#define ME_CREFLECT_SIZEOF(X, USER_DATA) ME_CREFLECT_SIZEOF_ X

#define ME_CREFLECT_SUM(...) +1

#define ME_CREFLECT_IS_TYPE_SIGNED_(C_TYPE) (C_TYPE) - 1 < (C_TYPE)1
#define ME_CREFLECT_IS_SIGNED_STRUCT(C_TYPE) 0
#define ME_CREFLECT_IS_SIGNED_STRING(C_TYPE) ME_CREFLECT_IS_TYPE_SIGNED_(C_TYPE)
#define ME_CREFLECT_IS_SIGNED_INTEGER(C_TYPE) ME_CREFLECT_IS_TYPE_SIGNED_(C_TYPE)
#define ME_CREFLECT_IS_SIGNED_FLOAT(C_TYPE) ME_CREFLECT_IS_TYPE_SIGNED_(C_TYPE)
#define ME_CREFLECT_IS_SIGNED_DOUBLE(C_TYPE) ME_CREFLECT_IS_TYPE_SIGNED_(C_TYPE)
#define ME_CREFLECT_IS_SIGNED_POINTER(C_TYPE) 0

#define ME_CREFLECT_IS_SIGNED_(DATA_TYPE, CTYPE) ME_CREFLECT_IS_SIGNED_##DATA_TYPE(CTYPE)

#define ME_CREFLECT_ARRAY_FIELD_INFO_(TYPE_NAME, DATA_TYPE, C_TYPE, FIELD_NAME, ARRAY_SIZE) \
#C_TYPE, #FIELD_NAME, sizeof(C_TYPE) * ARRAY_SIZE, offsetof(TYPE_NAME, FIELD_NAME), ME_CREFLECT_IS_SIGNED_(DATA_TYPE, C_TYPE), ARRAY_SIZE, ME_CREFLECT_TYPES_##DATA_TYPE

#define ME_CREFLECT_SIMPLE_FIELD_INFO_(TYPE_NAME, DATA_TYPE, C_TYPE, FIELD_NAME) \
#C_TYPE, #FIELD_NAME, sizeof(C_TYPE), offsetof(TYPE_NAME, FIELD_NAME), ME_CREFLECT_IS_SIGNED_(DATA_TYPE, C_TYPE), -1, ME_CREFLECT_TYPES_##DATA_TYPE

#define ME_CREFLECT_FIELD_INFO_(...) \
    {ME_CREFLECT_EXPAND_(ME_CREFLECT_OVERRIDE_5(__VA_ARGS__, ME_CREFLECT_ARRAY_FIELD_INFO_, ME_CREFLECT_SIMPLE_FIELD_INFO_, ME_CREFLECT_OVERRIDE_5_PLACEHOLDER)(__VA_ARGS__))},

#define ME_CREFLECT_FIELD_INFO(X, USER_DATA) ME_CREFLECT_FIELD_INFO_(USER_DATA, ME_CREFLECT_EXPAND_VA_ X)

#ifdef ME_CREFLECT_IMPL

#define ME_CREFLECT_DEFINE_GET_METHOD(TYPE_NAME, ...)                                                                                                                                          \
    ME_CREFLECT_TypeInfo *ME_creflect_get_##TYPE_NAME##_type_info(void) {                                                                                                                      \
        static ME_CREFLECT_FieldInfo fields_info[ME_CREFLECT_FOREACH(ME_CREFLECT_SUM, 0, __VA_ARGS__)] = {ME_CREFLECT_FOREACH(ME_CREFLECT_FIELD_INFO, TYPE_NAME, __VA_ARGS__)};                \
        static ME_CREFLECT_TypeInfo type_info = {#TYPE_NAME, ME_CREFLECT_FOREACH(ME_CREFLECT_SUM, 0, __VA_ARGS__), sizeof(TYPE_NAME), ME_CREFLECT_FOREACH(ME_CREFLECT_SIZEOF, 0, __VA_ARGS__), \
                                                 fields_info};                                                                                                                                 \
        return &type_info;                                                                                                                                                                     \
    }

#else

#define ME_CREFLECT_DEFINE_GET_METHOD(TYPE_NAME, ...)

#endif

#define ME_CREFLECT_DEFINE_STRUCT(TYPE_NAME, ...)                        \
    typedef struct {                                                     \
        ME_CREFLECT_FOREACH(ME_CREFLECT_DECLARE_FIELD, 0, __VA_ARGS__)   \
    } TYPE_NAME;                                                         \
    ME_CREFLECT_TypeInfo *ME_creflect_get_##TYPE_NAME##_type_info(void); \
    ME_CREFLECT_DEFINE_GET_METHOD(TYPE_NAME, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

namespace IamAfuckingNamespace {
int func1(float f, char c);
void func2(void);
void func_log_info(std::string info);
}  // namespace IamAfuckingNamespace

void init_reflection();

#pragma region AnyFunction

namespace ME::meta {

// https://stackoverflow.com/questions/26107041/how-can-i-determine-the-return-type-of-a-c11-member-function

template <typename T>
struct return_type;
template <typename R, typename... Args>
struct return_type<R (*)(Args...)> {
    using type = R;
};
template <typename R, typename C, typename... Args>
struct return_type<R (C::*)(Args...)> {
    using type = R;
};
template <typename R, typename C, typename... Args>
struct return_type<R (C::*)(Args...) const> {
    using type = R;
};
template <typename R, typename C, typename... Args>
struct return_type<R (C::*)(Args...) volatile> {
    using type = R;
};
template <typename R, typename C, typename... Args>
struct return_type<R (C::*)(Args...) const volatile> {
    using type = R;
};
template <typename T>
using return_type_t = typename return_type<T>::type;

struct any_function {
public:
    struct type {
        const std::type_info *info;
        bool is_lvalue_reference, is_rvalue_reference;
        bool is_const, is_volatile;
        bool operator==(const type &r) const {
            return info == r.info && is_lvalue_reference == r.is_lvalue_reference && is_rvalue_reference == r.is_rvalue_reference && is_const == r.is_const && is_volatile == r.is_volatile;
        }
        bool operator!=(const type &r) const { return !(*this == r); }
        template <class T>
        static type capture() {
            return {&typeid(T), std::is_lvalue_reference<T>::value, std::is_rvalue_reference<T>::value, std::is_const<typename std::remove_reference<T>::type>::value,
                    std::is_volatile<typename std::remove_reference<T>::type>::value};
        }
    };

    class result {
        struct result_base {
            virtual ~result_base() {}
            virtual std::unique_ptr<result_base> clone() const = 0;
            virtual type get_type() const = 0;
            virtual void *get_address() = 0;
        };
        template <class T>
        struct typed_result : result_base {
            T x;
            typed_result(T x) : x(get((void *)&x, tag<T>{})) {}
            std::unique_ptr<result_base> clone() const { return std::unique_ptr<typed_result>(new typed_result(get((void *)&x, tag<T>{}))); }
            type get_type() const { return type::capture<T>(); }
            void *get_address() { return (void *)&x; }
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
        template <class T>
        T get_value() {
            ME_ASSERT(get_type() == type::capture<T>());
            return get(p->get_address(), tag<T>{});
        }

        template <class T>
        static result capture(T x) {
            result r;
            r.p.reset(new typed_result<T>(static_cast<T>(x)));
            return r;
        }
    };
    any_function() : result_type{} {}
    any_function(std::nullptr_t) : result_type{} {}
    template <class R, class... A>
    any_function(R (*p)(A...)) : any_function(p, tag<R>{}, tag<A...>{}, build_indices<sizeof...(A)>{}) {}
    template <class R, class... A>
    any_function(std::function<R(A...)> f) : any_function(f, tag<R>{}, tag<A...>{}, build_indices<sizeof...(A)>{}) {}
    template <class F>
    any_function(F f) : any_function(f, &F::operator()) {}

    explicit operator bool() const { return static_cast<bool>(func); }
    const std::vector<type> &get_parameter_types() const { return parameter_types; }
    const type &get_result_type() const { return result_type; }
    result invoke(void *const args[]) const { return func(args); }
    result invoke(std::initializer_list<void *> args) const { return invoke(args.begin()); }

private:
    template <class... T>
    struct tag {};
    template <std::size_t... IS>
    struct indices {};
    template <std::size_t N, std::size_t... IS>
    struct build_indices : build_indices<N - 1, N - 1, IS...> {};
    template <std::size_t... IS>
    struct build_indices<0, IS...> : indices<IS...> {};

    template <class T>
    static T get(void *arg, tag<T>) {
        return *reinterpret_cast<T *>(arg);
    }
    template <class T>
    static T &get(void *arg, tag<T &>) {
        return *reinterpret_cast<T *>(arg);
    }
    template <class T>
    static T &&get(void *arg, tag<T &&>) {
        return std::move(*reinterpret_cast<T *>(arg));
    }
    template <class F, class R, class... A, size_t... I>
    any_function(F f, tag<R>, tag<A...>, indices<I...>) : parameter_types({type::capture<A>()...}), result_type(type::capture<R>()) {
        func = [f](void *const args[]) mutable { return result::capture<R>(f(get(args[I], tag<A>{})...)); };
    }
    template <class F, class... A, size_t... I>
    any_function(F f, tag<void>, tag<A...>, indices<I...>) : parameter_types({type::capture<A>()...}), result_type(type::capture<void>()) {
        func = [f](void *const args[]) mutable { return f(get(args[I], tag<A>{})...), result{}; };
    }
    template <class F, class R>
    any_function(F f, tag<R>, tag<>, indices<>) : parameter_types({}), result_type(type::capture<R>()) {
        func = [f](void *const args[]) mutable { return result::capture<R>(f()); };
    }
    template <class F>
    any_function(F f, tag<void>, tag<>, indices<>) : parameter_types({}), result_type(type::capture<void>()) {
        func = [f](void *const args[]) mutable { return f(), result{}; };
    }
    template <class F, class R, class... A>
    any_function(F f, R (F::*p)(A...)) : any_function(f, tag<R>{}, tag<A...>{}, build_indices<sizeof...(A)>{}) {}
    template <class F, class R, class... A>
    any_function(F f, R (F::*p)(A...) const) : any_function(f, tag<R>{}, tag<A...>{}, build_indices<sizeof...(A)>{}) {}

    std::function<result(void *const *)> func;
    std::vector<type> parameter_types;
    type result_type;
};

}  // namespace ME::meta

#pragma endregion AnyFunction

namespace tmp {

// type list
template <typename... TS>
struct typelist {
    static constexpr auto size = sizeof...(TS);
};

template <typename T>
struct is_typelist : std::false_type {};

template <typename... TS>
struct is_typelist<typelist<TS...>> : std::true_type {};

// basic operations
template <typename T, typename TL>
struct push_back;
template <typename T, typename TL>
struct push_front;
template <typename TL>
struct pop_front;
template <typename TL, size_t I>
struct at;

template <typename T, typename... TS>
struct push_back<T, typelist<TS...>> {
    using type = typelist<TS..., T>;
};

template <typename T, typename... TS>
struct push_front<T, typelist<TS...>> {
    using type = typelist<T, TS...>;
};

template <typename T, typename... TS>
struct pop_front<typelist<T, TS...>> {
    using type = typelist<TS...>;
};

template <typename T, typename... TS>
struct at<typelist<T, TS...>, 0> {
    using type = T;
};

template <typename T, typename... TS, size_t I>
struct at<typelist<T, TS...>, I> {
    ME_STATIC_ASSERT(I < (1 + sizeof...(TS)), "Out of bounds access");
    using type = typename at<typelist<TS...>, I - 1>::type;
};

// 'filter'
template <typename TL, template <typename> class PRED>
struct filter;

template <template <typename> class PRED>
struct filter<typelist<>, PRED> {
    using type = typelist<>;
};

template <typename T, typename... TS, template <typename> class PRED>
struct filter<typelist<T, TS...>, PRED> {
    using remaining = typename filter<typelist<TS...>, PRED>::type;
    using type = typename std::conditional<PRED<T>::value, typename push_front<T, remaining>::type, remaining>::type;
};

// 'max' given a template binary predicate
template <typename TL, template <typename, typename> class PRED>
struct max;

template <typename T, template <typename, typename> class PRED>
struct max<typelist<T>, PRED> {
    using type = T;
};

template <typename... TS, template <typename, typename> class PRED>
struct max<typelist<TS...>, PRED> {
    using first = typename at<typelist<TS...>, 0>::type;
    using remaining_max = typename max<typename pop_front<typelist<TS...>>::type, PRED>::type;
    using type = typename std::conditional<PRED<first, remaining_max>::value, first, remaining_max>::type;
};

// 'find_ancestors'
namespace impl {

template <typename SRCLIST, typename DESTLIST>
struct find_ancestors {

    template <typename B>
    using negation = typename std::integral_constant<bool, !bool(B::value)>::type;

    template <typename T, typename U>
    using cmp = typename std::is_base_of<T, U>::type;
    using most_ancient = typename max<SRCLIST, cmp>::type;

    template <typename T>
    using not_most_ancient = typename negation<std::is_same<most_ancient, T>>::type;

    using type = typename find_ancestors<typename filter<SRCLIST, not_most_ancient>::type, typename push_back<most_ancient, DESTLIST>::type>::type;
};

template <typename DESTLIST>
struct find_ancestors<typelist<>, DESTLIST> {
    using type = DESTLIST;
};

}  // namespace impl

template <typename TL, typename T>
struct find_ancestors {
    ME_STATIC_ASSERT(is_typelist<TL>::value, "The first parameter is not a typelist");

    template <typename U>
    using base_of_T = typename std::is_base_of<U, T>::type;
    using src_list = typename filter<TL, base_of_T>::type;
    using type = typename impl::find_ancestors<src_list, typelist<>>::type;
};

}  // namespace tmp

using namespace tmp;

template <typename TL>
struct hierarchy_iterator {
    ME_STATIC_ASSERT(is_typelist<TL>::value, "Not a typelist");
    inline static void exec(void *_p) {
        using target_t = typename pop_front<TL>::type;
        if (auto ptr = static_cast<target_t *>(_p)) {
            printf("%s\n", typeid(typename at<TL, 0>::type).name());
            // LOG(INFO) << "hierarchy_iterator : " << typeid(typename at<TL, 0>::type).name();
            hierarchy_iterator<target_t>::exec(_p);
        }
    }
};

template <>
struct hierarchy_iterator<typelist<>> {
    inline static void exec(void *) {}
};

namespace ME::meta {

namespace detail {
// 构造一个可以隐式转换为任意类型的类型
template <class T>
struct any_converter {
    // 不能convert至自身
    template <class U, class = typename std::enable_if<!std::is_same<typename std::decay<T>::type, typename std::decay<U>::type>::value>::type>
    operator U() const noexcept;
};

template <class T, std::size_t I>
struct any_converter_tagged : any_converter<T> {};

// 判断T是否可以使用Args...进行聚合初始化：T{ std::declval<Args>()... }
template <class T, class... Args>
constexpr auto is_aggregate_constructible_impl(T &&, Args &&...args) -> decltype(T{{args}...}, std::true_type());
// 多加一个重载可以去掉讨厌的clang warning
template <class T, class Arg>
constexpr auto is_aggregate_constructible_impl(T &&, Arg &&args) -> decltype(T{args}, std::true_type());
// 这个函数千万别改成模板函数否则会死机的！
constexpr auto is_aggregate_constructible_impl(...) -> std::false_type;

template <class T, class... Args>
struct is_aggregate_constructible : decltype(is_aggregate_constructible_impl(std::declval<T>(), std::declval<Args>()...)) {};

template <class T, class Seq>
struct is_aggregate_constructible_with_n_args;
template <class T, std::size_t... I>
struct is_aggregate_constructible_with_n_args<T, std::index_sequence<I...>> : is_aggregate_constructible<T, any_converter_tagged<T, I>...> {};
// 这里添加一个元函数是用来支持沙雕GCC的，不知道为什么GCC里面变长模板参数不能作为嵌套的实参展开……
template <class T, std::size_t N>
struct is_aggregate_constructible_with_n_args_ex : is_aggregate_constructible_with_n_args<T, std::make_index_sequence<N>> {};

// （原）线性查找法
template <class T, class Seq = std::make_index_sequence<sizeof(T)>>
struct struct_member_count_impl1;
template <class T, std::size_t... I>
struct struct_member_count_impl1<T, std::index_sequence<I...>> : std::integral_constant<std::size_t, (... + !!(is_aggregate_constructible_with_n_args_ex<T, I + 1>::value))> {};

template <class B, class T, class U>
struct lazy_conditional : lazy_conditional<typename B::type, T, U> {};
template <class T, class U>
struct lazy_conditional<std::true_type, T, U> : T {};
template <class T, class U>
struct lazy_conditional<std::false_type, T, U> : U {};

// 二分查找法
template <class T, class Seq = std::index_sequence<0>>
struct struct_member_count_impl2;
template <class T, std::size_t... I>
struct struct_member_count_impl2<T, std::index_sequence<I...>>
    : lazy_conditional<std::conjunction<is_aggregate_constructible_with_n_args_ex<T, I + 1>...>, struct_member_count_impl2<T, std::index_sequence<I..., (I + sizeof...(I))...>>,
                       std::integral_constant<std::size_t, (... + !!(is_aggregate_constructible_with_n_args_ex<T, I + 1>::value))>>::type {};

}  // namespace detail

template <class T>
struct StructMemberCount : detail::struct_member_count_impl2<T> {};

#include "enum_pp.hpp"

namespace detail {

#define APPLYER_DEF(N)                                                                          \
    template <class T, class ApplyFunc>                                                         \
    auto StructApply_impl(T &&my_struct, ApplyFunc f, std::integral_constant<std::size_t, N>) { \
        auto &&[ENUM_PARAMS(x, N)] = std::forward<T>(my_struct);                                \
        return std::invoke(f, ENUM_PARAMS(x, N));                                               \
    }

ENUM_FOR_EACH(APPLYER_DEF, 128)
#undef APPLYER_DEF
}  // namespace detail

// StructApply : 把结构体解包为变长参数调用可调用对象f
template <class T, class ApplyFunc>
auto struct_apply(T &&my_struct, ApplyFunc f) {
    return detail::StructApply_impl(std::forward<T>(my_struct), f, StructMemberCount<typename std::decay<T>::type>());
}

// StructTransformMeta : 把结构体各成员的类型作为变长参数调用元函数MetaFunc
template <class T, template <class...> class MetaFunc>
struct StructTransformMeta {
    struct FakeApplyer {
        template <class... Args>
        auto operator()(Args... args) -> MetaFunc<decltype(args)...>;
    };
    using type = decltype(struct_apply(std::declval<T>(), FakeApplyer()));
};
}  // namespace ME::meta

namespace reflect {

//--------------------------------------------------------
// Base class of all type descriptors
//--------------------------------------------------------

struct TypeDescriptor {
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
template <typename T>
TypeDescriptor *getPrimitiveDescriptor();

// A helper class to find TypeDescriptors in different ways:
struct DefaultResolver {
    template <typename T>
    static char func(decltype(&T::Reflection));
    template <typename T>
    static int func(...);
    template <typename T>
    struct IsReflected {
        enum { value = (sizeof(func<T>(nullptr)) == sizeof(char)) };
    };

    // This version is called if T has a static member named "Reflection":
    template <typename T, typename std::enable_if<IsReflected<T>::value, int>::type = 0>
    static TypeDescriptor *get() {
        return &T::Reflection;
    }

    // This version is called otherwise:
    template <typename T, typename std::enable_if<!IsReflected<T>::value, int>::type = 0>
    static TypeDescriptor *get() {
        return getPrimitiveDescriptor<T>();
    }
};

// This is the primary class template for finding all TypeDescriptors:
template <typename T>
struct TypeResolver {
    static TypeDescriptor *get() { return DefaultResolver::get<T>(); }
};

//--------------------------------------------------------
// Type descriptors for user-defined structs/classes
//--------------------------------------------------------

struct TypeDescriptor_Struct : TypeDescriptor {
    struct Member {
        const char *name;
        size_t offset;
        TypeDescriptor *type;
    };

    std::vector<Member> members;

    TypeDescriptor_Struct(void (*init)(TypeDescriptor_Struct *)) : TypeDescriptor{nullptr, 0} { init(this); }
    TypeDescriptor_Struct(const char *name, size_t size, const std::initializer_list<Member> &init) : TypeDescriptor{nullptr, 0}, members{init} {}
    virtual void dump(const void *obj, int indentLevel) const override {
        std::cout << name << " {" << std::endl;
        for (const Member &member : members) {
            std::cout << std::string(4 * (indentLevel + 1), ' ') << member.name << " = ";
            member.type->dump((char *)obj + member.offset, indentLevel + 1);
            std::cout << std::endl;
        }
        std::cout << std::string(4 * indentLevel, ' ') << "}";
    }
};

#define REFLECT_STRUCT()                              \
    friend struct reflect::DefaultResolver;           \
    static reflect::TypeDescriptor_Struct Reflection; \
    static void initReflection(reflect::TypeDescriptor_Struct *);

#define REFLECT_STRUCT_BEGIN(type)                                         \
    reflect::TypeDescriptor_Struct type::Reflection{type::initReflection}; \
    void type::initReflection(reflect::TypeDescriptor_Struct *typeDesc) {  \
        using T = type;                                                    \
        typeDesc->name = #type;                                            \
        typeDesc->size = sizeof(T);                                        \
        typeDesc->members = {

#define REFLECT_STRUCT_MEMBER(name) {#name, offsetof(T, name), reflect::TypeResolver<decltype(T::name)>::get()},

#define REFLECT_STRUCT_END() \
    }                        \
    ;                        \
    }

//--------------------------------------------------------
// Type descriptors for std::vector
//--------------------------------------------------------

struct TypeDescriptor_StdVector : TypeDescriptor {
    TypeDescriptor *itemType;
    size_t (*getSize)(const void *);
    const void *(*getItem)(const void *, size_t);

    template <typename ItemType>
    TypeDescriptor_StdVector(ItemType *) : TypeDescriptor{"std::vector<>", sizeof(std::vector<ItemType>)}, itemType{TypeResolver<ItemType>::get()} {
        getSize = [](const void *vecPtr) -> size_t {
            const auto &vec = *(const std::vector<ItemType> *)vecPtr;
            return vec.size();
        };
        getItem = [](const void *vecPtr, size_t index) -> const void * {
            const auto &vec = *(const std::vector<ItemType> *)vecPtr;
            return &vec[index];
        };
    }
    virtual std::string getFullName() const override { return std::string("std::vector<") + itemType->getFullName() + ">"; }
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
template <typename T>
class TypeResolver<std::vector<T>> {
public:
    static TypeDescriptor *get() {
        static TypeDescriptor_StdVector typeDesc{(T *)nullptr};
        return &typeDesc;
    }
};

}  // namespace reflect

#pragma endregion ME::meta

#endif