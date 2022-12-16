// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_CODEREFLECTION_H_
#define _METADOT_CODEREFLECTION_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum {
        METADOT_CREFLECT_TYPES_STRUCT = 1,
        METADOT_CREFLECT_TYPES_STRING = 2,
        METADOT_CREFLECT_TYPES_INTEGER = 3,
        METADOT_CREFLECT_TYPES_FLOAT = 4,
        METADOT_CREFLECT_TYPES_DOUBLE = 5,
        METADOT_CREFLECT_TYPES_POINTER = 6
    } METADOT_CREFLECT_Types;

    struct _METADOT_CREFLECT_FieldInfo
    {
        const char *field_type;
        const char *field_name;
        size_t size;
        size_t offset;
        int is_signed;
        int array_size;
        METADOT_CREFLECT_Types data_type;
    };

    typedef struct _METADOT_CREFLECT_FieldInfo METADOT_CREFLECT_FieldInfo;

    struct _METADOT_CREFLECT_TypeInfo
    {
        const char *name;
        size_t fields_count;
        size_t size;
        size_t packed_size;
        METADOT_CREFLECT_FieldInfo *fields;
    };

    typedef struct _METADOT_CREFLECT_TypeInfo METADOT_CREFLECT_TypeInfo;

#define METADOT_CREFLECT_EXPAND_(X) X
#define METADOT_CREFLECT_EXPAND_VA_(...) __VA_ARGS__
#define METADOT_CREFLECT_FOREACH_1_(FNC, USER_DATA, ARG) FNC(ARG, USER_DATA)
#define METADOT_CREFLECT_FOREACH_2_(FNC, USER_DATA, ARG, ...)                                      \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_1_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_3_(FNC, USER_DATA, ARG, ...)                                      \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_2_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_4_(FNC, USER_DATA, ARG, ...)                                      \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_3_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_5_(FNC, USER_DATA, ARG, ...)                                      \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_4_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_6_(FNC, USER_DATA, ARG, ...)                                      \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_5_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_7_(FNC, USER_DATA, ARG, ...)                                      \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_6_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_8_(FNC, USER_DATA, ARG, ...)                                      \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_7_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_9_(FNC, USER_DATA, ARG, ...)                                      \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_8_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_10_(FNC, USER_DATA, ARG, ...)                                     \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_9_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_11_(FNC, USER_DATA, ARG, ...)                                     \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_10_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_12_(FNC, USER_DATA, ARG, ...)                                     \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_11_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_13_(FNC, USER_DATA, ARG, ...)                                     \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_12_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_14_(FNC, USER_DATA, ARG, ...)                                     \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_13_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_15_(FNC, USER_DATA, ARG, ...)                                     \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_14_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_16_(FNC, USER_DATA, ARG, ...)                                     \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_15_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_17_(FNC, USER_DATA, ARG, ...)                                     \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_16_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_18_(FNC, USER_DATA, ARG, ...)                                     \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_17_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_19_(FNC, USER_DATA, ARG, ...)                                     \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_18_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_20_(FNC, USER_DATA, ARG, ...)                                     \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_19_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_21_(FNC, USER_DATA, ARG, ...)                                     \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_20_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_22_(FNC, USER_DATA, ARG, ...)                                     \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_21_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_23_(FNC, USER_DATA, ARG, ...)                                     \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_22_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_24_(FNC, USER_DATA, ARG, ...)                                     \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_23_(FNC, USER_DATA, __VA_ARGS__))
#define METADOT_CREFLECT_FOREACH_25_(FNC, USER_DATA, ARG, ...)                                     \
    FNC(ARG, USER_DATA)                                                                            \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_FOREACH_24_(FNC, USER_DATA, __VA_ARGS__))

#define METADOT_CREFLECT_OVERRIDE_4(_1, _2, _3, _4, FNC, ...) FNC
#define METADOT_CREFLECT_OVERRIDE_4_PLACEHOLDER 1, 2, 3, 4
#define METADOT_CREFLECT_OVERRIDE_5(_1, _2, _3, _4, _5, FNC, ...) FNC
#define METADOT_CREFLECT_OVERRIDE_5_PLACEHOLDER METADOT_CREFLECT_OVERRIDE_4_PLACEHOLDER, 5
#define METADOT_CREFLECT_OVERRIDE_14(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,  \
                                     FNC, ...)                                                     \
    FNC
#define METADOT_CREFLECT_OVERRIDE_14_PLACEHOLDER                                                   \
    METADOT_CREFLECT_OVERRIDE_5_PLACEHOLDER, 6, 7, 8, 9, 10, 11, 12, 13, 14
#define METADOT_CREFLECT_OVERRIDE_20(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,  \
                                     _15, _16, _17, _18, _19, _20, FNC, ...)                       \
    FNC
#define METADOT_CREFLECT_OVERRIDE_20_PLACEHOLDER                                                   \
    METADOT_CREFLECT_OVERRIDE_14_PLACEHOLDER, 15, 16, 17, 18, 19, 20
#define METADOT_CREFLECT_OVERRIDE_25(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,  \
                                     _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, FNC,   \
                                     ...)                                                          \
    FNC
#define METADOT_CREFLECT_OVERRIDE_25_PLACEHOLDER                                                   \
    METADOT_CREFLECT_OVERRIDE_20_PLACEHOLDER, 21, 22, 23, 24, 25

#define METADOT_CREFLECT_FOREACH(FNC, USER_DATA, ...)                                              \
    METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_OVERRIDE_25(                                         \
            __VA_ARGS__, METADOT_CREFLECT_FOREACH_25_, METADOT_CREFLECT_FOREACH_24_,               \
            METADOT_CREFLECT_FOREACH_23_, METADOT_CREFLECT_FOREACH_22_,                            \
            METADOT_CREFLECT_FOREACH_21_, METADOT_CREFLECT_FOREACH_20_,                            \
            METADOT_CREFLECT_FOREACH_19_, METADOT_CREFLECT_FOREACH_18_,                            \
            METADOT_CREFLECT_FOREACH_17_, METADOT_CREFLECT_FOREACH_16_,                            \
            METADOT_CREFLECT_FOREACH_15_, METADOT_CREFLECT_FOREACH_14_,                            \
            METADOT_CREFLECT_FOREACH_13_, METADOT_CREFLECT_FOREACH_12_,                            \
            METADOT_CREFLECT_FOREACH_11_, METADOT_CREFLECT_FOREACH_10_,                            \
            METADOT_CREFLECT_FOREACH_9_, METADOT_CREFLECT_FOREACH_8_, METADOT_CREFLECT_FOREACH_7_, \
            METADOT_CREFLECT_FOREACH_6_, METADOT_CREFLECT_FOREACH_5_, METADOT_CREFLECT_FOREACH_4_, \
            METADOT_CREFLECT_FOREACH_3_, METADOT_CREFLECT_FOREACH_2_,                              \
            METADOT_CREFLECT_FOREACH_1_)(FNC, USER_DATA, __VA_ARGS__))

#define METADOT_CREFLECT_DECLARE_SIMPLE_FIELD_(IGNORE, TYPE, FIELD_NAME) TYPE FIELD_NAME;
#define METADOT_CREFLECT_DECLARE_ARRAY_FIELD_(IGNORE, TYPE, FIELD_NAME, ARRAY_SIZE)                \
    TYPE FIELD_NAME[ARRAY_SIZE];

#define METADOT_CREFLECT_DECLARE_FIELD_(...)                                                       \
    METADOT_CREFLECT_EXPAND_(                                                                      \
            METADOT_CREFLECT_OVERRIDE_4(__VA_ARGS__, METADOT_CREFLECT_DECLARE_ARRAY_FIELD_,        \
                                        METADOT_CREFLECT_DECLARE_SIMPLE_FIELD_,                    \
                                        METADOT_CREFLECT_OVERRIDE_4_PLACEHOLDER)(__VA_ARGS__))

#define METADOT_CREFLECT_DECLARE_FIELD(X, USER_DATA) METADOT_CREFLECT_DECLARE_FIELD_ X

#define METADOT_CREFLECT_SIZEOF_(IGNORE, C_TYPE, ...) +sizeof(C_TYPE)
#define METADOT_CREFLECT_SIZEOF(X, USER_DATA) METADOT_CREFLECT_SIZEOF_ X

#define METADOT_CREFLECT_SUM(...) +1

#define METADOT_CREFLECT_IS_TYPE_SIGNED_(C_TYPE) (C_TYPE) - 1 < (C_TYPE) 1
#define METADOT_CREFLECT_IS_SIGNED_STRUCT(C_TYPE) 0
#define METADOT_CREFLECT_IS_SIGNED_STRING(C_TYPE) METADOT_CREFLECT_IS_TYPE_SIGNED_(C_TYPE)
#define METADOT_CREFLECT_IS_SIGNED_INTEGER(C_TYPE) METADOT_CREFLECT_IS_TYPE_SIGNED_(C_TYPE)
#define METADOT_CREFLECT_IS_SIGNED_FLOAT(C_TYPE) METADOT_CREFLECT_IS_TYPE_SIGNED_(C_TYPE)
#define METADOT_CREFLECT_IS_SIGNED_DOUBLE(C_TYPE) METADOT_CREFLECT_IS_TYPE_SIGNED_(C_TYPE)
#define METADOT_CREFLECT_IS_SIGNED_POINTER(C_TYPE) 0

#define METADOT_CREFLECT_IS_SIGNED_(DATA_TYPE, CTYPE) METADOT_CREFLECT_IS_SIGNED_##DATA_TYPE(CTYPE)

#define METADOT_CREFLECT_ARRAY_FIELD_INFO_(TYPE_NAME, DATA_TYPE, C_TYPE, FIELD_NAME, ARRAY_SIZE)   \
#C_TYPE, #FIELD_NAME, sizeof(C_TYPE) * ARRAY_SIZE, offsetof(TYPE_NAME, FIELD_NAME),            \
            METADOT_CREFLECT_IS_SIGNED_(DATA_TYPE, C_TYPE), ARRAY_SIZE,                            \
            METADOT_CREFLECT_TYPES_##DATA_TYPE

#define METADOT_CREFLECT_SIMPLE_FIELD_INFO_(TYPE_NAME, DATA_TYPE, C_TYPE, FIELD_NAME)              \
#C_TYPE, #FIELD_NAME, sizeof(C_TYPE), offsetof(TYPE_NAME, FIELD_NAME),                         \
            METADOT_CREFLECT_IS_SIGNED_(DATA_TYPE, C_TYPE), -1, METADOT_CREFLECT_TYPES_##DATA_TYPE

#define METADOT_CREFLECT_FIELD_INFO_(...)                                                          \
    {METADOT_CREFLECT_EXPAND_(METADOT_CREFLECT_OVERRIDE_5(                                         \
            __VA_ARGS__, METADOT_CREFLECT_ARRAY_FIELD_INFO_, METADOT_CREFLECT_SIMPLE_FIELD_INFO_,  \
            METADOT_CREFLECT_OVERRIDE_5_PLACEHOLDER)(__VA_ARGS__))},

#define METADOT_CREFLECT_FIELD_INFO(X, USER_DATA)                                                  \
    METADOT_CREFLECT_FIELD_INFO_(USER_DATA, METADOT_CREFLECT_EXPAND_VA_ X)

#ifdef METADOT_CREFLECT_IMPL

#define METADOT_CREFLECT_DEFINE_GET_METHOD(TYPE_NAME, ...)                                         \
    METADOT_CREFLECT_TypeInfo *metadot_creflect_get_##TYPE_NAME##_type_info(void) {                      \
        static METADOT_CREFLECT_FieldInfo fields_info[METADOT_CREFLECT_FOREACH(                    \
                METADOT_CREFLECT_SUM, 0, __VA_ARGS__)] = {                                         \
                METADOT_CREFLECT_FOREACH(METADOT_CREFLECT_FIELD_INFO, TYPE_NAME, __VA_ARGS__)};    \
        static METADOT_CREFLECT_TypeInfo type_info = {                                             \
                #TYPE_NAME, METADOT_CREFLECT_FOREACH(METADOT_CREFLECT_SUM, 0, __VA_ARGS__),        \
                sizeof(TYPE_NAME),                                                                 \
                METADOT_CREFLECT_FOREACH(METADOT_CREFLECT_SIZEOF, 0, __VA_ARGS__), fields_info};   \
        return &type_info;                                                                         \
    }

#else

#define METADOT_CREFLECT_DEFINE_GET_METHOD(TYPE_NAME, ...)

#endif

#define METADOT_CREFLECT_DEFINE_STRUCT(TYPE_NAME, ...)                                             \
    typedef struct                                                                                 \
    {                                                                                              \
        METADOT_CREFLECT_FOREACH(METADOT_CREFLECT_DECLARE_FIELD, 0, __VA_ARGS__)                   \
    } TYPE_NAME;                                                                                   \
    METADOT_CREFLECT_TypeInfo *metadot_creflect_get_##TYPE_NAME##_type_info(void);                       \
    METADOT_CREFLECT_DEFINE_GET_METHOD(TYPE_NAME, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
