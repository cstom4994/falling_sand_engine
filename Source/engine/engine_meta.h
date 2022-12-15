// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_ENGINEMETA_H_
#define _METADOT_ENGINEMETA_H_

#include "core/core.h"
#include "libs/cJSON.h"

#include <stdlib.h>// malloc, realloc, free
#include <string.h>// strlen

#define meta_offset(T, E) ((size_t) (&(((T *) (0))->E)))
#define meta_to_str(T) ((const char *) #T)

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

typedef struct meta_property_type_info_t
{
    const char *name;// Display name
    U32 id;          // Matches up to property type, used for lookups and switch statements
} meta_property_type_info_t;

extern meta_property_type_info_t _meta_property_type_info_decl_impl(const char *name, U32 id);

#define meta_property_type_info_decl(T, PROP_TYPE)                                                 \
    _meta_property_type_info_decl_impl(meta_to_str(T), (PROP_TYPE))

#define META_PROPERTY_TYPE_INFO_U8 meta_property_type_info_decl(U8, META_PROPERTY_TYPE_U8)
#define META_PROPERTY_TYPE_INFO_S8 meta_property_type_info_decl(I8, META_PROPERTY_TYPE_S8)
#define META_PROPERTY_TYPE_INFO_U16 meta_property_type_info_decl(U16, META_PROPERTY_TYPE_U16)
#define META_PROPERTY_TYPE_INFO_S16 meta_property_type_info_decl(I16, META_PROPERTY_TYPE_S16)
#define META_PROPERTY_TYPE_INFO_U32 meta_property_type_info_decl(U32, META_PROPERTY_TYPE_U32)
#define META_PROPERTY_TYPE_INFO_S32 meta_property_type_info_decl(I32, META_PROPERTY_TYPE_S32)
#define META_PROPERTY_TYPE_INFO_U64 meta_property_type_info_decl(U64, META_PROPERTY_TYPE_U64)
#define META_PROPERTY_TYPE_INFO_S64 meta_property_type_info_decl(I64, META_PROPERTY_TYPE_S64)
#define META_PROPERTY_TYPE_INFO_F32 meta_property_type_info_decl(F32, META_PROPERTY_TYPE_F32)
#define META_PROPERTY_TYPE_INFO_F64 meta_property_type_info_decl(F64, META_PROPERTY_TYPE_F64)
#define META_PROPERTY_TYPE_INFO_SIZE_T                                                             \
    meta_property_type_info_decl(size_t, META_PROPERTY_TYPE_SIZE_T)
#define META_PROPERTY_TYPE_INFO_STR meta_property_type_info_decl(char *, META_PROPERTY_TYPE_STR)

typedef struct meta_property_t
{
    const char *name;              // Display name of field
    size_t offset;                 // Offset in bytes to struct
    meta_property_type_info_t type;// Type info
} meta_property_t;

extern meta_property_t _meta_property_impl(const char *name, size_t offset,
                                           meta_property_type_info_t type);

#define meta_property(CLS, FIELD, TYPE)                                                            \
    _meta_property_impl(meta_to_str(FIELD), meta_offset(CLS, FIELD), TYPE)

typedef struct meta_class_t
{
    const char *name;           // Display name
    U32 property_count;         // Count of all properties in list
    meta_property_t *properties;// List of all properties
} meta_class_t;

typedef struct meta_registry_t
{
    struct
    {
        U64 key;
        meta_class_t value;
    } *classes;
} meta_registry_t;

typedef struct meta_class_decl_t
{
    meta_property_t *properties;// Array of properties
    size_t size;                // Size of array in bytes
} meta_class_decl_t;

// Functions
extern meta_registry_t meta_registry_new();
extern void meta_registry_free(meta_registry_t *meta);
extern U64 _meta_registry_register_class_impl(meta_registry_t *meta, const char *name,
                                              const meta_class_decl_t *decl);
extern meta_class_t *_meta_class_getp_impl(meta_registry_t *meta, const char *name);

#define meta_registry_register_class(META, T, DECL)                                                \
    _meta_registry_register_class_impl((META), meta_to_str(T), (DECL))

#define meta_registry_class_get(META, T) _meta_class_getp_impl(META, meta_to_str(T))

#define meta_registry_getvp(OBJ, T, PROP) ((T *) ((U8 *) (OBJ) + (PROP)->offset))

#define meta_registry_getv(OBJ, T, PROP) (*((T *) ((U8 *) (OBJ) + (PROP)->offset)))

typedef struct engine_meta
{

} engine_meta;

typedef struct c_meta
{
    cJSON *cjson;
} c_meta;

#endif// META_H
