// Copyright(c) 2023, KaoruXun All rights reserved.

#include "reflection.hpp"

#include <chrono>
#include <functional>
#include <istream>
#include <map>
#include <string>

#include "engine/core/core.hpp"
#include "engine/core/dbgtools.h"
#include "engine/core/memory.h"
#include "engine/utils/utility.hpp"

// #define STB_DS_IMPLEMENTATION
#include "libs/external/stb_ds.h"

meta_registry_t meta_registry_new() {
    meta_registry_t meta = {0};
    return meta;
}

void meta_registry_free(meta_registry_t *meta) {
    // Handle in a little bit
}

u64 _meta_registry_register_class_impl(meta_registry_t *meta, const char *name, const meta_class_decl_t *decl) {
    meta_class_t cls = {0};

    // Make value for pair
    u32 ct = decl->size / sizeof(meta_property_t);
    cls.name = name;
    cls.property_count = ct;
    cls.properties = (meta_property_t *)ME_MALLOC(decl->size);
    memcpy(cls.properties, decl->properties, decl->size);

    // Make key for pair
    // Hash our name into a 64-bit identifier
    u64 id = (u64)stbds_hash_string((char *)name, 0);

    // Insert key/value pair into our hash table
    hmput(meta->classes, id, cls);
    return id;
}

meta_property_t _meta_property_impl(const char *name, size_t offset, meta_property_type_info_t type) {
    meta_property_t mp = {0};
    mp.name = name;
    mp.offset = offset;
    mp.type = type;
    return mp;
}

meta_property_type_info_t _meta_property_type_info_decl_impl(const char *name, u32 id) {
    meta_property_type_info_t info = {0};
    info.name = name;
    info.id = id;
    return info;
}

meta_class_t *_meta_class_getp_impl(meta_registry_t *meta, const char *name) {
    u64 id = (u64)stbds_hash_string((char *)name, 0);
    return (&hmgetp(meta->classes, id)->value);
}

typedef struct other_thing_t {
    i32 s_val;
} other_thing_t;

#define META_PROPERTY_TYPE_OBJ (META_PROPERTY_TYPE_COUNT + 1)
#define META_PROPERTY_TYPE_INFO_OBJ meta_property_type_info_decl(other_thing_t, META_PROPERTY_TYPE_OBJ)

typedef struct thing_t {
    u32 u_val;
    f32 f_val;
    const char *name;
    other_thing_t o_val;
} thing_t;

void print_obj(meta_registry_t *meta, void *obj, meta_class_t *cls) {
    for (u32 i = 0; i < cls->property_count; ++i) {
        meta_property_t *prop = &cls->properties[i];

        switch (prop->type.id) {
            case META_PROPERTY_TYPE_U32: {
                u32 *v = meta_registry_getvp(obj, u32, prop);
                printf("%s (%s): %zu\n", prop->name, prop->type.name, *v);
            } break;

            case META_PROPERTY_TYPE_S32: {
                i32 *v = meta_registry_getvp(obj, i32, prop);
                printf("%s (%s): %d\n", prop->name, prop->type.name, *v);
            } break;

            case META_PROPERTY_TYPE_F32: {
                f32 *v = meta_registry_getvp(obj, f32, prop);
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
                print_obj(meta, (void *)v, (meta_class_t *)clz);
            } break;
        }
    }
}

#if 1

namespace UMeta {
struct Range {
    float minV;
    float maxV;
};
}  // namespace UMeta

template <>
struct ME::meta::static_refl::TypeInfo<UMeta::Range> : TypeInfoBase<UMeta::Range> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("minV"), &Type::minV},
            Field{TSTR("maxV"), &Type::maxV},
    };
};

struct Point {
    [[Attr::Range(1.f, 2.f)]] float x;
    float y;
};

template <>
struct ME::meta::static_refl::TypeInfo<Point> : TypeInfoBase<Point> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("x"), &Type::x, AttrList{Attr{TSTR("UMeta::Range"), std::tuple{1.f, 2.f}}}},
            Field{TSTR("y"), &Type::y},
    };
};

#endif

i32 test() {
    meta_registry_t meta = meta_registry_new();

    thing_t thing = {.u_val = 42069, .f_val = 3.145f, .name = "THING", .o_val = other_thing_t{.s_val = -380}};

    // thing_t registry
    meta_property_t mp1[] = {meta_property(thing_t, u_val, META_PROPERTY_TYPE_INFO_U32), meta_property(thing_t, f_val, META_PROPERTY_TYPE_INFO_F32),
                             meta_property(thing_t, name, META_PROPERTY_TYPE_INFO_STR), meta_property(thing_t, o_val, META_PROPERTY_TYPE_INFO_OBJ)};
    meta_class_decl_t cd1 = {.properties = mp1, .size = 4 * sizeof(meta_property_t)};
    meta_registry_register_class(&meta, thing_t, &cd1);

    // other_thing_t registry
    meta_property_t mp2[] = {meta_property(other_thing_t, s_val, META_PROPERTY_TYPE_INFO_S32)};
    meta_class_decl_t cd2 = {.properties = mp2, .size = 1 * sizeof(meta_property_t)};
    meta_registry_register_class(&meta, other_thing_t, &cd2);

    meta_class_t *cls = meta_registry_class_get(&meta, thing_t);
    print_obj(&meta, &thing, cls);

    return 0;
}

namespace reflect {

//--------------------------------------------------------
// A type descriptor for int
//--------------------------------------------------------

struct TypeDescriptor_Int : TypeDescriptor {
    TypeDescriptor_Int() : TypeDescriptor{"int", sizeof(int)} {}
    virtual void dump(const void *obj, int /* unused */) const override { std::cout << "int{" << *(const int *)obj << "}"; }
};

template <>
TypeDescriptor *getPrimitiveDescriptor<int>() {
    static TypeDescriptor_Int typeDesc;
    return &typeDesc;
}

//--------------------------------------------------------
// A type descriptor for std::string
//--------------------------------------------------------

struct TypeDescriptor_StdString : TypeDescriptor {
    TypeDescriptor_StdString() : TypeDescriptor{"std::string", sizeof(std::string)} {}
    virtual void dump(const void *obj, int /* unused */) const override { std::cout << "std::string{\"" << *(const std::string *)obj << "\"}"; }
};

template <>
TypeDescriptor *getPrimitiveDescriptor<std::string>() {
    static TypeDescriptor_StdString typeDesc;
    return &typeDesc;
}

}  // namespace reflect

// Here is acknowledgement
// https://stackoverflow.com/questions/41453/how-can-i-add-reflection-to-a-c-application/

struct Node {
    std::string key;
    int value;
    std::vector<Node> children;

    REFLECT_STRUCT()  // Enable reflection for this type
};

// Define Node's type descriptor
REFLECT_STRUCT_BEGIN(Node)
REFLECT_STRUCT_MEMBER(key)
REFLECT_STRUCT_MEMBER(value)
REFLECT_STRUCT_MEMBER(children)
REFLECT_STRUCT_END()

void TestRefleaction() {

    // Create an object of type Node
    Node node = {"apple", 3, {{"banana", 7, {}}, {"cherry", 11, {}}}};

    // Find Node's type descriptor
    reflect::TypeDescriptor *typeDesc = reflect::TypeResolver<Node>::get();

    // Dump a description of the Node object to the console
    typeDesc->dump(&node);
}

namespace IamAfuckingNamespace {
int func1(float f, char c) {
    std::cout << "func1(" << f << ", " << c << ")" << std::endl;
    return 42;
}

void func2(void) { std::cout << "func2()" << std::endl; }

void func_log_info(std::string info) { METADOT_INFO(info.c_str()); }
}  // namespace IamAfuckingNamespace

#define RETURNTYPE(fc) ME::return_type_t<decltype(&fc)>

auto TEST() -> void {
    // std::map<std::string, std::function<double(double)>> func_map;

    ME::meta::any_function f{&TestRefleaction};

    METADOT_INFO(f.get_result_type().info->name());

    f.invoke({});
}

extern int test_reflection();

class ReflTestClass {
public:
    float f1;
    float f2;
    float a1[2];

    REFLECT(ReflTestClass, f1, f2, a1)
};

void init_reflection() {
    // test_reflection();

    Point p{1.f, 2.f};
    ME::meta::static_refl::TypeInfo<Point>::ForEachVarOf(p, [](const auto &field, auto &&var) {
        constexpr auto tstr_range = TSTR("UMeta::Range");
        if constexpr (decltype(field.attrs)::Contains(tstr_range)) {
            // auto r = attr_init(tstr_range, field.attrs.Find(tstr_range).value);
            std::cout << "[" << tstr_range.View() << "] " << std::endl;
        }
        std::cout << field.name << ": " << var << "\n====\n" << std::endl;
    });

    ReflTestClass testclass;
    testclass.f1 = 1200.f;
    testclass.f2 = 2.f;
    testclass.a1[0] = 1001.f;
    testclass.a1[1] = 2002.f;

    for (size_t i = 0; i < ReflTestClass::Class::TotalFields; i++) {
        ReflTestClass::Class::FieldAt(testclass, i, [&](auto &field, auto &value) { std::cout << field.name << ": " << value << std::endl; });
    }

    /*
    ExtendedTypeSupport defines many useful interfaces for these purposes...

        pair_lhs<T>::type // Gets the type of the left-hand value in an std::pair
        pair_rhs<T>::type // Gets the type of the right-hand value in an std::pair
        element_type<T>::type // Gets the type of the element contained in some iterable (be it a static_array, or STL container)
        is_pointable<T>::value // Checks whether a type is a regular or smart pointer
        remove_pointer<T>::type // Gets the type pointed to by a regular or smart pointer type
        static_array_size<T>::value // Gets the size of a static array, which may be a basic C++ array or the STL std::array type
        is_static_array<T>::value // Checks whether a type is a static array
        is_iterable<T>::value // Checks whether a type can be iterated, meaning it's a static array or other STL container
        is_map<T>::value // Checks whether a type is a map (std::map, std::unordered_map, std::multimap, std::unordered_multimap)
        is_stl_iterable<T>::value // Checks whether a type can be iterated with begin()/end()
        is_adaptor<T>::value // Checks whether a type is an STL adaptor (std::stack, std::queue, std::priority_queue)
        is_forward_list<T>::value // Checks whether a type is a forward list
        is_pair<T>::value // Checks whether a type is a pair
        is_bool<T>::value // Checks whether a type is a bool
        is_string<T>::value // Checks whether a type is a std::string
        has_push_back<T>::value // Checks whether a type is an STL container with a push_back method
        has_push<T>::value // Checks whether a type is an STL container with a push method
        has_insert<T>::value // Checks whether a type is an STL container with an insert method
        has_clear<T>::value // Checks whether a type is an STL container with a clear method

    Extended type support also provides several other pieces of functionality including...

        The Append, Clear, and IsEmpty methods that work on all stl containers
        The has_type interface to check whether a type is included in a list of types
        A get_underlying_container method to retrieve a const version of the underlying container for an STL adaptor
        An interface "get<T>::from(tuple)" to get the first instance of type T from a tuple
        An interface "for_each<T>::in(tuple)" to iterate each tuple element of type T
        An interface "for_each_in(tuple)" to iterate all tuple elements
        A TypeToStr method to retrieve a string representation of a type
    */
    ReflTestClass::Class::ForEachField(testclass, [&](auto &field, auto &value) {
        using Type = typename std::remove_reference<decltype(value)>::type;
        if constexpr (ExtendedTypeSupport::is_static_array<Type>::value)
            std::cout << field.name << ": " << value[0] << std::endl;
        else
            std::cout << field.name << ": " << value << std::endl;
    });
}
