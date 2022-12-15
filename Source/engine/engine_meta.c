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
