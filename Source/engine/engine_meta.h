// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_ENGINEMETA_H_
#define _METADOT_ENGINEMETA_H_

#include "core/core.h"
#include "libs/cJSON.h"

#include <stdlib.h>// malloc, realloc, free
#include <string.h>// strlen

#ifdef __cplusplus
#define C_API extern "C"
#else
#define C_API
#endif

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

#pragma region Reflection

/************************************************************************/
/* we use CSTR for string type                                          */
/************************************************************************/
typedef struct
{
    char *cstr;
} const_string_t;

C_API const_string_t const_string_new(const char *str);
C_API void const_string_clear(const_string_t cs);

#define CSTR const_string_t
#define CS(str) const_string_new(str)
#define CS_CLEAR(cs) const_string_clear(cs), (cs).cstr = NULL

/************************************************************************/
/* free buffer which is easy to use                                     */
/************************************************************************/
typedef struct
{
    char *ptr;
    size_t length;
    size_t size;
    int count;
} free_buffer_t;

C_API void free_buffer_init(free_buffer_t *buf);
C_API void free_buffer_append(free_buffer_t *buf, void *data, size_t len);
C_API void free_buffer_append_empty(free_buffer_t *buf, size_t len);
C_API void free_buffer_cleanup(free_buffer_t *buf);

/************************************************************************/
/* self-define List type with generic type supported                    */
/************************************************************************/
#define DECLARE_LIST(TYPE)                                                                         \
    struct                                                                                         \
    {                                                                                              \
        TYPE *array;                                                                               \
        size_t _buffer_length;                                                                     \
        size_t _buffer_size;                                                                       \
        int size;                                                                                  \
    }

#define LIST_INIT(list) free_buffer_init((free_buffer_t *) &list)
#define LIST_ADD(list, obj) free_buffer_append((free_buffer_t *) &list, &obj, sizeof(obj))
#define LIST_ADD_EMPTY(list, s) free_buffer_append_empty((free_buffer_t *) &list, sizeof(struct s))
#define LIST_CLEAR(list) free_buffer_cleanup((free_buffer_t *) &list)

#define LIST_ADD_INT(list, s)                                                                      \
    {                                                                                              \
        int i = s;                                                                                 \
        free_buffer_append((free_buffer_t *) &list, &i, sizeof(i));                                \
    }
#define LIST_ADD_BOOL(list, s)                                                                     \
    {                                                                                              \
        R_bool b = s;                                                                              \
        free_buffer_append((free_buffer_t *) &list, &b, sizeof(b));                                \
    }
#define LIST_ADD_STR(list, s)                                                                      \
    {                                                                                              \
        CSTR cstr = CS(s);                                                                         \
        free_buffer_append((free_buffer_t *) &list, &cstr, sizeof(cstr));                          \
    }
#define LIST_ADD_OBJ(list, s) LIST_ADD(list, s)

/************************************************************************/
/* metainfo                                                             */
/************************************************************************/
enum {
    /* Note that now supported types as below */
    FIELD_TYPE_INT = 0,
    FIELD_TYPE_BOOL,
    FIELD_TYPE_CSTR,
    FIELD_TYPE_STRUCT = 100,
};

typedef struct
{
    int pos;
    int type;
    int islist;
    const char *name;
    void *metainfo;
} field_t;

typedef struct
{
    int obj_size;
    DECLARE_LIST(field_t) fs;
} metainfo_t;

C_API metainfo_t *metainfo_create(int obj_size);
C_API void metainfo_destroy(metainfo_t *mi);
C_API void metainfo_add_member(metainfo_t *mi, int pos, int type, const char *name, int islist);
C_API metainfo_t *metainfo_add_child(metainfo_t *mi, int pos, int obj_size, const char *name,
                                     int islist);

#define METAINFO(s) _metainfo_##s
#define REGISTER_METAINFO(s) _register_metainfo_##s()
#define DEFINE_METAINFO(s)                                                                         \
    metainfo_t *METAINFO(s);                                                                       \
    REGISTER_METAINFO(s)

#define METAINFO_CREATE(s) METAINFO(s) = metainfo_create((int) sizeof(struct s))
#define METAINFO_DESTROY(s) metainfo_destroy(METAINFO(s))

#define METAINFO_ADD_MEMBER(s, t, n)                                                               \
    metainfo_add_member(METAINFO(s), offsetof(struct s, n), t, #n, 0)
#define METAINFO_ADD_MEMBER_LIST(s, t, n)                                                          \
    metainfo_add_member(METAINFO(s), offsetof(struct s, n), t, #n, 1)
#define METAINFO_ADD_CHILD(s, t, n)                                                                \
    metainfo_add_child(METAINFO(s), offsetof(struct s, n), (int) sizeof(struct t), #n, 0)
#define METAINFO_ADD_CHILD_LIST(s, t, n)                                                           \
    metainfo_add_child(METAINFO(s), offsetof(struct s, n), (int) sizeof(struct t), #n, 1)

#define METAINFO_CHILD_BEGIN(s, t, n)                                                              \
    {                                                                                              \
        metainfo_t *METAINFO(t) = METAINFO_ADD_CHILD(s, t, n);
#define METAINFO_CHILD_LIST_BEGIN(s, t, n)                                                         \
    {                                                                                              \
        metainfo_t *METAINFO(t) = METAINFO_ADD_CHILD_LIST(s, t, n);
#define METAINFO_CHILD_END() }

/************************************************************************/
/* C Object <===> Json String                                           */
/************************************************************************/
C_API void *cobject_new(metainfo_t *mi);
#define OBJECT_NEW(s) (struct s *) cobject_new(METAINFO(s))

C_API void cobject_clear(void *obj, metainfo_t *mi);
#define OBJECT_CLEAR(ptr, s) cobject_clear(ptr, METAINFO(s))

C_API void cobject_delete(void *obj, metainfo_t *mi);
#define OBJECT_DELETE(ptr, s) cobject_delete(ptr, METAINFO(s))

C_API CSTR cobject_to_json(void *obj, metainfo_t *mi);
#define OBJECT_TO_JSON(ptr, s) cobject_to_json(ptr, METAINFO(s))

C_API void *cobject_from_json(metainfo_t *mi, const char *str);
#define OBJECT_FROM_JSON(s, str) cobject_from_json(METAINFO(s), str)

#pragma endregion Reflection

#endif// META_H
