// Copyright(c) 2022, KaoruXun All rights reserved.

#include "engine_meta.h"
#include "core/core.h"
#include "libs/external/stb_ds.h"

#include <stdio.h>

meta_registry_t meta_registry_new() {
    meta_registry_t meta = {0};
    return meta;
}

void meta_registry_free(meta_registry_t *meta) {
    // Handle in a little bit
}

U64 _meta_registry_register_class_impl(meta_registry_t *meta, const char *name,
                                       const meta_class_decl_t *decl) {
    meta_class_t cls = {0};

    // Make value for pair
    U32 ct = decl->size / sizeof(meta_property_t);
    cls.name = name;
    cls.property_count = ct;
    cls.properties = malloc(decl->size);
    memcpy(cls.properties, decl->properties, decl->size);

    // Make key for pair
    // Hash our name into a 64-bit identifier
    U64 id = (U64) stbds_hash_string(name, 0);

    // Insert key/value pair into our hash table
    hmput(meta->classes, id, cls);
    return id;
}

meta_property_t _meta_property_impl(const char *name, size_t offset,
                                    meta_property_type_info_t type) {
    meta_property_t mp = {0};
    mp.name = name;
    mp.offset = offset;
    mp.type = type;
    return mp;
}

meta_property_type_info_t _meta_property_type_info_decl_impl(const char *name, U32 id) {
    meta_property_type_info_t info = {0};
    info.name = name;
    info.id = id;
    return info;
}

meta_class_t *_meta_class_getp_impl(meta_registry_t *meta, const char *name) {
    U64 id = (U64) stbds_hash_string(name, 0);
    return (&hmgetp(meta->classes, id)->value);
}

typedef struct other_thing_t
{
    I32 s_val;
} other_thing_t;

#define META_PROPERTY_TYPE_OBJ (META_PROPERTY_TYPE_COUNT + 1)
#define META_PROPERTY_TYPE_INFO_OBJ                                                                \
    meta_property_type_info_decl(other_thing_t, META_PROPERTY_TYPE_OBJ)

typedef struct thing_t
{
    U32 u_val;
    F32 f_val;
    const char *name;
    other_thing_t o_val;
} thing_t;

void print_obj(meta_registry_t *meta, void *obj, meta_class_t *cls) {
    for (U32 i = 0; i < cls->property_count; ++i) {
        meta_property_t *prop = &cls->properties[i];

        switch (prop->type.id) {
            case META_PROPERTY_TYPE_U32: {
                U32 *v = meta_registry_getvp(obj, U32, prop);
                printf("%s (%s): %zu\n", prop->name, prop->type.name, *v);
            } break;

            case META_PROPERTY_TYPE_S32: {
                I32 *v = meta_registry_getvp(obj, I32, prop);
                printf("%s (%s): %d\n", prop->name, prop->type.name, *v);
            } break;

            case META_PROPERTY_TYPE_F32: {
                F32 *v = meta_registry_getvp(obj, F32, prop);
                printf("%s (%s): %.2f\n", prop->name, prop->type.name, *v);
            } break;

            case META_PROPERTY_TYPE_STR: {
                const char *v = meta_registry_getv(obj, const char *, prop);
                printf("%s (%s): %s\n", prop->name, prop->type.name, v);
            } break;

            case META_PROPERTY_TYPE_OBJ: {
                printf("%s (%s):\n", prop->name, prop->type.name);
                const other_thing_t *v = meta_registry_getvp(obj, other_thing_t, prop);
                const meta_class_t *clz = meta_registry_class_get(meta, other_thing_t);
                print_obj(meta, v, clz);
            } break;
        }
    }
}

I32 test() {
    meta_registry_t meta = meta_registry_new();

    thing_t thing = {.u_val = 42069,
                     .f_val = 3.145f,
                     .name = "THING",
                     .o_val = (other_thing_t){.s_val = -380}};

    // thing_t registry
    meta_registry_register_class(
            &meta, thing_t,
            (&(meta_class_decl_t){
                    .properties =
                            (meta_property_t[]){
                                    meta_property(thing_t, u_val, META_PROPERTY_TYPE_INFO_U32),
                                    meta_property(thing_t, f_val, META_PROPERTY_TYPE_INFO_F32),
                                    meta_property(thing_t, name, META_PROPERTY_TYPE_INFO_STR),
                                    meta_property(thing_t, o_val, META_PROPERTY_TYPE_INFO_OBJ)},
                    .size = 4 * sizeof(meta_property_t)}));

    // other_thing_t registry
    meta_registry_register_class(
            &meta, other_thing_t,
            (&(meta_class_decl_t){.properties = (meta_property_t[]){meta_property(
                                          other_thing_t, s_val, META_PROPERTY_TYPE_INFO_S32)},
                                  .size = 1 * sizeof(meta_property_t)}));

    meta_class_t *cls = meta_registry_class_get(&meta, thing_t);
    print_obj(&meta, &thing, cls);

    return 0;
}

#pragma region Reflection

/************************************************************************/
/* const string                                                         */
/************************************************************************/
C_API const_string_t const_string_new(const char *str) {
    const_string_t cs;
    cs.cstr = strdup(str);
    return cs;
}

C_API void const_string_clear(const_string_t cs) {
    if (cs.cstr) { free(cs.cstr); }
}

/************************************************************************/
/* free buffer                                                          */
/************************************************************************/
#define BLOCK_SIZE 1024
#define CEIL(n) (((n) + BLOCK_SIZE - 1) / BLOCK_SIZE * BLOCK_SIZE)

C_API void free_buffer_init(free_buffer_t *buf) { memset(buf, 0, sizeof(free_buffer_t)); }

C_API void free_buffer_append(free_buffer_t *buf, void *data, size_t len) {
    size_t need_len = buf->length + len + 1;
    if (need_len > buf->size) {
        size_t newlen = buf->length + len;
        size_t newsize = CEIL(need_len);

        char *newptr = (char *) malloc(newsize);
        memcpy(newptr, buf->ptr, buf->length);
        memcpy(newptr + buf->length, data, len);

        free(buf->ptr);
        buf->ptr = newptr;
        buf->length = newlen;
        buf->size = newsize;
    } else {
        memcpy(buf->ptr + buf->length, data, len);
        buf->length += len;
    }

    buf->ptr[buf->length] = 0;

    buf->count++;
}

C_API void free_buffer_append_empty(free_buffer_t *buf, size_t len) {
    void *data = malloc(len);
    memset(data, 0, len);
    free_buffer_append(buf, data, len);
    free(data);
}

C_API void free_buffer_cleanup(free_buffer_t *buf) {
    if (buf->ptr) { free(buf->ptr); }
    buf->ptr = NULL;
    buf->length = 0;
    buf->size = 0;
    buf->count = 0;
}

/************************************************************************/
/* metainfo                                                             */
/************************************************************************/
C_API metainfo_t *metainfo_create(int obj_size) {
    metainfo_t *mi = (metainfo_t *) malloc(sizeof(metainfo_t));
    memset(mi, 0, sizeof(metainfo_t));
    mi->obj_size = obj_size;
    return mi;
}

C_API void metainfo_destroy(metainfo_t *mi) {
    LIST_CLEAR(mi->fs);
    free(mi);
}

C_API void metainfo_add_member(metainfo_t *mi, int pos, int type, const char *name, int islist) {
    field_t f;
    f.pos = pos;
    f.type = type;
    f.islist = islist;
    f.name = name;
    f.metainfo = NULL;
    LIST_ADD_OBJ(mi->fs, f);
}

C_API metainfo_t *metainfo_add_child(metainfo_t *mi, int pos, int obj_size, const char *name,
                                     int islist) {
    metainfo_t *child = metainfo_create(obj_size);
    field_t f;
    f.pos = pos;
    f.type = FIELD_TYPE_STRUCT;
    f.islist = islist;
    f.name = name;
    f.metainfo = child;
    LIST_ADD_OBJ(mi->fs, f);
    return child;
}

/************************************************************************/
/* C Object                                                             */
/************************************************************************/
C_API void *cobject_new(metainfo_t *mi) {
    void *ptr = malloc(mi->obj_size);
    memset(ptr, 0, mi->obj_size);
    return ptr;
}

C_API void cobject_init(void *obj, metainfo_t *mi) { memset(obj, 0, mi->obj_size); }

C_API void cobject_clear(void *obj, metainfo_t *mi) {
    int i;

    for (i = 0; i < mi->fs.size; i++) {
        field_t *f = &mi->fs.array[i];
        void *ptr = ((char *) obj + f->pos);

        if (f->islist) {
            free_buffer_t *fb = (free_buffer_t *) ptr;
            int k;

            if (f->type == FIELD_TYPE_CSTR) {
                for (k = 0; k < fb->count; k++) {
                    CSTR *p = (CSTR *) (fb->ptr + k * sizeof(CSTR));
                    CS_CLEAR(*p);
                }
            } else if (f->type == FIELD_TYPE_STRUCT) {
                for (k = 0; k < fb->count; k++) {
                    metainfo_t *mi = (metainfo_t *) f->metainfo;
                    void *obj = fb->ptr + k * mi->obj_size;
                    cobject_clear(obj, mi);
                }
            }

            LIST_CLEAR(*fb);
        } else {
            if (f->type == FIELD_TYPE_INT) {
                int *p = (int *) ptr;
                *p = 0;
            } else if (f->type == FIELD_TYPE_BOOL) {
                R_bool *p = (R_bool *) ptr;
                *p = 0;
            } else if (f->type == FIELD_TYPE_CSTR) {
                CSTR *p = (CSTR *) ptr;
                CS_CLEAR(*p);
            } else if (f->type == FIELD_TYPE_STRUCT) {
                metainfo_t *mi = (metainfo_t *) f->metainfo;
                void *obj = ptr;
                cobject_clear(obj, mi);
            }
        }
    }
}

C_API void cobject_delete(void *obj, metainfo_t *mi) {
    cobject_clear(obj, mi);
    free(obj);
}

static cJSON *_to_jsonobject(void *obj, metainfo_t *mi) {
    cJSON *root = cJSON_CreateObject();
    int i;

    for (i = 0; i < mi->fs.size; i++) {
        field_t *f = &mi->fs.array[i];
        void *ptr = ((char *) obj + f->pos);

        if (f->islist) {
            free_buffer_t *fb = (free_buffer_t *) ptr;
            cJSON *array = NULL;

            if (f->type == FIELD_TYPE_INT) {
                array = cJSON_CreateIntArray((int *) fb->ptr, fb->count);
            } else if (f->type == FIELD_TYPE_CSTR) {
                array = cJSON_CreateStringArray((const char **) fb->ptr, fb->count);
            } else if (f->type == FIELD_TYPE_STRUCT) {
                int k;
                array = cJSON_CreateArray();
                for (k = 0; k < fb->count; k++) {
                    metainfo_t *mi = (metainfo_t *) f->metainfo;
                    void *obj = fb->ptr + k * mi->obj_size;
                    cJSON *item = _to_jsonobject(obj, mi);
                    cJSON_AddItemToArray(array, item);
                }
            }

            if (array) { cJSON_AddItemToObject(root, f->name, array); }
        } else {
            if (f->type == FIELD_TYPE_INT) {
                int *p = (int *) ptr;
                cJSON_AddNumberToObject(root, f->name, *p);
            } else if (f->type == FIELD_TYPE_BOOL) {
                R_bool *p = (R_bool *) ptr;
                if (*p) {
                    cJSON_AddTrueToObject(root, f->name);
                } else {
                    cJSON_AddFalseToObject(root, f->name);
                }
            } else if (f->type == FIELD_TYPE_CSTR) {
                CSTR *p = (CSTR *) ptr;
                if (p->cstr) {
                    cJSON_AddStringToObject(root, f->name, p->cstr);
                } else {
                    cJSON_AddStringToObject(root, f->name, "");
                }
            } else if (f->type == FIELD_TYPE_STRUCT) {
                cJSON *child = _to_jsonobject(ptr, f->metainfo);
                cJSON_AddItemToObject(root, f->name, child);
            }
        }
    }

    return root;
}

C_API CSTR cobject_to_json(void *obj, metainfo_t *mi) {
    const_string_t cs = {NULL};
    cJSON *json;

    if (!obj || !mi) { return cs; }

    json = _to_jsonobject(obj, mi);

    cs.cstr = cJSON_PrintUnformatted(json);

    cJSON_Delete(json);

    return cs;
}

static void _from_jsonobject(void *obj, metainfo_t *mi, cJSON *json) {
    int i;
    for (i = 0; i < mi->fs.size; i++) {
        field_t *f = &mi->fs.array[i];
        void *ptr = ((char *) obj + f->pos);

        cJSON *item = cJSON_GetObjectItem(json, f->name);
        if (!item) { continue; }

        if (f->islist) {
            free_buffer_t *fb = (free_buffer_t *) ptr;

            if (item->type == cJSON_Array) {
                cJSON *array = item;
                int size = cJSON_GetArraySize(array);
                int k;
                for (k = 0; k < size; k++) {
                    cJSON *item = cJSON_GetArrayItem(array, k);
                    if (f->type == FIELD_TYPE_INT) {
                        if (item->type == cJSON_Number) { LIST_ADD_INT(*fb, item->valueint); }
                    } else if (f->type == FIELD_TYPE_CSTR) {
                        if (item->type == cJSON_String) { LIST_ADD_STR(*fb, item->valuestring); }
                    } else if (f->type == FIELD_TYPE_STRUCT) {
                        if (item->type == cJSON_Object) {
                            metainfo_t *mi = (metainfo_t *) f->metainfo;
                            void *obj = cobject_new(mi);
                            _from_jsonobject(obj, mi, item);
                            free_buffer_append(fb, obj, mi->obj_size);
                            free(obj);
                        }
                    }
                }
            }
        } else {
            if (f->type == FIELD_TYPE_INT) {
                int *p = (int *) ptr;
                if (item->type == cJSON_Number) { *p = item->valueint; }
            } else if (f->type == FIELD_TYPE_BOOL) {
                R_bool *p = (R_bool *) ptr;
                if (item->type == cJSON_True) {
                    *p = 1;
                } else if (item->type == cJSON_False) {
                    *p = 0;
                }
            } else if (f->type == FIELD_TYPE_CSTR) {
                CSTR *p = (CSTR *) ptr;
                if (item->type == cJSON_String) { *p = CS(item->valuestring); }
            } else if (f->type == FIELD_TYPE_STRUCT) {
                if (item->type == cJSON_Object) {
                    metainfo_t *mi = (metainfo_t *) f->metainfo;
                    void *obj = ptr;
                    _from_jsonobject(obj, mi, item);
                }
            }
        }
    }
}

C_API void *cobject_from_json(metainfo_t *mi, const char *str) {
    void *obj = cobject_new(mi);
    cJSON *json;
    if (!mi || !str) { return NULL; }

    json = cJSON_Parse(str);
    if (json == NULL) { return NULL; }

    _from_jsonobject(obj, mi, json);

    cJSON_Delete(json);

    return obj;
}

#pragma endregion Reflection
