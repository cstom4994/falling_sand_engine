// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "meta.hpp"

#include <chrono>
#include <functional>
#include <istream>
#include <map>
#include <string>

#include "core/core.hpp"
#include "core/debug.hpp"
#include "meta/json.h"
#include "meta/reflect.hpp"

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

#define RETURNTYPE(fc) MetaEngine::return_type_t<decltype(&fc)>

auto fuckme() -> void {
    // std::map<std::string, std::function<double(double)>> func_map;

    Meta::AnyFunction f{&TestRefleaction};

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

void InitCppReflection() {
    // test_reflection();

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

namespace MetaEngine {
auto tedtH() -> void {

    /*
    =================================================================
                                        F
                                       / \
         A                            H   \
        / \                          / \   \
       B   C                        I   J   G
      /   / \                        \ /   / \
     T   D   E                        K   L   Z
    ================================================================= */
    class A {};
    class F {};
    class B : public A {};
    class G : public F {};
    class C : public A {};
    class L : public G {};
    class T : public B {};
    class Z : public G {};
    class D : public C {};
    class H : public F {};
    class E : public C {};
    class I : public H {};
    class J : public H {};
    class K : public I, public J {};
    // =================================

    using namespace std;

    // using REGISTRY = typelist<I, C, Z, G, D, F, L, C, I, A, T, B, J, K, H, E, E>;
    using REGISTRY = typelist<A, B, C, D, E, F, G, H, I, J, K, L, T>;
    using D_ANCESTORS = find_ancestors<REGISTRY, D>::type;
    using T_ANCESTORS = find_ancestors<REGISTRY, T>::type;
    using K_ANCESTORS = find_ancestors<REGISTRY, K>::type;
    using D_EXPECTED = typelist<A, C, D>;
    using K_EXPECTED = typelist<F, H, J, I, K>;

    static_assert(is_same<D_ANCESTORS, D_EXPECTED>::value, "Ancestor of D test failed");
    static_assert(is_same<K_ANCESTORS, K_EXPECTED>::value, "Ancestor of K test failed");

    D d_instance;
    printf("The hierarchy tree of class D is:\n");
    hierarchy_iterator<D_ANCESTORS>::exec(&d_instance);
    printf("\n\n");

    T t_instance;
    printf("The hierarchy tree of class T is:\n");
    hierarchy_iterator<T_ANCESTORS>::exec(&t_instance);
    printf("\n\n");

    K k_instance;
    printf("The hierarchy tree of class K is:\n");
    hierarchy_iterator<K_ANCESTORS>::exec(&k_instance);
    printf("\n\n");
}

}  // namespace MetaEngine